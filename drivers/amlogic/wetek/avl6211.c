/*
 * Driver for the Availink AVL6211+AV2011 DVB-S/S2 demod+tuner
 *
 * Copyright (C) 2014 Sasa Savic <sasa.savic.sr@gmail.com>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/firmware.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/version.h>

#include "dvb_frontend.h"
#include "avl6211_reg.h"
#include "avl6211.h"


const struct avl6211_pllconf pll_conf[] = {
	/* The following set of PLL configuration at different reference clock frequencies refer to demod operation */
	/* in standard performance mode. */
	 { 503,  1, 7, 4, 2,  4000, 11200, 16800, 25200 } /* Reference clock 4 MHz,   Demod clock 112 MHz, FEC clock 168 MHz, MPEG clock 252 MHz */
	,{ 447,  1, 7, 4, 2,  4500, 11200, 16800, 25200 } /* Reference clock 4.5 MHz, Demod clock 112 MHz, FEC clock 168 MHz, MPEG clock 252 MHz */
	,{ 503,  4, 7, 4, 2, 10000, 11200, 16800, 25200 } /* Reference clock 10 MHz,  Demod clock 112 MHz, FEC clock 168 MHz, MPEG clock 252 MHz */
	,{ 503,  7, 7, 4, 2, 16000, 11200, 16800, 25200 } /* Reference clock 16 MHz,  Demod clock 112 MHz, FEC clock 168 MHz, MPEG clock 252 MHz */
	,{ 111,  2, 7, 4, 2, 27000, 11200, 16800, 25200 } /* Reference clock 27 MHz,  Demod clock 112 MHz, FEC clock 168 MHz, MPEG clock 252 MHz */
	
	/* The following set of PLL configuration at different reference clock frequencies refer to demod operation */
	/* in high performance mode. */
	,{ 566,  1, 7, 4, 2,  4000, 12600, 18900, 28350 } /* Reference clock 4 MHz,   Demod clock 126 MHz, FEC clock 189 MHz, MPEG clock 283.5 MHz */
	,{ 503,  1, 7, 4, 2,  4500, 12600, 18900, 28350 } /* Reference clock 4.5 MHz, Demod clock 126 MHz, FEC clock 189 MHz, MPEG clock 283.5 MHz */
	,{ 566,  4, 7, 4, 2, 10000, 12600, 18900, 28350 } /* Reference clock 10 MHz,  Demod clock 126 MHz, FEC clock 189 MHz, MPEG clock 283.5 MHz */
	,{ 566,  7, 7, 4, 2, 16000, 12600, 18900, 28350 } /* Reference clock 16 MHz,  Demod clock 126 MHz, FEC clock 189 MHz, MPEG clock 283.5 MHz */
	,{ 377,  8, 7, 4, 2, 27000, 12600, 18900, 28350 } /* Reference clock 27 MHz,  Demod clock 126 MHz, FEC clock 189 MHz, MPEG clock 283.5 MHz */
};

const unsigned short pll_array_size = sizeof(pll_conf) / sizeof(struct avl6211_pllconf);

struct avl6211_state
{
	struct i2c_adapter* i2c;
	struct avl6211_config* config;
	struct dvb_frontend frontend;
	
	u8 diseqc_status;
	u16 locked;
	u32 frequency;
	u32 symbol_rate;	
	u32 flags;
	
	int demod_id;
	
	u16 tuner_lpf;
	u16 demod_freq;	/* Demod clock in 10kHz units */
	u16 fec_freq;	/* FEC clock in 10kHz units */
	u16 mpeg_freq;	/* MPEG clock in 10kHz units */
	
	bool boot;
	bool gpio_on;
};
struct avl6211_diseqc_tx_status
{
	u8 tx_done;		
	u8 tx_fifo_cnt;
};
static u16 extract_16(const u8 * buf)
{
	u16 data;
	data = buf[0];
	data = (u16)(data << 8) + buf[1];
	return data;
}
static u32 extract_32(const u8 * buf)
{
	unsigned int data;
	data = buf[0];
	data = (data << 8) + buf[1];
	data = (data << 8) + buf[2];
	data = (data << 8) + buf[3];
	return data;
}
static int avl6211_i2c_writereg(struct avl6211_state *state, u8 *data, u16 *size)
{
	int ret;
	struct i2c_msg msg[1] = {
			{
				.addr = state->config->demod_address, 
				.flags = 0,
				.buf = data,
				.len = *size,
			}
	};
	
	ret = i2c_transfer(state->i2c, msg, 1);	
	if (ret == 1) {
		ret = 0;
	} else {
		dev_warn(&state->i2c->dev, "i2c wr failed=%d", ret);
		ret = -EREMOTEIO;
	}
	
	return ret;
}
static int avl6211_i2c_readreg(struct avl6211_state* state, u8 * data, u16 * size)
{
	int ret;
	struct i2c_msg msg[1] = {
			{
					.addr = state->config->demod_address, 
					.flags = I2C_M_RD,
					.buf = data,
					.len = *size,
			}
	};

	ret = i2c_transfer(state->i2c, msg, 1);
	
	if (ret == 1) {
		ret = 0;
	} else {
		dev_warn(&state->i2c->dev, "i2c rd failed=%d", ret);
		ret = -EREMOTEIO;
	}
	
	return ret;
}
static int avl6211_i2c_read(struct avl6211_state* state, u32 offset, u8 * buf, u16 buf_size)
{
	int ret;
	u8 buf_tmp[3];
	u16 x1 = 3, x2 = 0;
	u16 size;

	format_addr(offset, buf_tmp);
	ret = avl6211_i2c_writereg(state, buf_tmp, &x1);  
	if (ret)
		goto err;

	if (buf_size & 1)
		size = buf_size - 1;
	else
		size = buf_size;

	while (size > I2C_MAX_READ) {
		x1 = I2C_MAX_READ;
		ret = avl6211_i2c_readreg(state, buf + x2, &x1);
		if (ret)
			goto err;			
		x2 += I2C_MAX_READ;
		size -= I2C_MAX_READ;
	}

	if (size != 0) {
		ret = avl6211_i2c_readreg(state, buf + x2, &size);
		if (ret)
			goto err;
	}

	if (buf_size & 1) {
		x1 = 2;
		ret = avl6211_i2c_readreg(state, buf_tmp, &x1);
		if (ret)
			goto err;
		buf[buf_size-1] = buf_tmp[0];
	}
	
	return 0;
	
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_i2c_write(struct avl6211_state* state, u8 * buf, u16 buf_size)
{
	int ret;
	u8 buf_tmp[5], *x3;
	u16 x1, x2 = 0, tmp;
	u16 size;
	u32 addr;

	if (WARN_ON(buf_size < 3))
		return -EINVAL;	
		
	/* Actual data size */
	buf_size -= 3;
	/* Dump address */
	addr = buf[0];
	addr = addr << 8;
	addr += buf[1];
	addr = addr << 8;
	addr += buf[2];
	
	if (buf_size & 1)
		size = buf_size -1;
	else
		size = buf_size;
	
	tmp = (I2C_MAX_WRITE - 3) & 0xfffe; /* How many bytes data we can transfer every time */
	
	x2 = 0;
	while( size > tmp ) {
		x1 = tmp + 3;
		/* Save the data */
		buf_tmp[0] = buf[x2];
		buf_tmp[1] = buf[x2 + 1];
		buf_tmp[2] = buf[x2 + 2];
		x3 = buf + x2;
		format_addr(addr, x3);
		ret = avl6211_i2c_writereg(state, buf + x2, &x1);
		if (ret)
			goto err;
		/* Restore data */
		buf[x2] = buf_tmp[0];
		buf[x2 + 1] = buf_tmp[1];
		buf[x2 + 2] = buf_tmp[2];
		addr += tmp;
		x2 += tmp;
		size -= tmp;
	}

	x1 = size + 3;
	/* Save the data */
	buf_tmp[0] = buf[x2];
	buf_tmp[1] = buf[x2 + 1];
	buf_tmp[2] = buf[x2 + 2];
	x3 = buf + x2;
	format_addr(addr, x3);
	ret = avl6211_i2c_writereg(state, buf + x2, &x1);
	if (ret)
		goto err;
	/* Restore data */
	buf[x2] = buf_tmp[0];
	buf[x2 + 1] = buf_tmp[1];
	buf[x2 + 2] = buf_tmp[2];
	addr += size;
	x2 += size;
		
	if (buf_size & 1) {
		format_addr(addr, buf_tmp);
		x1 = 3;
		ret = avl6211_i2c_writereg(state, buf_tmp, &x1);
		if (ret)
			goto err;
		x1 = 2;
		ret = avl6211_i2c_readreg(state, buf_tmp + 3, &x1);
			goto err;
		buf_tmp[3] = buf[x2 + 3];
		x1 = 5;
		ret = avl6211_i2c_writereg(state, buf_tmp, &x1);
		if (ret)
			goto err;
	}
		
	return 0;

err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_i2c_read16(struct avl6211_state* state, u32 addr, u16 *data)
{
	int ret;
	u8 buf[2];

	ret = avl6211_i2c_read(state, addr, buf, 2);
	if (ret)
		goto err;
		
	*data = extract_16(buf);

	return 0;
	
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_i2c_read32(struct avl6211_state* state, u32 addr, u32 *data)
{
	int ret;
	u8 buf[4];

	ret = avl6211_i2c_read(state, addr, buf, 4);
	if (ret)
		goto err;
		
	*data = extract_32(buf);

	return 0;
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_i2c_write16(struct avl6211_state* state, u32 addr, u16 data)
{
	int ret;
	u8 buf[5], *p;

	format_addr(addr, buf);
	p = buf + 3;
	format_16(data, p);

	ret = avl6211_i2c_write(state, buf, 5);
	if (ret)
		goto err;
		
	return 0;

err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_i2c_write32(struct avl6211_state* state, u32 addr, u32 data)
{
	int ret;
	u8 buf[7], *p;

	format_addr(addr, buf);
	p = buf + 3;
	format_32(data, p);
	ret = avl6211_i2c_write(state, buf, 7);
	if (ret)
		goto err;
		
	return 0;
	
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_get_op_status(struct avl6211_state* state)
{
	int ret;
	u8 buf[2];

	ret = avl6211_i2c_read(state, rx_cmd_addr, buf, 2);
	if (ret)
		goto err;
		
	if (buf[1] != 0) {
		ret = -EINVAL;
		goto err;
	}
	
	return 0;
err:	
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_send_op(u8 ucOpCmd, struct avl6211_state* state)
{
	int ret;
	u8 buf[2];
	u16 x1;
	int cnt = 20;

	do {
		ret = avl6211_get_op_status(state);
		if (!ret)
			break;
	
		msleep(10);
		cnt--;
	} while (cnt != 0);
	
	if (ret)
		goto err;
		
	buf[0] = 0;
	buf[1] = ucOpCmd;
	x1 = extract_16(buf);
	ret = avl6211_i2c_write16(state, rx_cmd_addr, x1);   
	if (ret)
		goto err;
		
	return 0;
	
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_i2c_repeater_get_status(struct avl6211_state* state)
{
	int ret;
	u8 buf[2];

	ret = avl6211_i2c_read(state, i2cm_cmd_addr + I2CM_CMD_LENGTH - 2, buf, 2);
	if (ret)
		goto err;
		
	if (buf[1] != 0) {
		ret = -EINVAL;
		goto err;
	}
		
	return 0;
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}

static int avl6211_i2c_repeater_exec(struct avl6211_state* state, u8 * buf, u8 size)
{
	int ret, i = 0;

	do {
		ret = avl6211_i2c_repeater_get_status(state);
		if (ret && 60 < i++) 
			goto err;
			
		msleep(5);
	
	} while (ret);
	
	ret = avl6211_i2c_write(state, buf, size);
	if (ret)
		goto err;
		
	return 0;

err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_i2c_repeater_send(struct avl6211_state* state, u8 * buf, u16 size)
{
	int ret;
	u8 tmp_buf[I2CM_CMD_LENGTH + 3];
	u16 i, j;
	u16 cmd_size;

	if (WARN_ON(size > I2CM_CMD_LENGTH - 3))
		return -EINVAL;

	memset(tmp_buf, 0, sizeof(tmp_buf));
	
	cmd_size = ((size + 3) % 2) + 3 + size;
	format_addr(i2cm_cmd_addr + I2CM_CMD_LENGTH - cmd_size, tmp_buf);

	i = 3 + ((3 + size) % 2);	  /* skip one byte if the size +3 is odd */

	for (j = 0; j < size; j++)
		tmp_buf[i++] = buf[j];

	tmp_buf[i++] = (u8)size;
	tmp_buf[i++] = state->config->tuner_address;
	tmp_buf[i++] = OP_I2CM_WRITE;

			
	ret = avl6211_i2c_repeater_exec(state, tmp_buf, (u8)(cmd_size + 3));
	if (ret)
		goto err;
		
	return 0;

err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}

static int avl6211_i2c_repeater_recv(struct avl6211_state* state, u8 * buf, u16 size)
{
	int ret, i = 0;
	u8 tmp_buf[I2CM_RSP_LENGTH];

	if (WARN_ON(size > I2CM_RSP_LENGTH))
		return -EINVAL;

	memset(tmp_buf, 0, sizeof(tmp_buf));
	
	format_addr(i2cm_cmd_addr + I2CM_CMD_LENGTH - 4, tmp_buf);
	tmp_buf[3] = 0x0;
	tmp_buf[4] = (u8)size;
	tmp_buf[5] = state->config->tuner_address;
	tmp_buf[6] = OP_I2CM_READ;

	ret = avl6211_i2c_repeater_exec(state, tmp_buf, 7);
	if (ret)
		goto err;
	
	do {
		ret = avl6211_i2c_repeater_get_status(state);
		if (ret && 100 < i++) 
			goto err;
			
		msleep(10);
	
	} while (ret);

	ret = avl6211_i2c_read(state, i2cm_rsp_addr, buf, size);
	if (ret)
		goto err;
		
	return 0;

err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_i2c_repeater_init(u16 bus_clk, struct avl6211_state* state)
{
	u8 buf[5];
	int ret;

	ret = avl6211_i2c_write16(state, rc_i2cm_speed_kHz_addr, bus_clk);
	if (ret)
		goto err;
		
	format_addr(i2cm_cmd_addr + I2CM_CMD_LENGTH - 2, buf);
	buf[3] = 0x01;
	buf[4] = OP_I2CM_INIT;
	ret = avl6211_i2c_repeater_exec(state, buf, 5);
	if (ret)
		goto err;

	return 0;
	
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int AV2011_I2C_write(u8 reg_start, u8* buff, u8 len, struct avl6211_state* state)
{
	int ret, i = 0;
	u8 ucTemp[50] = { 0 };
	
	msleep(5);
	ucTemp[0] = reg_start;
	ret = avl6211_i2c_repeater_get_status(state);
	
	do {
		ret = avl6211_i2c_repeater_get_status(state);
		if (ret && 100 < i++) 
			goto err;
			
		msleep(1);
	
	} while (ret);
		
	for (i = 1; i < len + 1; i++)			
		ucTemp[i] = *(buff + i - 1);
			
	ret = avl6211_i2c_repeater_send(state, ucTemp, len+1);
	if (ret)
		goto err;
	
	msleep(5);
	return 0;
	
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}

static int av2011_tuner_lock_status(struct dvb_frontend* fe)
{
	struct avl6211_state *state = fe->demodulator_priv;	
	int ret;
	u8 lock = 0x0b;
	u8 locked = 0;
	ret = avl6211_i2c_repeater_send(state, &lock, 1);
	if (ret)
		goto err;
		
	ret = avl6211_i2c_repeater_recv(state, &locked, 1);
	if (ret)
		goto err;

	if (!(locked & 0x01)) 
		return -EINVAL;
		
	return 0;

err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int av2011_lock(struct dvb_frontend* fe)
{
	int ret;
	struct avl6211_state *state = fe->demodulator_priv;	
	u8 reg[50];
	u32 fracN;
	u32 BW;
	u32 BF;
	u32 freq = state->frequency / 1000;
	u32 LPF = state->tuner_lpf * 100;

	memset(reg, 0, sizeof(reg));

	msleep(50);
	
	fracN = (freq + 27/2) / 27;
	if (fracN > 0xff)
		fracN = 0xff;
		
	reg[0] = (char)(fracN & 0xff);
	fracN = (freq << 17) / 27;
	fracN = fracN & 0x1ffff;
	reg[1] = (char)((fracN >> 9) & 0xff);
	reg[2] = (char)((fracN >> 1) & 0xff);
	reg[3] = (char)((fracN << 7) & 0x80) | 0x50;

	BW = (LPF * 135) / 200;
	if (LPF < 6500)
		BW = BW + 6000;
   	BW = BW + 2000;
	BW = BW*108/100;

	if (BW < 4000)
		BW = 4000;
	if ( BW > 40000)
		BW = 40000;
	BF = (BW * 127 + 21100/2) / 21100; 
	
	dev_dbg(&state->i2c->dev, "BF is %d,BW is %d\n", BF, BW);
	
	reg[5] = (u8)BF;

	msleep(5);
	ret = AV2011_I2C_write(0, reg, 4, state);
	if (ret)
		goto err;
		
	msleep(5);
		
	ret = AV2011_I2C_write(5, reg+5, 1, state);
	if (ret)
		goto err;
		
	msleep(5);
			
	reg[37] = 0x06;
	ret = AV2011_I2C_write(37, reg+37, 1, state);
	if (ret)
		goto err;;
		
	msleep(5);
		
	reg[12] = 0x96 + (1 << 6);
	ret = AV2011_I2C_write(12, reg+12, 1, state);
	if (ret)
		goto err;

	return 0;
	
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int av2011_tuner_reg_init(struct dvb_frontend* fe)
{
	struct avl6211_state *state = fe->demodulator_priv;
    int ret;
	
	u8 reg[50] = { 
		0x38, 0x00, 0x00, 0x50, 0x1f, 0xa3, 0xfd, 0x58, 0x0e,
		0xc2, 0x88, 0xb4, 0xd6, 0x40, 0x94, 0x9a, 0x66, 0x40,
		0x80, 0x2b, 0x6a, 0x50, 0x91, 0x27, 0x8f, 0xcc, 0x21,
		0x10, 0x80, 0x02, 0xf5, 0x7f, 0x4a, 0x9b, 0xe0, 0xe0,
		0x36, 0x00, 0xab, 0x97, 0xc5, 0xa8,
	};
	
	ret = AV2011_I2C_write(0, reg, 12, state);
	if (ret)
		goto err;
		
	msleep(1);
		
	ret = AV2011_I2C_write(13, reg+13, 12, state);
	if (ret)
		goto err;
		
	ret = AV2011_I2C_write(25, reg+25, 11, state);
	if (ret)
		goto err;
		
	ret = AV2011_I2C_write(36, reg+36, 6, state);
	if (ret)
		goto err;
		
	msleep(1);
		
	ret = AV2011_I2C_write(12, reg+12, 1, state);
	if (ret)
		goto err;
		
	msleep(10);
	
	ret = AV2011_I2C_write(0, reg, 12, state);
	if (ret)
		goto err;
			
	msleep(1);
					
	ret = AV2011_I2C_write(13, reg+13 , 12, state);
	if (ret)
		goto err;
			
	ret = AV2011_I2C_write(25, reg+25 , 11, state);
	if (ret)
		goto err;
			
	ret = AV2011_I2C_write(36, reg+36, 6, state);
	if (ret)
		goto err;
			
	msleep(1);
		
	ret = AV2011_I2C_write(12, reg+12, 1, state);
	if (ret)
		goto err;
		
	msleep(5);
	
	return 0;

err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int av2011_tuner_init(struct dvb_frontend* fe) 
{
    struct avl6211_state *state = fe->demodulator_priv;
    int ret;
	
    ret = avl6211_i2c_write16(state, rc_tuner_slave_addr_addr, state->config->tuner_address);
	if (ret)
		goto err;
	/* Use external control */
	ret = avl6211_i2c_write16(state, rc_tuner_use_internal_control_addr, 0);
	if (ret)
		goto err;
	ret = avl6211_i2c_write16(state, rc_tuner_LPF_margin_100kHz_addr, 0);
	if (ret)
		goto err;
	ret = avl6211_i2c_write16(state, rc_tuner_max_LPF_100kHz_addr, 360 );
	if (ret)
		goto err;
	
    ret = avl6211_i2c_repeater_init(state->config->tuner_i2c_clock, state);
	if (ret)
		goto err;

	ret = av2011_tuner_reg_init(fe);
	if (ret)
		goto err;

	return 0;
	
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
    return ret;
}

static int avl6211_diseqc_init(struct dvb_frontend* fe)
{
	struct avl6211_state *state = fe->demodulator_priv;
	int ret;
	u32 x1;
	
	ret = avl6211_i2c_write32(state, diseqc_srst_addr, 1);
	if (ret)
		goto err;
	ret = avl6211_i2c_write32(state, diseqc_samp_frac_n_addr, 200);	/* 2M = 200 * 10kHz */
	if (ret)
		goto err;
	ret = avl6211_i2c_write32(state, diseqc_samp_frac_d_addr, state->demod_freq);
	if (ret)
		goto err;
	ret = avl6211_i2c_write32(state, diseqc_tone_frac_n_addr, (22 << 1));
	if (ret)
		goto err;
	ret = avl6211_i2c_write32(state, diseqc_tone_frac_d_addr, state->demod_freq * 10);
	if (ret)
		goto err;

	/* Initialize the tx_control */
	ret = avl6211_i2c_read32(state, diseqc_tx_cntrl_addr, &x1);
	if (ret)
		goto err;
	x1 &= 0x00000300;
	x1 |= 0x20;		/* Reset tx_fifo */
	x1 |= (u32)(0 << 6);
	x1 |= (u32)(0 << 4);
	x1 |= (1 << 3);			/* Enable tx gap */
	ret = avl6211_i2c_write32(state, diseqc_tx_cntrl_addr, x1);
	if (ret)
		goto err;
	x1 &= ~(0x20);	/* Release tx_fifo reset */
	ret = avl6211_i2c_write32(state, diseqc_tx_cntrl_addr, x1);
	if (ret)
		goto err;

	/* Initialize the rx_control */
	x1 = (u32)(0 << 2);
	x1 |= (1 << 1);	/* Activate the receiver */
	x1 |= (1 << 3);	/* Envelop high when tone present */
	ret = avl6211_i2c_write32(state, diseqc_rx_cntrl_addr, x1);
	if (ret)
		goto err;
	x1 = (u32)(0 >> 12);
	ret = avl6211_i2c_write32(state, diseqc_rx_msg_tim_addr, x1);
	if (ret)
		goto err;

	ret = avl6211_i2c_write32(state, diseqc_srst_addr, 0);
	if (ret)
		goto err;

	
	state->diseqc_status = DISEQC_STATUS_INIT;

	return 0;
	
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_diseqc_switch_mode(struct dvb_frontend* fe)
{
	struct avl6211_state *state = fe->demodulator_priv;
	int ret = 0;
	u32 x1;

	switch (state->diseqc_status) {
		case DISEQC_STATUS_MOD:
		case DISEQC_STATUS_TONE:
			ret = avl6211_i2c_read32(state, diseqc_tx_st_addr, &x1);
			if (ret)
				goto err;
			if (((x1 & 0x00000040) >> 6) != 1)
				ret = -EINVAL;
			break;
		case DISEQC_STATUS_CONTINUOUS:
		case DISEQC_STATUS_INIT:
			break;
		default:
			ret = -EINVAL;
			break;
	}
	if (ret)
		goto err;
	
	return 0;
	
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_diseqc_get_tx_status(struct dvb_frontend* fe, struct avl6211_diseqc_tx_status * pTxStatus)
{
	struct avl6211_state *state = fe->demodulator_priv;
	int ret;
	u32 x1;

	if ((state->diseqc_status == DISEQC_STATUS_MOD) || 
		(state->diseqc_status == DISEQC_STATUS_TONE)) {
		ret = avl6211_i2c_read32(state, diseqc_tx_st_addr, &x1);
		if (ret)
			goto err;
			
		pTxStatus->tx_done = (u8)((x1 & 0x00000040) >> 6);
		pTxStatus->tx_fifo_cnt = (u8)((x1 & 0x0000003c) >> 2);
	}
	else
		ret = -EINVAL;

	if (ret)
		goto err;
	
	return 0;
	
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_diseqc_send_mod_data(struct dvb_frontend* fe, const u8 * buf, u8 size)
{
	struct avl6211_state *state = fe->demodulator_priv;
	int ret;
	u32 x1, x2;
	int cnt = 0;
	u8 buf_tmp[8];
	u8 Continuousflag = 0;


	if (WARN_ON(size > 8))
		return -EINVAL;		
	else {
		ret = avl6211_diseqc_switch_mode(fe);
		if (ret)
			goto err;
		
		if (state->diseqc_status == DISEQC_STATUS_CONTINUOUS) {
			ret = avl6211_i2c_read32(state, diseqc_tx_cntrl_addr, &x1);
			if (ret)
				goto err;
			if ((x1 >> 10) & 0x01) {
				Continuousflag = 1;
				x1 &= 0xfffff3ff;
				ret = avl6211_i2c_write32(state, diseqc_tx_cntrl_addr, x1);
				if (ret)
					goto err;
				msleep(20);
			}
		}
			/* Reset rx_fifo */
		ret = avl6211_i2c_read32(state, diseqc_rx_cntrl_addr, &x2);
		if (ret)
			goto err;
		ret = avl6211_i2c_write32(state, diseqc_rx_cntrl_addr, (x2 | 0x01));
		if (ret)
			goto err;
		ret = avl6211_i2c_write32(state, diseqc_rx_cntrl_addr, (x2 & 0xfffffffe));
		if (ret)
			goto err;

		ret = avl6211_i2c_read32(state, diseqc_tx_cntrl_addr, &x1);
		if (ret)
			goto err;
		x1 &= 0xfffffff8;	//set to modulation mode and put it to FIFO load mode
		ret = avl6211_i2c_write32(state, diseqc_tx_cntrl_addr, x1);
		if (ret)
			goto err;
			
			/* Trunk address */
		format_addr(diseqc_tx_fifo_map_addr, buf_tmp);
		buf_tmp[3] = 0;
		buf_tmp[4] = 0;
		buf_tmp[5] = 0;
		for (x2 = 0; x2 < size; x2++) {
			buf_tmp[6] = buf[x2];
			ret = avl6211_i2c_write(state, buf_tmp, 7);
			if (ret)
				goto err;
		}

		x1 |= (1 << 2);  //start fifo transmit.
		ret = avl6211_i2c_write32(state, diseqc_tx_cntrl_addr, x1);
		if (ret)
			goto err;
		
		state->diseqc_status = DISEQC_STATUS_MOD;
		do 
		{
			msleep(1);
			if (++cnt > 500) {
				ret = -ETIME;
				goto err;
			}
			ret = avl6211_i2c_read32(state, diseqc_tx_st_addr, &x1);
			if (ret)
				goto err;
		} while ( 1 != ((x1 & 0x00000040) >> 6) );

		msleep(20);
		if (Continuousflag == 1)			//resume to send out wave
		{
			//No data in FIFO
			ret = avl6211_i2c_read32(state, diseqc_tx_cntrl_addr, &x1);
			if (ret)
				goto err;
			x1 &= 0xfffffff8; 
			x1 |= 0x03;		//switch to continuous mode
			ret = avl6211_i2c_write32(state, diseqc_tx_cntrl_addr, x1);
			if (ret)
				goto err;

			//start to send out wave
			x1 |= (1<<10);  
			ret = avl6211_i2c_write32(state, diseqc_tx_cntrl_addr, x1);
			if (ret)
				goto err;
			
			state->diseqc_status = DISEQC_STATUS_CONTINUOUS;
		}
	}

	return 0;
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;;
}

static int avl6211_send_diseqc_msg(struct dvb_frontend* fe, struct dvb_diseqc_master_cmd *d)
{
	struct avl6211_state *state = fe->demodulator_priv;
	struct avl6211_diseqc_tx_status tx_status;
	int cnt = 100;
	int ret;

	if ((d->msg_len < 3) || (d->msg_len > 6))
        	return -EINVAL;

	ret = avl6211_diseqc_send_mod_data(fe, d->msg, d->msg_len);
	if (ret)
		goto err;
	
	msleep(55);		
		
	do {
		ret = avl6211_diseqc_get_tx_status(fe, &tx_status);
		if (ret)
			goto err;
			
		if ( tx_status.tx_done == 1 )
			break;

		msleep(10);
		cnt--;
		if (!cnt) {
			ret = -ETIME;
			goto err;
		}
	} while (tx_status.tx_done != 1);

	return 0;
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}

static int avl6211_diseqc_send_burst(struct dvb_frontend* fe, enum fe_sec_mini_cmd burst)
{
	struct avl6211_state *state = fe->demodulator_priv;
	struct avl6211_diseqc_tx_status tx_status;
	int cnt = 100;
	int tx_cnt = 0;
	int ret;
	u32 x1;
	u8 buf[8];
	u8 Continuousflag = 0;
	
	ret = avl6211_diseqc_switch_mode(fe);
	if (ret)
		goto err;

	if (state->diseqc_status == DISEQC_STATUS_CONTINUOUS) {
		ret = avl6211_i2c_read32(state, diseqc_tx_cntrl_addr, &x1);
		if (ret)
			goto err;
		if ((x1 >> 10) & 0x01) {
			Continuousflag = 1;
			x1 &= 0xfffff3ff;
			ret = avl6211_i2c_write32(state, diseqc_tx_cntrl_addr, x1);
			if (ret)
				goto err;
			msleep(20);
		}
	}
	/* No data in the FIFO */
	ret = avl6211_i2c_read32(state, diseqc_tx_cntrl_addr, &x1);
	if (ret)
		goto err;
	x1 &= 0xfffffff8;  /* Put it into the FIFO load mode */
	if (burst == SEC_MINI_A)
		x1 |= 0x02;
	else
		x1 |= 0x01;
	ret = avl6211_i2c_write32(state, diseqc_tx_cntrl_addr, x1);
	if (ret)
		goto err;
	/* Trunk address */
	format_addr(diseqc_tx_fifo_map_addr, buf);
	buf[3] = 0;
	buf[4] = 0;
	buf[5] = 0;
	buf[6] = 1;

	ret = avl6211_i2c_write(state, buf, 7);
	if (ret)
		goto err;

	x1 |= (1<<2);  /* Start fifo transmit */
	ret = avl6211_i2c_write32(state, diseqc_tx_cntrl_addr, x1);
	if (ret)
		goto err;
	
	state->diseqc_status = DISEQC_STATUS_TONE;
	
	do 
	{
		msleep(1);
		if (++tx_cnt > 500) {
			ret = -ETIME;
			goto err;
		}
		ret = avl6211_i2c_read32(state, diseqc_tx_st_addr, &x1);
		if (ret)
			goto err;
	} while ( 1 != ((x1 & 0x00000040) >> 6) );

	msleep(20);
	
	if (Continuousflag == 1)			//resume to send out wave
	{
		//No data in FIFO
		ret = avl6211_i2c_read32(state, diseqc_tx_cntrl_addr, &x1);
		if (ret)
			goto err;
		x1 &= 0xfffffff8; 
		x1 |= 0x03;		//switch to continuous mode
		ret = avl6211_i2c_write32(state, diseqc_tx_cntrl_addr, x1);
		if (ret)
			goto err;

		//start to send out wave
		x1 |= (1<<10);  
		ret = avl6211_i2c_write32(state, diseqc_tx_cntrl_addr, x1);
		if (ret)
			goto err;
		
		state->diseqc_status = DISEQC_STATUS_CONTINUOUS;
			
	}	
	do {
		ret = avl6211_diseqc_get_tx_status(fe, &tx_status);
		if ( tx_status.tx_done == 1 )
			break;

		msleep(10);
		cnt--;
		if (!cnt) {
			ret = -ETIME;
			goto err;
		}
	} while (tx_status.tx_done != 1);
	
	return 0;
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}

static int avl6211_set_tone(struct dvb_frontend* fe, enum fe_sec_tone_mode tone)
{
	struct avl6211_state *state = fe->demodulator_priv;
	int ret;
	u32 x1;

	if (tone == SEC_TONE_ON) {
		
		ret = avl6211_diseqc_switch_mode(fe);
		if (ret)
			goto err;

		ret = avl6211_i2c_read32(state, diseqc_tx_cntrl_addr, &x1);
		if (ret)
			goto err;
		x1 &= 0xfffffff8;
		x1 |= 0x03;	
		ret = avl6211_i2c_write32(state, diseqc_tx_cntrl_addr, x1);
		if (ret)
			goto err;
		x1 |= (1 << 10);
		ret = avl6211_i2c_write32(state, diseqc_tx_cntrl_addr, x1);
		if (ret)
			goto err;
			
		state->diseqc_status = DISEQC_STATUS_CONTINUOUS;
	} else {
	
		if (state->diseqc_status == DISEQC_STATUS_CONTINUOUS) {
			ret = avl6211_i2c_read32(state, diseqc_tx_cntrl_addr, &x1);
			if (ret)
				goto err;
			x1 &= 0xfffff3ff;
			ret = avl6211_i2c_write32(state, diseqc_tx_cntrl_addr, x1);
			if (ret)
				goto err;
		}
	}
	
	return 0;
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}

static int avl6211_set_voltage(struct dvb_frontend *fe,
	enum fe_sec_voltage voltage)
{
	struct avl6211_state *state = fe->demodulator_priv;
	int ret;
	u32 x1;
	
	if (voltage == SEC_VOLTAGE_OFF) {
	
		if (state->config->set_external_vol_gpio)
			state->config->set_external_vol_gpio(&state->demod_id, 0);
		
		state->gpio_on = false;
		
		return 0;
	}
	if (voltage == SEC_VOLTAGE_13) {
		if (state->config->use_lnb_pin59) {
			ret = avl6211_i2c_read32(state, diseqc_tx_cntrl_addr, &x1);
			if (ret)
				goto err;
			x1 &= 0xfffffdff;		
			ret = avl6211_i2c_write32(state, diseqc_tx_cntrl_addr, x1);
			if (ret)
				goto err;
			msleep(20);	//delay 20ms
		}
		
		if (state->config->use_lnb_pin60) {
					
			ret = avl6211_i2c_read32(state, gpio_reg_enb, &x1);
			if (ret)
				goto err;
			x1 &= ~(1<<1);
			ret = avl6211_i2c_write32(state, gpio_reg_enb, x1);  
			if (ret)
				goto err;
			ret = avl6211_i2c_read32(state, gpio_data_reg_out, &x1);	
			if (ret)
				goto err;
			x1 &= ~(1<<1) ;			
			ret = avl6211_i2c_write32(state, gpio_data_reg_out, x1);
			if (ret)
				goto err;
			msleep(20);

		}						
	} else if (voltage == SEC_VOLTAGE_18) {
	
		if (state->config->use_lnb_pin59) {
			ret = avl6211_i2c_read32(state, diseqc_tx_cntrl_addr, &x1);
			if (ret)
				goto err;
			x1 &= 0xfffffdff;
			x1 |= 0x200;
			ret = avl6211_i2c_write32(state, diseqc_tx_cntrl_addr, x1);
			if (ret)
				goto err;
			msleep(20);	//delay 20ms
		}
		if (state->config->use_lnb_pin60) {
			ret = avl6211_i2c_read32(state, gpio_reg_enb, &x1);
			if (ret)
				goto err;
			x1 &= ~(1<<1);
			ret = avl6211_i2c_write32(state, gpio_reg_enb, x1);  
			if (ret)
				goto err;
			ret = avl6211_i2c_read32(state, gpio_data_reg_out, &x1);	
			if (ret)
				goto err;
			x1 |= 1<<1 ;			
			ret = avl6211_i2c_write32(state, gpio_data_reg_out, x1);
			if (ret)
				goto err;
			msleep(20);
		}		
	}
	
	if (!state->gpio_on) {	
		state->gpio_on = true;		
		if (state->config->set_external_vol_gpio) 
			state->config->set_external_vol_gpio(&state->demod_id, 1);
	}
	return 0;
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_read_ber(struct dvb_frontend* fe, u32* ber)
{
	struct avl6211_state *state = fe->demodulator_priv;
	int ret;
	u32 r_ber;

	*ber = 0;
	
	if (state->locked == 1) {		
		ret = avl6211_i2c_read32(state, rp_uint_BER_addr, &r_ber);
		if (ret)
			goto err;
			
		if (r_ber > 0)			
			*ber = r_ber / 1000000000;			
	}
	
	return 0;
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}

u8 DVBS_SNR[6] = { 12, 32, 41, 52, 58, 62 };
u8 DVBS2Qpsk_SNR[8] = { 10, 24, 32, 41, 47, 52, 63, 65 };
u8 DVBS28psk_SNR[6] = { 57, 67, 80, 95, 100, 110 };
static int avl6211_read_snr(struct dvb_frontend* fe, u16 *snr)
{
	struct avl6211_state *state = fe->demodulator_priv;
	int ret;
	u8 SNRrefer = 0;
	u32 r_snr, code_rate, modulation;

	*snr = 0;
	
	if (state->locked == 1) {
		ret = avl6211_i2c_read32(state, rs_int_SNR_dB_addr, &r_snr);
		if (ret)
			goto err;
		if (r_snr < 10000) {
			ret = avl6211_i2c_read32(state, rs_code_rate_addr, &code_rate);
			if (ret)
				goto err;
			ret = avl6211_i2c_read32(state, rs_modulation_addr, &modulation);
			if (ret)
				goto err;
				
			if (code_rate < 6)
				SNRrefer = DVBS_SNR[code_rate];			
			else {
				if (modulation == 1)
					SNRrefer = DVBS28psk_SNR[code_rate - 10];
				else	
					SNRrefer = DVBS2Qpsk_SNR[code_rate - 9];
			}	
			if ((r_snr / 10) > SNRrefer) {
				r_snr = r_snr/10 - SNRrefer;
				if (r_snr >= 100)
					*snr = 99;
				else if (r_snr >= 50)  //  >5.0dB
					*snr = 80+ (r_snr - 50)*20/50;
				else if (r_snr >= 25)  //  > 2.5dB
					*snr = 50+ (r_snr - 25)*30/25;
				else if (r_snr >= 10)  //  > 1dB
					*snr = 25+ (r_snr - 10)*25/15;			
				else 
					*snr = 5 + (r_snr)*20/10;
					
				*snr = (*snr * 65535) / 100;
			}
		}
	}
	
	return 0;
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
struct Signal_Level
{
	u16 SignalLevel;
	short SignalDBM;
};
struct Signal_Level  AGC_LUT [91]=
{
    {63688,  0},{62626, -1},{61840, -2},{61175, -3},{60626, -4},{60120, -5},{59647, -6},{59187, -7},{58741, -8},{58293, -9},
    {57822,-10},{57387,-11},{56913,-12},{56491,-13},{55755,-14},{55266,-15},{54765,-16},{54221,-17},{53710,-18},{53244,-19},
    {52625,-20},{52043,-21},{51468,-22},{50904,-23},{50331,-24},{49772,-25},{49260,-26},{48730,-27},{48285,-28},{47804,-29},
    {47333,-30},{46880,-31},{46460,-32},{46000,-33},{45539,-34},{45066,-35},{44621,-36},{44107,-37},{43611,-38},{43082,-39},
    {42512,-40},{41947,-41},{41284,-42},{40531,-43},{39813,-44},{38978,-45},{38153,-46},{37294,-47},{36498,-48},{35714,-49},
    {35010,-50},{34432,-51},{33814,-52},{33315,-53},{32989,-54},{32504,-55},{32039,-56},{31608,-57},{31141,-58},{30675,-59},
    {30215,-60},{29711,-61},{29218,-62},{28688,-63},{28183,-64},{27593,-65},{26978,-66},{26344,-67},{25680,-68},{24988,-69},
    {24121,-70},{23285,-71},{22460,-72},{21496,-73},{20495,-74},{19320,-75},{18132,-76},{16926,-77},{15564,-78},{14398,-79},
    {12875,-80},{11913,-81},{10514,-82},{ 9070,-83},{ 7588,-84},{ 6044,-85},{ 4613,-86},{ 3177,-87},{ 1614,-88},{  123,-89},
    {    0,-90}
};
static int avl6211_read_signal_strength(struct dvb_frontend* fe, u16* signal_strength)
{
	#define Level_High_Stage	36
	#define Level_Low_Stage		76

	#define Percent_Space_High	10
	#define Percent_Space_Mid	30
	#define Percent_Space_Low	60
	
	struct avl6211_state *state = fe->demodulator_priv;
	int ret;
	u32 rf;
	u16 Level;
	int i = 0;
	int Percent = 0;
	*signal_strength = 0;
		
	if (state->locked == 1) {
		ret = avl6211_i2c_read32(state, rx_aagc_gain, &rf);
		if (ret)
			goto err;
		
		rf += 0x800000;
		rf &= 0xffffff;	
		Level = (u16)(rf >> 8);

	while( Level < AGC_LUT[i++].SignalLevel);
	
	if (i <= Level_High_Stage)
		Percent = Percent_Space_Low+Percent_Space_Mid+ (Level_High_Stage-i)*Percent_Space_High/Level_High_Stage;
	else if(i<=Level_Low_Stage)
		Percent = Percent_Space_Low+ (Level_Low_Stage-i)*Percent_Space_Mid/(Level_Low_Stage-Level_High_Stage);
	else
		Percent =(90-i)*Percent_Space_Low/(90-Level_Low_Stage);

	*signal_strength = (Percent * 65535) / 100;	
	}
	
	return 0;
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_read_status(struct dvb_frontend* fe, enum fe_status* status)
{
	struct avl6211_state *state = fe->demodulator_priv;	
	int ret;
	*status = 0;
	
	ret = avl6211_i2c_read16(state, rs_fec_lock_addr, &state->locked);
	if (ret)
		goto err;

	if (state->locked == 1) 
		*status |= FE_HAS_SIGNAL | FE_HAS_CARRIER | 
				FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;
		
	return 0;
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_read_ucblocks(struct dvb_frontend* fe, u32* ucblocks)
{
	*ucblocks = 0;
	return 0;
}
static int avl6211_get_frontend(struct dvb_frontend* fe)
{
	struct avl6211_state *state = fe->demodulator_priv;
	struct dtv_frontend_properties *p = &fe->dtv_property_cache;
	u32 code_rate;
	u16 ret;
		
	if (!state->locked)
		return 0;
	
	ret = avl6211_i2c_read32(state, rs_code_rate_addr, &code_rate);
	if (ret)
		goto err;
		
	p->frequency = state->frequency;
	p->inversion = INVERSION_AUTO;
	p->symbol_rate = state->symbol_rate;
	
	switch (code_rate) { 
		case 0:
		p->fec_inner = FEC_1_2;
		break;
		case 1:
		p->fec_inner = FEC_2_3;
		break;
		case 2:
		p->fec_inner = FEC_3_4;
		break;
		case 13:
		p->fec_inner = FEC_4_5;
		break;
		case 14:
		p->fec_inner = FEC_5_6;
		break;
		case 4:
		p->fec_inner = FEC_6_7;
		break;
		case 5:
		p->fec_inner = FEC_7_8;
		break;
		case 15:
		p->fec_inner = FEC_8_9;
		break;
		case 10:
		p->fec_inner = FEC_3_5;
		break;
		case 16:
		p->fec_inner = FEC_9_10;
		break;
		default:
		p->fec_inner = FEC_AUTO;
		break;
	}
	return 0;
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_channel_lock(struct dvb_frontend* fe)
{
    struct avl6211_state *state = fe->demodulator_priv;
	int ret;
	u32 IQ;
	u32 autoIQ_Detect;
	u16 Standard;
	u16 auto_manual_lock;
	int cnt = 0;

	ret = avl6211_i2c_write16(state, rc_lock_mode_addr, 0);	
	if (ret)
		goto err;
		
	IQ = ((state->flags) & CI_FLAG_IQ_BIT_MASK) >> CI_FLAG_IQ_BIT;
	ret = avl6211_i2c_write32(state, rc_specinv_addr, IQ);
	if (ret)
		goto err;
	
	Standard = (u16)(((state->flags) & CI_FLAG_DVBS2_BIT_MASK) >> CI_FLAG_DVBS2_BIT);
	autoIQ_Detect = (((state->flags) & CI_FLAG_IQ_AUTO_BIT_MASK) >> CI_FLAG_IQ_AUTO_BIT);
	auto_manual_lock = (u16)(((state->flags) & CI_FLAG_MANUAL_LOCK_MODE_BIT_MASK) >> CI_FLAG_MANUAL_LOCK_MODE_BIT);

	
	if((Standard == CI_FLAG_DVBS2_UNDEF) || (autoIQ_Detect == 1))
		Standard = 0x14;

	if (state->symbol_rate == 0)
		state->symbol_rate = 1;
	
	ret = avl6211_i2c_write16(state, rc_fec_bypass_coderate_addr, auto_manual_lock);
	if (ret)
		goto err;
	ret = avl6211_i2c_write16(state, rc_decode_mode_addr, Standard);
	if (ret)
		goto err;
	ret = avl6211_i2c_write16(state, rc_iq_mode_addr, (u16)autoIQ_Detect);
	if (ret)
		goto err;
	ret = avl6211_i2c_write32(state, rc_int_sym_rate_MHz_addr, state->symbol_rate);
	if (ret)
		goto err;
	
	
	ret = avl6211_send_op(OP_RX_INIT_GO, state);
	if (ret)
		goto err;
		
	do {
		ret = avl6211_get_op_status(state);
		if(!ret)
			break;
		msleep(1);
	} while(cnt++ < 200);
	
	if (ret)
		goto err;
		
	return 0;
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_set_frontend(struct dvb_frontend* fe)
{
	struct avl6211_state *state = fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	int ret;
	u16 cnt;
	u32 max_time;
	
	
	state->frequency = c->frequency;
	state->symbol_rate = c->symbol_rate;
	
	state->locked = 0;

	dev_dbg(&state->i2c->dev, 
		"%s: delivery_system=%d frequency=%d symbol_rate=%d\n", 
		__func__, c->delivery_system, c->frequency, c->symbol_rate);

	state->tuner_lpf = (state->symbol_rate / 100000);
	if (state->tuner_lpf > 440)
		state->tuner_lpf = 440;

	ret = av2011_lock(fe);
	if (ret)
		goto err;

	/* Wait for tuner locking */
	max_time = 150;  /* Max waiting time: 150ms */

	cnt = max_time / 10;
	do {	
		ret = av2011_tuner_lock_status(fe);		

		if (!ret)
			break;
		else {		
			msleep(10);    /* Wait 10ms for demod to lock the channel */			
			continue;
		}		
		
	} while (--cnt);

	if (!cnt) {
		ret = -EAGAIN;
		goto err;
	}
	
	dev_dbg(&state->i2c->dev, "Tuner successfully lock!\n");

	state->flags = (CI_FLAG_IQ_NO_SWAPPED) << CI_FLAG_IQ_BIT;			//Normal IQ
	state->flags |= (CI_FLAG_IQ_AUTO_BIT_AUTO) << CI_FLAG_IQ_AUTO_BIT;	//Enable automatic IQ swap detection
	state->flags |= (CI_FLAG_DVBS2_UNDEF) << CI_FLAG_DVBS2_BIT;			//Enable automatic standard detection

	//This function should be called after tuner locked to lock the channel.
	ret = avl6211_channel_lock(fe);
	if (ret)
		goto err;
		
	/* Wait a bit more when we have slow symbol rates */
	if (c->symbol_rate < 5000000)
		max_time = 5000*2; /* Max waiting time: 1000ms */
	else if (c->symbol_rate < 10000000)
		max_time = 600*2;  /* Max waiting time: 600ms */
	else
		max_time = 250*2;  /* Max waiting time: 250ms */

	cnt = max_time / 10;
	do {
		ret = avl6211_i2c_read16(state, rs_fec_lock_addr, &state->locked);

		if (!ret && state->locked == 1)
				break;

		msleep(10);    /* Wait 10ms for demod to lock the channel */		
	} while (--cnt);
	
	if (!cnt) {
		ret = -EAGAIN;
		goto err;
	}
	dev_dbg(&state->i2c->dev, "Service locked!!!\n");

	ret = avl6211_send_op(OP_RX_RESET_BERPER, state);
	if (ret)
		goto err;
	
	return 0;
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
	
}

static int avl6211_get_demod_status(struct dvb_frontend* fe)
{
	struct avl6211_state *state = fe->demodulator_priv;
	int ret;
	u8	buf[2]; 
	u32 x1 = 0;
	
	ret = avl6211_i2c_read32(state, core_reset_b_reg, &x1);
	if (ret)
		goto err;
	ret = avl6211_i2c_read16(state, core_ready_word_addr, (u16 *)buf);	
	if (ret)
		goto err;
	
	if ((x1 == 0) || (buf[0] != 0x5a) || (buf[1] != 0xa5)) {
		ret = -EINVAL;
		goto err;
	}
	
	return 0;
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}

static int avl6211_setup_pll(struct avl6211_state* state, const struct avl6211_pllconf * pll_ptr)
{
	int ret;
	
	ret = avl6211_i2c_write32(state, pll_clkf_map_addr, pll_ptr->m_uiClkf);
	if (ret)
		goto err;
	ret = avl6211_i2c_write32(state, pll_bwadj_map_addr, pll_ptr->m_uiClkf);
	if (ret)
		goto err;
	ret = avl6211_i2c_write32(state, pll_clkr_map_addr, pll_ptr->m_uiClkr);
	if (ret)
		goto err;
	ret = avl6211_i2c_write32(state, pll_od_map_addr, pll_ptr->m_uiPllod);
	if (ret)
		goto err;
	ret = avl6211_i2c_write32(state, pll_od2_map_addr, pll_ptr->m_uiPllod2);
	if (ret)
		goto err;
	ret = avl6211_i2c_write32(state, pll_od3_map_addr, pll_ptr->m_uiPllod3);
	if (ret)
		goto err;
	ret = avl6211_i2c_write32(state, pll_softvalue_en_map_addr, 1);
	if (ret)
		goto err;
	ret = avl6211_i2c_write32(state, reset_register_addr, 0);
	if (ret)
		goto err;
	
	/* Reset do not check for error */
	avl6211_i2c_write32(state, reset_register_addr, 1);
	
	state->demod_freq = pll_ptr->demod_freq;
	state->fec_freq = pll_ptr->fec_freq;
	state->mpeg_freq = pll_ptr->mpeg_freq;

	return 0;
	
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static int avl6211_load_firmware(struct dvb_frontend* fe)
{
	struct avl6211_state* state = fe->demodulator_priv;
	const struct firmware *fw = NULL;
	u8 *buffer = NULL;
	u32 buf_size, data_size;
	u32 i = 4;
	int ret;
	
	ret = avl6211_i2c_write32(state, core_reset_b_reg, 0);
	if (ret)
		goto err;

	dev_dbg(&state->i2c->dev, "Uploading demod firmware (%s)...\n", AVL6211_DEMOD_FW);
	ret = request_firmware(&fw, AVL6211_DEMOD_FW, &state->i2c->dev);
	if (ret) {
		dev_dbg(&state->i2c->dev, "Firmware upload failed. Timeout or file not found\n");
		goto err;
	}    

	buffer = kmalloc(fw->size , GFP_KERNEL);
	if (!buffer) {
		release_firmware(fw);
		fw = NULL;
		dev_dbg(&state->i2c->dev, "Failed to allocate tmp memory for firmware\n");
		return -ENOMEM;
	}
	memcpy(buffer, fw->data, fw->size);
	
	release_firmware(fw);
	fw = NULL;
	
	data_size = extract_32(buffer);
	while (i < data_size)
	{
		buf_size = extract_32(buffer + i);
		i += 4;
		ret = avl6211_i2c_write(state, buffer + i + 1, (u16)(buf_size + 3));
		if (ret)
			goto err;
			
		i += 4 + buf_size;
	}
	
	ret = avl6211_i2c_write32(state, 0x00000000, 0x00003ffc);
	if (ret)
			goto err;
	ret = avl6211_i2c_write16(state, core_ready_word_addr, 0x0000);
	if (ret)
			goto err;
	ret = avl6211_i2c_write32(state, error_msg_addr, 0x00000000);
	if (ret)
			goto err;
	ret = avl6211_i2c_write32(state, error_msg_addr + 4, 0x00000000);
	if (ret)
			goto err;

	/* Reset do not check for error */
	avl6211_i2c_write32(state, core_reset_b_reg, 1);
	
	kfree(buffer);
	return 0;
	
err:
	kfree(buffer);
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}

static int avl6211_init(struct dvb_frontend* fe)
{
	struct avl6211_state* state = fe->demodulator_priv;
	int ret;

	if (state->boot)
		return 0;
		
	ret = avl6211_setup_pll(state, (const struct avl6211_pllconf * )(pll_conf + state->config->demod_refclk));
	if (ret)
		goto err;
		
	msleep(100);
	
	ret = avl6211_load_firmware(fe);
	if (ret)
		goto err;
		
	msleep(100);
	
	ret = avl6211_get_demod_status(fe);
	if (ret)
		goto err;
	
	
	ret = avl6211_i2c_write32(state, 0x263E, 50000);
	if (ret)
		goto err;
	/* Set clk to match the PLL */
	ret = avl6211_i2c_write16(state, rc_int_dmd_clk_MHz_addr,  state->demod_freq);
	if (ret)
		goto err;
	ret = avl6211_i2c_write16(state, rc_int_fec_clk_MHz_addr, state->fec_freq);
	if (ret)
		goto err;
	ret = avl6211_i2c_write16(state, rc_int_mpeg_clk_MHz_addr, state->mpeg_freq);
	if (ret)
		goto err;
	ret = avl6211_i2c_write32(state, rc_format_addr, 1);
	if (ret)
		goto err;
	
	/* Set AGC polarization */
	ret = avl6211_i2c_write32(state, rc_rfagc_pol_addr, (u32)state->config->tuner_rfagc);
	if (ret)
		goto err;	
	/* Drive RF AGC */
	ret = avl6211_i2c_write16(state, rc_aagc_ref_addr, 0x30);
	if (ret)
		goto err;
	ret = avl6211_i2c_write32(state, rc_rfagc_tri_enb, 1);
	if (ret)
		goto err;

	ret = avl6211_i2c_write16(state, rc_blind_scan_tuner_spectrum_inversion_addr, (u16)state->config->tuner_spectrum);
	if (ret)
		goto err;
	
	ret = avl6211_i2c_write32(state, rc_mpeg_mode_addr, (u32)(state->config->mpeg_format));
	if (ret)
		goto err;
	ret = avl6211_i2c_write16(state, rc_mpeg_serial_addr, (u16)(state->config->mpeg_mode));
	if (ret)
		goto err;
	ret = avl6211_i2c_write16(state, rc_mpeg_posedge_addr, (u16)(state->config->mpeg_pol));
	if (ret)
		goto err;
	
	if (state->config->mpeg_mode) {
		ret = avl6211_i2c_write32(state, rc_outpin_sel_addr, (u32)(state->config->mpeg_pin));		
		if (ret)
			goto err;
	}
	
	ret = avl6211_i2c_write32(state, rc_mpeg_bus_tri_enb, 1);
	if (ret)
		goto err;
	
	ret = av2011_tuner_init(fe);
	if (ret)
		goto err;
	ret = avl6211_diseqc_init(fe);
	if (ret)
		goto err;
	
	ret = avl6211_i2c_write32(state,  gpio_data_reg_out, 0);
	if (ret)
		goto err;
	
	ret = avl6211_i2c_write32(state, gpio_reg_enb, 0);
	if (ret)
		goto err;
	
	state->boot = true;
	
	dev_dbg(&state->i2c->dev, "AVL6211+AV2011 init OK\n");
	
	return 0;

err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}
static void avl6211_release(struct dvb_frontend* fe)
{
	struct avl6211_state* state = fe->demodulator_priv;
	kfree(state);
}

static struct dvb_frontend_ops avl6211_ops = {
	.delsys = { SYS_DVBS, SYS_DVBS2 }, 
	.info = {
		.name = "Availink AVL6211+AV2011 DVB-S/S2",	
		.frequency_min = 950000,
		.frequency_max = 2150000,
		.frequency_stepsize = 0,		
		.frequency_tolerance = 0,
		.symbol_rate_min = 800000,		/* Min = 800K */
		.symbol_rate_max = 50000000,	/* Max = 50M */
		.caps = FE_CAN_INVERSION_AUTO |
			FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
			FE_CAN_FEC_4_5 | FE_CAN_FEC_5_6 | FE_CAN_FEC_6_7 |
			FE_CAN_FEC_7_8 | FE_CAN_FEC_8_9 | FE_CAN_FEC_AUTO |
			FE_CAN_QPSK    | FE_CAN_RECOVER | FE_CAN_2G_MODULATION			
	},

	.init = avl6211_init,
	.release = avl6211_release,
	.read_status = avl6211_read_status,
	.read_ber = avl6211_read_ber,
	.read_signal_strength = avl6211_read_signal_strength,
	.read_snr = avl6211_read_snr,
	.read_ucblocks = avl6211_read_ucblocks,
	.set_tone = avl6211_set_tone,	
	.set_voltage = avl6211_set_voltage,
	.diseqc_send_master_cmd = avl6211_send_diseqc_msg,
	.diseqc_send_burst = avl6211_diseqc_send_burst,
	.set_frontend = avl6211_set_frontend,
	.get_frontend = avl6211_get_frontend,
};

struct dvb_frontend* avl6211_attach(struct i2c_adapter* i2c,
									struct avl6211_config* config,
									int id)
									
{
	struct avl6211_state* state = NULL;
	int ret;
	u32 ChipID = 0;
	
	state = kzalloc(sizeof(struct avl6211_state), GFP_KERNEL);
	if (!state) {
		ret = -ENOMEM;
		dev_err(&i2c->dev, "kzalloc() failed\n");
		goto err1;
	}

	state->config = config;
	state->i2c = i2c;	
	state->demod_id = id;
	
	ret = avl6211_i2c_read32(state, rs_cust_chip_id_addr, &ChipID);
	if (ret || ChipID != 0x0000000F)
		goto err2;

	dev_info(&i2c->dev, "AVL6211+AV2011 DVB-S/S2 successfully attached\n");	
	
	memcpy(&state->frontend.ops, &avl6211_ops, sizeof(struct dvb_frontend_ops));
	state->frontend.demodulator_priv = state;

	return &state->frontend;
	
err2:	
	kfree(state);
err1:	
	dev_dbg(&i2c->dev, "%s: failed=%d\n", __func__, ret);	
	return NULL;
}

EXPORT_SYMBOL(avl6211_attach);

MODULE_DESCRIPTION("Availink AVL6211+AV2011 demod+tuner driver");
MODULE_AUTHOR("Sasa Savic <sasa.savic.sr@gmail.com>");
MODULE_LICENSE("GPL");
