/*
 *  drivers/amlogic/uart/uart/meson_uart.c
 *
 *  Copyright (C) 2013 AMLOGIC, INC.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_reg.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/nmi.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <mach/io.h>
#include <mach/uart.h>
//#include <mach/pinmux.h>
#include <linux/clk.h>
#include <linux/vmalloc.h>
#include <linux/amlogic/uart-aml.h>

#include <asm/io.h>
#include <asm/irq.h>

#include "meson_uart.h"


#include <asm/serial.h>
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <mach/mod_gate.h>

/* UART name and device definitions */

#define MESON_SERIAL_NAME		"ttyS"
#define MESON_SERIAL_MAJOR	4
#define MESON_SERIAL_MINOR	64
#define MESON_SERIAL_BUFFER_SIZE (1024 * 128)
#define DEFAULT_STR_LEN	4
#define DEFAULT_STR   "..."
static int meson_uart_console_index =  -1;
struct console * meson_register_uart_console = NULL;
static int meson_uart_max_port = MESON_UART_PORT_NUM;
static struct uart_driver meson_uart_driver;
static struct aml_uart_platform  aml_uart_driver_data;
static const struct of_device_id meson_uart_dt_match[];
struct meson_uart_struct {
	struct console *co;
	const char *s;
	u_int offset;
	int count;
	struct list_head list;
};
struct meson_uart_management{
	int user_count;
	struct list_head list_head;
	struct spinlock lock;
};
struct meson_uart_list{
	int user_count;
	struct meson_uart_struct *co_head;
	struct meson_uart_struct *co_tail;
};
static struct meson_uart_management cur_col_management;
static struct meson_uart_list cur_col_list[MESON_UART_PORT_NUM];
static char * data_cache;
struct meson_uart_port {
	struct uart_port	port;

	/* We need to know the current clock divisor
	* to read the bps rate the chip has currently
	* loaded.
	*/
	char 		name[32];
	int 			baud;
	int			magic;
	int			baud_base;
	int			irq;
	int			flags; 		/* defined in tty.h */
	int			type; 		/* UART type */
	struct tty_struct 	*tty;
	int			read_status_mask;
	int			ignore_status_mask;
	int			timeout;
	int			custom_divisor;
	int			x_char;	/* xon/xoff character */
	int			close_delay;
	unsigned short		closing_wait;
	unsigned short		closing_wait2;
	unsigned long		event;
	unsigned long		last_active;
	int			line;
	int			count;	    /* # of fd on device */
	int			blocked_open; /* # of blocked opens */
	long			session; /* Session of opening process */
	long			pgrp; /* pgrp of opening process */

	int                  rx_cnt;
	int                  rx_error;

	struct mutex	info_mutex;

	struct tasklet_struct	tlet;
	struct work_struct	tqueue;
	struct work_struct	tqueue_hangup;

	wait_queue_head_t	open_wait;
	wait_queue_head_t	close_wait;
    /* bt wake control ops */
    struct bt_wake_ops *bt_ops;

	spinlock_t rd_lock;
	spinlock_t wr_lock;
	am_uart_t * uart;
	struct platform_device *pdev;
	struct aml_uart_platform *aup;
	struct pinctrl *p;
};

static struct meson_uart_port am_ports[MESON_UART_PORT_NUM];

static void meson_uart_start_port(struct meson_uart_port *mup);
void get_next_node(struct meson_uart_list *cur_col, struct meson_uart_struct *co_struct,
				   int index);

/**
 * Global printk mode.
 */
uint32_t g_printk_mode = 0;		/*0: old mode
                                                            1: new mode */
/**
 * This function show printk_mode.
 */
static ssize_t printk_mode_show(struct device_driver *drv, char *buf)
{
	return sprintf(buf, "0x%0x\n", g_printk_mode);
}

/**
 * This function stores printk_mode.
 */
static ssize_t printk_mode_store(struct device_driver *drv, const char *buf,
			       size_t count)
{
	g_printk_mode = simple_strtoul(buf, NULL, 16);
	return count;
}

static DRIVER_ATTR(printkmode, S_IRUGO | S_IWUSR, printk_mode_show,
		   printk_mode_store);

/*********************************************/
static struct bt_wake_ops *am_bt_ops = NULL;

void register_bt_wake_ops(struct bt_wake_ops *ops)
{
    am_bt_ops = ops;
}
EXPORT_SYMBOL(register_bt_wake_ops);

void unregister_bt_wake_ops(void)
{
    am_bt_ops = NULL;
}
EXPORT_SYMBOL(unregister_bt_wake_ops);

static void am_uart_stop_tx(struct uart_port *port)
{
	unsigned long mode;
	struct meson_uart_port * mup = &am_ports[port->line];
	am_uart_t *uart = mup->uart;


	//mutex_lock(&mup->info_mutex);
	preempt_disable();
	mode = readl(&uart->mode);
	mode &= ~UART_TXENB;
	writel(mode, &uart->mode);
	//mutex_unlock(&mup->info_mutex);
	preempt_enable();
}

static void am_uart_start_tx(struct uart_port *port)
{
	unsigned int ch;
	struct meson_uart_port * mup = &am_ports[port->line];
	am_uart_t *uart = mup->uart;
	struct uart_port * up = &mup->port;
	struct circ_buf *xmit = &up->state->xmit;
    unsigned long flags;

	spin_lock_irqsave(&mup->wr_lock, flags);
	while(!uart_circ_empty(xmit)){
		if (((readl(&uart->status) & UART_TXFULL) == 0)) {
			ch = xmit->buf[xmit->tail];
			writel(ch, &uart->wdata);
			xmit->tail = (xmit->tail+1) & (SERIAL_XMIT_SIZE - 1);
		}
        else
            break;
	}
	//mutex_unlock(&info->info_mutex);
	spin_unlock_irqrestore(&mup->wr_lock, flags);
}

static void am_uart_stop_rx(struct uart_port *port)
{
	unsigned long mode;
	struct meson_uart_port * mup = &am_ports[port->line];
	am_uart_t *uart = mup->uart;

	//mutex_lock(&mup->info_mutex);
	preempt_disable();
	mode = readl(&uart->mode);
	mode &= ~UART_RXENB;
	writel(mode, &uart->mode);
	preempt_enable();
	//mutex_unlock(&mup->info_mutex);
}

static void am_uart_enable_ms(struct uart_port *port)
{
	return;
}

unsigned int am_uart_tx_empty(struct uart_port *port)
{
	unsigned long mode;
	struct meson_uart_port * mup = &am_ports[port->line];
	am_uart_t *uart = mup->uart;

	mutex_lock(&mup->info_mutex);
	mode = readl(&uart->status);
	mutex_unlock(&mup->info_mutex);

	return ( mode & UART_TXEMPTY) ? TIOCSER_TEMT : 0;
}

static unsigned int am_uart_get_mctrl(struct uart_port *port)
{
	return TIOCM_CTS;
}

static void am_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	return;
}

static void am_uart_break_ctl(struct uart_port *port, int break_state)
{
      return;
}



#ifdef CONFIG_CONSOLE_POLL
/*
 * Console polling routines for writing and reading from the uart while
 * in an interrupt or debug context.
 */

static int am_uart_get_poll_char(struct uart_port *port)
{
	int rx;
	struct meson_uart_port * mup = &am_ports[port->line];
	am_uart_t *uart = mup->uart;

	if (!(readl(&uart->status) & UART_RXEMPTY))
		rx = readl(&uart->rdata);
	else
		rx = 0;

	return rx;
}


static void am_uart_put_poll_char(struct uart_port *port,
			 unsigned char c)
{
	struct meson_uart_port * mup = &am_ports[port->line];
	am_uart_t *uart = mup->uart;

	while ((readl(&uart->status) & UART_TXFULL))
		;
	writel(c, &uart->wdata);

    return;
}

#endif /* CONFIG_CONSOLE_POLL */

static int am_uart_startup(struct uart_port *port)
{
	unsigned long mode;
	struct meson_uart_port * mup = &am_ports[port->line];
	am_uart_t *uart = mup->uart;

	mutex_init(&mup->info_mutex);

	mutex_lock(&mup->info_mutex);
    aml_set_reg32_mask((uint32_t)&uart->mode, UART_RXRST|UART_TXRST|UART_CLEAR_ERR);
    aml_clr_reg32_mask((uint32_t)&uart->mode, UART_RXRST|UART_TXRST|UART_CLEAR_ERR);
	mode = readl(&uart->mode);
	mode |= UART_RXENB | UART_TXENB;
	writel(mode, &uart->mode);
	mutex_unlock(&mup->info_mutex);
	//meson_uart_start_port(mup);

	return 0;
}

static void am_uart_shutdown(struct uart_port *port)
{
    return;
}

static void meson_uart_change_speed(struct meson_uart_port *mup, unsigned long newbaud)
{
#ifndef CONFIG_MESON_CPU_EMULATOR
	am_uart_t *uart = mup->uart;
	unsigned long value;
	struct clk * clk81;
	struct clk * xtal;

	if (!newbaud || newbaud == mup->baud)
		return;

	if (IS_MESON_M8B_CPU || IS_MESON_M8M2_CPU) {
		msleep(1);
		xtal = clk_get_sys("xtal", NULL);
		value = (clk_get_rate(xtal) / 3) / newbaud - 1;
	}
	else {
		clk81 = clk_get_sys("clk81", "pll_fixed");
		if (IS_ERR_OR_NULL(clk81)) {
			printk(KERN_ERR "meson_uart_change_speed: clk81 is not available\n");
			return;
		}
		msleep(1);
		value = ((clk_get_rate(clk81) * 10 / (newbaud * 4) + 5) / 10) - 1;
	}
	while (!(readl(&uart->status) & UART_TXEMPTY))
		msleep(10);

	printk(KERN_INFO "Changing %s baud from %d to %d\n", mup->name,mup->baud, (int)newbaud);

	mup->baud = (int)newbaud;
#if MESON_CPU_TYPE < MESON_CPU_TYPE_MESON6
	value = (readl(&uart->mode) & ~0xfff) | (value & 0xfff);
	writel(value, &uart->mode);
#else
    value = (readl(&uart->reg5) & ~0x7fffff) | (value & 0x7fffff);

	value |= 0x800000;

	//Set USE_XTAL_CLK bit
	if(IS_MESON_M8B_CPU || IS_MESON_M8M2_CPU)
		value  |= 1<<24;

	writel(value, &uart->reg5);
#endif
	msleep(1);
#endif
}

static void
am_uart_set_termios(struct uart_port *port, struct ktermios *termios,
		       struct ktermios *old)
{
	unsigned int baud;
	struct meson_uart_port * mup = &am_ports[port->line];
    am_uart_t *uart = mup->uart;
    unsigned tmp;

    tmp = readl(&uart->mode);
    switch (termios->c_cflag & CSIZE) {
    case CS8:
        printk(KERN_DEBUG "config %s: Character length 8bits/char\n", mup->name);
        tmp &= ~(0x3 << 20);
        break;
    case CS7:
        printk(KERN_DEBUG "config %s: Character length 7bits/char\n", mup->name);
        tmp &= ~(0x1 << 21);
        tmp |= (0x1 << 20);
        break;
    case CS6:
        printk(KERN_DEBUG "config %s: Character length 6bits/char\n", mup->name);
        tmp |= 0x1 << 21;
        tmp &= ~(0x1 << 20);
        break;
    case CS5:
        printk(KERN_DEBUG "config %s: Character length 5bits/char\n", mup->name);
        tmp |= 0x3 << 20;
        break;
    default:
        printk(KERN_DEBUG "default config %s: Character length 8bits/char\n", mup->name);
        tmp &= ~(0x3 << 20);
        break;
    }

    if(PARENB & termios->c_cflag) {
        tmp |= 0x1 << 19;
        if(PARODD & termios->c_cflag) {
            tmp |= 0x1<<18;
        }
        else {
        tmp &=~(0x1 << 18);
        }
    }
    else {
        tmp &=~(0x1 << 19);
    }

    if(termios->c_cflag & CSTOPB) {
        tmp |= 0x1 << 16 ;
        tmp &=~(0x1 << 17);
    } else {
        tmp &=~(0x3 << 16);
    }

    if(termios->c_cflag & CRTSCTS) {
        tmp &= ~(0x1 << 15);
    } else {
        tmp |= (0x1 << 15);
    }
    writel(tmp, &uart->mode);

    baud = tty_termios_baud_rate(termios);
    if(baud){
        meson_uart_change_speed(mup, baud);
        uart_update_timeout(port, termios->c_cflag, baud);
    }
	return;
}

static void
am_uart_set_ldisc(struct uart_port *port,int new)
{
	return;
}

static void
am_uart_pm(struct uart_port *port, unsigned int state,
	      unsigned int oldstate)
{
	return;
}

static void am_uart_release_port(struct uart_port *port)
{
	return;
}

static int am_uart_request_port(struct uart_port *port)
{
	 return 0;
}

static void am_uart_config_port(struct uart_port *port, int flags)
{
	return;
}

static int
am_uart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	return 0;
}

static const char *
am_uart_type(struct uart_port *port)
{
	return NULL;
}

static int am_uart_ioctl(struct uart_port *port, unsigned int cmd, unsigned long arg)
{
    struct meson_uart_port * mup = &am_ports[port->line];
    //void __user *uarg = (void __user *)arg;
    int ret = 0;

    switch(cmd) {
    case TIOCSETBTPORT:
        ret = am_bt_ops->setup_bt_port(port);
        if(!ret)
            mup->bt_ops = am_bt_ops;
        break;
    case TIOCCLRBTPORT:
        mup->bt_ops = NULL;
        break;
    default:
        ret = -ENOIOCTLCMD;
        //printk(KERN_ERR "%s not support cmd\n", __FUNCTION__);
    }

    return ret;
}

static struct uart_ops am_uart_pops = {
	.tx_empty	= am_uart_tx_empty,
	.set_mctrl	= am_uart_set_mctrl,
	.get_mctrl	= am_uart_get_mctrl,
	.stop_tx	= am_uart_stop_tx,
	.start_tx	= am_uart_start_tx,
	.stop_rx	= am_uart_stop_rx,
	.enable_ms	= am_uart_enable_ms,
	.break_ctl	= am_uart_break_ctl,
	.startup	= am_uart_startup,
	.shutdown	= am_uart_shutdown,
	.set_termios	= am_uart_set_termios,
	.set_ldisc	= am_uart_set_ldisc,
	.pm		= am_uart_pm,
	.type		= am_uart_type,
	.release_port	= am_uart_release_port,
	.request_port	= am_uart_request_port,
	.config_port	= am_uart_config_port,
	.verify_port	= am_uart_verify_port,
	.ioctl      = am_uart_ioctl,
#ifdef CONFIG_CONSOLE_POLL
	.poll_get_char = am_uart_get_poll_char,
	.poll_put_char = am_uart_put_poll_char,
#endif
};

/*********************************************/
/*
 * This routine is used by the interrupt handler to schedule
 * processing in the software interrupt portion of the driver.
 */
static inline void meson_uart_sched_event(struct meson_uart_port *mup, int event)
{
    mup->event |= 1 << event;
    schedule_work(&mup->tqueue);
}

static void meson_receive_chars(struct meson_uart_port *mup, struct pt_regs *regs,
			  unsigned short rx)
{
	char ch;
	int status;
	int mode;
	unsigned long flag = TTY_NORMAL;
	//struct tty_struct *tty = mup->port.state->port.tty;
	struct tty_port *tport = &mup->port.state->port;
	am_uart_t *uart = mup->uart;

	if (!tport) {
		//printk("Uart : missing tty on line %d\n", info->line);
		goto clear_and_exit;
	}
	status = readl(&uart->status);
	if (status & UART_OVERFLOW_ERR) {
              mup->rx_error |= UART_OVERFLOW_ERR;
              mode = readl(&uart->mode) | UART_CLEAR_ERR;
		writel(mode, &uart->mode);
	} else if (status & UART_FRAME_ERR) {
		mup->rx_error |= UART_FRAME_ERR;
		mode = readl(&uart->mode) | UART_CLEAR_ERR;
		writel(mode, &uart->mode);
	} else if (status & UART_PARITY_ERR) {
		mup->rx_error |= UART_PARITY_ERR;
		mode = readl(&uart->mode) | UART_CLEAR_ERR;
		writel(mode, &uart->mode);
	}

	do {
		ch = (rx & 0x00ff);
                tty_insert_flip_char(tport,ch,flag);

		mup->rx_cnt++;
        status = (readl(&uart->status) & UART_RXEMPTY);
		if (!status)
			rx = readl(&uart->rdata);
	} while (!status);

clear_and_exit:
	return;
}

static void meson_BH_receive_chars(struct meson_uart_port *mup)
{
	//struct tty_struct *tty = mup->port.state->port.tty;
	struct tty_port *tport = &mup->port.state->port;
	unsigned long flag;
	int status;
	int cnt = mup->rx_cnt;

	if (!tport) {
		printk(KERN_ERR"Uart : missing tty on line %d\n", mup->line);
		goto clear_and_exit;
	}

	tport->low_latency = 1;	// Originally added by Sameer, serial I/O slow without this
	flag = TTY_NORMAL;
	status = mup->rx_error;
	if (status & UART_OVERFLOW_ERR) {
		printk(KERN_ERR"Uart %d Driver: Overflow Error while receiving a character\n", mup->line);
		flag = TTY_OVERRUN;
	} else if (status & UART_FRAME_ERR) {
		printk(KERN_ERR"Uart %d Driver: Framing Error while receiving a character\n", mup->line);
		flag = TTY_FRAME;
	} else if (status & UART_PARITY_ERR) {
		printk(KERN_ERR"Uart %d Driver: Parity Error while receiving a character\n", mup->line);
		flag = TTY_PARITY;
	}

	mup->rx_error = 0;
	if(cnt){
		mup->rx_cnt =0;
		tty_flip_buffer_push(tport);
	}

clear_and_exit:
	if (mup->rx_cnt > 0)
		meson_uart_sched_event(mup, 0);

	return;
}

static void meson_uart_workqueue(struct work_struct *work)
{

    struct meson_uart_port *mup=container_of(work,struct meson_uart_port,tqueue);
    am_uart_t *uart = NULL;
    struct tty_struct *tty = NULL;

    if (!mup || !mup->uart)
        goto out;

    uart = mup->uart;
    tty = mup->port.state->port.tty;
    if (!tty){
        goto out;
    }

    if (mup->rx_cnt > 0)
        meson_BH_receive_chars(mup);

    if (readl(&uart->status) & UART_FRAME_ERR)
        writel(readl(&uart->status) & ~UART_FRAME_ERR,&uart->status);
    if (readl(&uart->status) & UART_OVERFLOW_ERR)
        writel(readl(&uart->status) & ~UART_OVERFLOW_ERR,&uart->status);
out:
    return ;
}

static void meson_transmit_chars(struct meson_uart_port *mup)
{
    am_uart_t *uart = mup->uart;
    struct uart_port * up = &mup->port;
    unsigned int ch;
    struct circ_buf *xmit = &up->state->xmit;
    int count = 256;

    if (up->x_char) {
        writel(up->x_char, &uart->wdata);
        up->x_char = 0;
        goto clear_and_return;
    }

    if (uart_circ_empty(xmit) || uart_tx_stopped(up))
        goto clear_and_return;

    spin_lock(&mup->wr_lock);
    while(!uart_circ_empty(xmit) && count-- > 0){
        if((readl(&uart->status) & UART_TXFULL) ==0) {
            ch = xmit->buf[xmit->tail];
            writel(ch, &uart->wdata);
            xmit->tail = (xmit->tail+1) & (SERIAL_XMIT_SIZE - 1);
        }
	    else {
		    break;
        }
    }
    spin_unlock(&mup->wr_lock);
clear_and_return:

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(up);

    return;
}

int print_more(struct meson_uart_port *mup);
/*
 * This is the serial driver's generic interrupt routine
 */
static irqreturn_t meson_uart_interrupt(int irq, void *dev, struct pt_regs *regs)
{
	struct meson_uart_port *mup=(struct meson_uart_port *)dev;
    am_uart_t *uart = NULL;
    struct tty_struct *tty = NULL;


	if (!mup || !mup->uart)
        goto out;

    uart = mup->uart;
    tty = mup->port.state->port.tty;
    if (!tty){
        goto out;
    }

    if (!(readl(&uart->status) & UART_RXEMPTY)){
        meson_receive_chars(mup, 0, readl(&uart->rdata));
    }

    if ((readl(&uart->mode) & UART_TXENB)&&!(readl(&uart->status) & UART_TXFULL)) {
        meson_transmit_chars(mup);
    }

    if(!list_empty(&cur_col_management.list_head)){
	 print_more(mup);
    }
out:
    meson_uart_sched_event(mup, 0);
    return IRQ_HANDLED;
}

static void meson_uart_start_port(struct meson_uart_port *mup)
{
	int index = mup->line;
	struct uart_port *up = &mup->port;
	am_uart_t *uart = mup->uart;

//	void * pinmux = mup->aup->pinmux_uart[index];
    unsigned int fifo_level = mup->aup->fifo_level[mup->line];

	mup->magic = SERIAL_MAGIC;

	mup->custom_divisor = 16;
	mup->close_delay = 50;
	mup->closing_wait = 100;
	mup->event = 0;
	mup->count = 0;
	mup->blocked_open = 0;

	mup->rx_cnt= 0;

	init_waitqueue_head(&mup->open_wait);
	init_waitqueue_head(&mup->close_wait);

	INIT_WORK(&mup->tqueue,meson_uart_workqueue);

	//tasklet_init(&mup->tlet, meson_uart_tasklet_action,
	// (unsigned long)mup);
	if(of_get_property(mup->pdev->dev.of_node, "pinctrl-names", NULL)){
		mup->p=devm_pinctrl_get_select_default(&mup->pdev->dev);
		if (IS_ERR(mup->p)){
			printk(KERN_ERR"meson_uart request pinmux error!\n");
		}
		/* set pinmux here */
		printk("set %s pinmux use pinctrl subsystem\n",mup->aup->port_name[index]);
		printk("P_AO_RTI_PIN_MUX_REG:%x\n",aml_read_reg32(P_AO_RTI_PIN_MUX_REG));
	}
	// need put pinctrl
	aml_set_reg32_mask((uint32_t)&uart->mode, UART_RXRST);
	aml_clr_reg32_mask((uint32_t)&uart->mode, UART_RXRST);
	aml_set_reg32_mask((uint32_t)&uart->mode, UART_RXINT_EN | UART_TXINT_EN);

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
	writel( (fifo_level/2) << 8 | 1, &uart->intctl);
#else
	writel((fifo_level/2) << 7 | 1, &uart->intctl);
#endif

	aml_clr_reg32_mask((uint32_t)&uart->mode, (1 << 19)) ;

	sprintf(mup->name,"%s_ttyS%d:",mup->aup->port_name[index],index);
	if (request_irq(up->irq, (irq_handler_t) meson_uart_interrupt, IRQF_SHARED,
		mup->name, mup)) {
			printk(KERN_ERR"meson_uart request irq error!\n");
	}

	printk(KERN_NOTICE"start %s(irq = %d)\n", mup->name,up->irq);
}

static int meson_uart_register_port(struct platform_device *pdev,struct aml_uart_platform * aup,int port_index)
{
	int ret;
	struct uart_port * up;
	struct meson_uart_port *mup;

	mup = &am_ports[port_index];

	mup->line = port_index;
	mup->pdev = pdev;
	mup->aup = aup;
	mup->uart = aup->regaddr[port_index];
	spin_lock_init(&mup->wr_lock);
	up = &mup->port;
	up->dev = &pdev->dev;
	up->iotype	= UPIO_PORT;
	up->flags	= UPF_BOOT_AUTOCONF;
	up->ops	= &am_uart_pops;
	up->fifosize	= 1;
	up->line	= port_index;
	up->irq	= aup->irq_no[port_index];
	up->type =1;
	up->x_char = 0;
	spin_lock_init(&up->lock);

	if(meson_uart_console_index == port_index){
		up->cons = meson_register_uart_console;
		up->cons->flags |= CON_ENABLED;
	}

	ret = uart_add_one_port(&meson_uart_driver, up);
    meson_uart_start_port(mup);

    platform_set_drvdata(pdev, mup);

	printk(KERN_NOTICE"register %s %s\n", aup->port_name[port_index],
		ret ? "failed" : "ok" );

       return ret;
}


/***************************************/

static inline struct aml_uart_platform   *aml_get_driver_data(
			struct platform_device *pdev, int * port_index)
{
	int index;
	const char * port_name, * enable;
	struct aml_uart_platform * aup = NULL;

	if (pdev->dev.of_node) {
		const struct of_device_id *match;
		match = of_match_node(meson_uart_dt_match, pdev->dev.of_node);
		if(match){
			port_name = of_get_property(pdev->dev.of_node, "port_name", NULL);
			enable = of_get_property(pdev->dev.of_node, "status", NULL);

			/*
			  * Find matched port index by port name
			  *  the index indecate the location of driver data arrary
			 */
			aup = &aml_uart_driver_data;
			for(index = 0; index < meson_uart_max_port; index++)
				if(!strcasecmp(port_name,aup->port_name[index]))
					break;

			if(index < meson_uart_max_port){
				*port_index = index;

				//printk(" %s (index: %d) %s\n", port_name, index,enable);
				switch_mod_gate_by_name(aup->clk_name[index], 1);
				return aup;
			}
		}
	}

	return NULL;
	//return (struct aml_uart_platform *)
	//		platform_get_device_id(pdev)->driver_data;
}



static int meson_uart_probe(struct platform_device *pdev)
{
	int index = -1;
	struct aml_uart_platform *uart_data;

	uart_data = aml_get_driver_data(pdev, &index);


	if(!uart_data)
		return -ENODEV;

       return meson_uart_register_port(pdev,uart_data,index);
}

static int meson_uart_remove(struct platform_device *pdev)
{
       int ret = 0;

	/* Todo, unregister ports */

	return ret;
}

#ifdef CONFIG_AM_UART_CONSOLE
/*
 * Output a single character, using UART polled mode.
 * This is used for console output.
 */
static void inline am_uart_put_char(am_uart_t *uart, int index,char ch)
{

	while ((readl(&uart->status) & UART_TXFULL)) ;
	writel(ch, &uart->wdata);
}

static int  meson_serial_console_setup(struct console *co, char *options)
{
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';
	int index;
	am_uart_t *uart;

	printk(KERN_DEBUG"meson_serial_console_setup\n");

	/*
	 * Check whether an invalid uart number has been specified, and
	 * if so, search for the first available port that does have
	 * console support.
	 */
	if (co->index >= MESON_UART_PORT_NUM)
		co->index = 0;

	/*
	 * Find line index to match co->index
	 */
	for(index = 0; index < MESON_UART_PORT_NUM; index++){
		if(aml_uart_driver_data.line[index] == co->index)
			break;
	}

	if(index >= MESON_UART_PORT_NUM){
		printk(KERN_ERR"console index not matched\n");
		return -ENODEV;
	}

	uart = aml_uart_driver_data.regaddr[index];

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	if (baud < 300 || baud > 115200)
		baud = 115200;

//	am_uart_init_port(port,index);
	meson_uart_console_index = index;
	meson_register_uart_console = co;
//	return uart_set_options(&am_ports->port,co,baud,parity,bits,flow)
	return 0;
}

bool new_printk_enabled = 0;
static int __init new_printk_enable(char *str)
{
	new_printk_enabled = 1;
	return 1;
}
__setup("printk_no_block", new_printk_enable);
module_param_named(new_printk, new_printk_enabled,
		bool, S_IRUGO | S_IWUSR);

MODULE_PARM_DESC(new_printk, "printk use no block mode"
	" ");

static void meson_serial_console_write(struct console *co, const char *s, u_int count)
{
	struct meson_uart_struct *head_s = NULL;
	struct meson_uart_struct *tail_s = NULL;
	struct list_head *list_head = NULL;
	const char *cur_s = NULL;
	int tmp_count = 0, struct_size = sizeof(struct meson_uart_struct);
	int tmp_count_1 = 0;
	struct meson_uart_list *cur_col = NULL;
	struct meson_uart_struct *co_struct = NULL;
	am_uart_t *uart = NULL;
	unsigned long flags = 0;
	int index = co->index;
	int mode;
	int need_default=0;
	static int last_msg_full = 0;

#ifdef CONFIG_PRINTK_NOBLOCK_MODE
	mode = 1;
#else
	mode = 0;
#endif

	if(g_printk_mode||new_printk_enabled)
		mode = 1-mode;

	if(!mode  || !data_cache){
		uart = aml_uart_driver_data.regaddr[index];
		if (index!= meson_uart_console_index)
			meson_serial_console_setup(co, NULL);
		while (count-- > 0) {
			if (*s == '\n')
				am_uart_put_char(uart, index, '\r');
			am_uart_put_char(uart, index, *s++);
		}
		return;
	}
	spin_lock_irqsave(&cur_col_management.lock, flags);
	if(count > 0 && count < MESON_SERIAL_BUFFER_SIZE){
		list_head = &cur_col_management.list_head;
		cur_s = NULL;
		if(list_empty(list_head)){
			cur_s = data_cache;
		}
		else{
			head_s = list_entry(list_head->prev, struct meson_uart_struct, list);
			tail_s = list_entry(list_head->next, struct meson_uart_struct, list);
			tmp_count_1 = (int)((struct_size << 1) + tail_s->s + tail_s->offset + tail_s->count);
			if(head_s->s <= tail_s->s){
				tmp_count = data_cache + MESON_SERIAL_BUFFER_SIZE - tail_s->s - tail_s->offset - tail_s->count;
				if(tmp_count < count + struct_size+(struct_size+DEFAULT_STR_LEN)){
					if((head_s->s > (data_cache + (struct_size << 1))) &&
					   ((head_s->s - (data_cache + (struct_size << 1))) > count+(struct_size+DEFAULT_STR_LEN))){
						cur_s = data_cache;
					}
					else{
						cur_s = data_cache;
						need_default = 1;
					}
				}
				else{
					cur_s = tail_s->s + tail_s->offset + tail_s->count;
				}
			}
			else if(((int)head_s->s > tmp_count_1) &&
					((int)head_s->s - tmp_count_1 > count+(struct_size+DEFAULT_STR_LEN))){
				cur_s = tail_s->s + tail_s->offset + tail_s->count;
			}
			else{
				cur_s =  tail_s->s + tail_s->offset + tail_s->count;
				need_default = 1;
			}
		}

		if(need_default==0){
			co_struct = (struct meson_uart_struct *)cur_s;
			co_struct->s = cur_s + struct_size;
			co_struct->count = count;
			co_struct->co = co;
			co_struct->offset = 0;
			cur_col = &cur_col_list[index];
			cur_col->co_tail = co_struct;
			if(!cur_col->co_head){
				cur_col->co_head = co_struct;
			}
			cur_col->user_count++;
			cur_col_management.user_count++;
			memcpy((char *)(co_struct->s), (char *)s, count);
			
			list_add(&co_struct->list, &cur_col_management.list_head);
			last_msg_full = 0; 
		}
		else
		{
			if(last_msg_full==0)
			{
				co_struct = (struct meson_uart_struct *)cur_s;
				co_struct->s = cur_s + struct_size;
				co_struct->count = DEFAULT_STR_LEN;
				co_struct->co = co;
				co_struct->offset = 0;
				cur_col = &cur_col_list[index];
				cur_col->co_tail = co_struct;
				if(!cur_col->co_head){
					cur_col->co_head = co_struct;
				}
				cur_col->user_count++;
				cur_col_management.user_count++;
				memcpy((char *)(co_struct->s), DEFAULT_STR, DEFAULT_STR_LEN);
				s = co_struct->s;
				*((char *)s + DEFAULT_STR_LEN-1) = '\n';
			
				list_add(&co_struct->list, &cur_col_management.list_head);
				last_msg_full = 1;
			}
		}

		cur_col = &cur_col_list[index];
		if( cur_col->co_head){
			co_struct = cur_col->co_head;
			s = co_struct->s + co_struct->offset;
			uart = aml_uart_driver_data.regaddr[index];
			if (index!= meson_uart_console_index)
				meson_serial_console_setup(co, NULL);
			while ((readl(&uart->mode) & UART_TXENB)&&
				   !(readl(&uart->status) & UART_TXFULL)) {
				if(co_struct->count <= 0){
					get_next_node(cur_col, co_struct, index);
					if(!cur_col->co_head)
						break;
					co_struct = cur_col->co_head;
					s = co_struct->s + co_struct->offset;
				}
				if (*s == '\n')
					writel('\r', &uart->wdata);
				writel(*s++, &uart->wdata);
				co_struct->count--;
				co_struct->offset++;
			}
		}

	}
	spin_unlock_irqrestore(&cur_col_management.lock, flags);
		
	return;
}

static int meson_uart_resume(struct platform_device *pdev)
{
    unsigned tmp;
    struct meson_uart_port *mup = (struct meson_uart_port *)platform_get_drvdata(pdev);

    printk(KERN_DEBUG"meson_uart_resume\n");

    if(mup->bt_ops) {
	    tmp = readl(&mup->uart->mode);
	    writel(tmp & (~(0x1 <<31)), &(mup->uart->mode));
	    printk(KERN_DEBUG "disable Invert the RTS signal %lx \n",mup->uart->mode);
    }

    return 0;
}

static int meson_uart_suspend(struct platform_device *pdev, pm_message_t state)
{
    unsigned tmp;
    struct meson_uart_port *mup = (struct meson_uart_port *)platform_get_drvdata(pdev);

    if(mup->bt_ops) {
        if(mup->bt_ops->bt_can_sleep()) {
            tmp = readl(&(mup->uart->mode));
            writel(tmp | (0x1 << 31), &(mup->uart->mode));
            printk(KERN_DEBUG "Invert the RTS signal %lx \n",mup->uart->mode);
        }
        else {
            printk("bt is busy\n");
            return -EBUSY;
        }
    }
    return 0;
}

struct console meson_serial_console = {
	.name		= MESON_SERIAL_NAME,
	.write		= meson_serial_console_write,
	.device		= uart_console_device,
	.setup		= meson_serial_console_setup,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
	.data		= &meson_uart_driver,
};

static int __init meson_serial_console_init(void)
{
	register_console(&meson_serial_console);
	return 0;
}
console_initcall(meson_serial_console_init);

#define MESON_SERIAL_CONSOLE	&meson_serial_console
#else
#define MESON_SERIAL_CONSOLE	NULL
#endif

/***************************************************/

static struct aml_uart_platform  aml_uart_driver_data = {
	.line 	= {MESON_UART_LINE},
	.port_name={MESON_UART_NAME},
	.regaddr 	= {MESON_UART_ADDRS},
	.irq_no 	= {MESON_UART_IRQS},
	.fifo_level = {MESON_UART_FIFO},
	.clk_name = {MESON_UART_CLK_NAME},
};

#define MESON_SERIAL_DRV_DATA ((kernel_ulong_t)&aml_uart_driver_data)


static const struct of_device_id meson_uart_dt_match[]={
	{	.compatible 	= "amlogic,aml_uart",
		.data		= (void *)MESON_SERIAL_DRV_DATA
	},
	{},
};
MODULE_DEVICE_TABLE(of,meson_uart_dt_match);
static struct platform_device_id meson_uart_driver_ids[] = {
	{
		.name		= "aml-uart",
		.driver_data	= MESON_SERIAL_DRV_DATA,
	},
	{},
};
MODULE_DEVICE_TABLE(platform, meson_uart_driver_ids);
/*
static struct ktermios meson_std_termios = {
	.c_iflag = ICRNL | IXON,
	.c_oflag = OPOST | ONLCR,
	.c_cflag = B115200 | CS8 | CREAD | HUPCL,
	.c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK |
		   ECHOCTL | ECHOKE | IEXTEN,
	.c_cc = INIT_C_CC,
	.c_ispeed = 115200,
	.c_ospeed = 115200
};
*/
static struct uart_driver meson_uart_driver = {
	.owner		= THIS_MODULE,
	.driver_name	= "serial",
	.nr		= MESON_UART_PORT_NUM,
	.cons		= MESON_SERIAL_CONSOLE,
	.dev_name	= MESON_SERIAL_NAME,
	.major		= MESON_SERIAL_MAJOR,
	.minor		= MESON_SERIAL_MINOR,
};

static  struct platform_driver meson_uart_platform_driver = {
	.probe		= meson_uart_probe,
	.remove		= meson_uart_remove,
	.suspend	= meson_uart_suspend,
	.resume		= meson_uart_resume,
	.id_table	= meson_uart_driver_ids,
	.driver		= {
		.name	= "mesonuart",
		.owner	= THIS_MODULE,
		.of_match_table=meson_uart_dt_match,
	},
};
void get_next_node(struct meson_uart_list *cur_col, struct meson_uart_struct *co_struct,
				   int index)
{
	struct list_head *entry;
	struct console *co = NULL;
	struct list_head *list_head = NULL;
	

	cur_col->user_count--;
	
	list_del(&co_struct->list);
	
	co_struct = NULL;
	list_head = &cur_col_management.list_head;
	if(!list_empty(list_head)){
		cur_col_management.user_count--;
		list_for_each_prev(entry, list_head){
			co_struct = list_entry(entry, struct meson_uart_struct, list);
			co = co_struct->co;
			if(co->index == index)
				break;
		}
	}
	if(co_struct && (co->index == index)){
		cur_col->co_head = co_struct;
	}
	else{
		cur_col->co_head = NULL;
		cur_col->co_tail = NULL;
		cur_col->user_count = 0;
	}
}
int print_more(struct meson_uart_port *mup)
{
	int index = 0;
	struct meson_uart_struct *co_struct = NULL;
	struct meson_uart_port *mup_tmp = NULL;
	struct meson_uart_list *cur_col = NULL;
	struct console *co = NULL;
	am_uart_t *uart = NULL;
	const char *s = NULL;
	unsigned long flags = 0;
	spin_lock_irqsave(&cur_col_management.lock, flags);
	if(!data_cache || list_empty(&cur_col_management.list_head)){
		goto no_data_print;
	}
	for(index = 0; index < MESON_UART_PORT_NUM; index++){
		mup_tmp = &am_ports[index];
		if(mup == mup_tmp)
			break;
	}
	if(index >= MESON_UART_PORT_NUM)
		goto no_data_print;

	cur_col = &cur_col_list[index];
	if((cur_col->user_count == 0) || !cur_col->co_head)
		goto no_data_print;

	co_struct = cur_col->co_head;
	co = co_struct->co;
	index = co->index;
	if(index < 0)
		goto no_data_print;

	uart = aml_uart_driver_data.regaddr[index];
	s = co_struct->s + co_struct->offset;

	spin_lock(&mup->wr_lock);
	if (index!= meson_uart_console_index)
		meson_serial_console_setup(co, NULL);
	while ((readl(&uart->mode) & UART_TXENB)&&!(readl(&uart->status) & UART_TXFULL)) {
		if(co_struct->count <= 0){
			get_next_node(cur_col, co_struct, index);
			if(!cur_col->co_head)
				break;
			co_struct = cur_col->co_head;
			s = co_struct->s + co_struct->offset;
		}
		if (*s == '\n')
			writel('\r', &uart->wdata);
		writel(*s++, &uart->wdata);
		co_struct->count--;
		co_struct->offset++;
	}

	spin_unlock(&mup->wr_lock);
	
no_data_print:
	spin_unlock_irqrestore(&cur_col_management.lock, flags);
	return 0;
}
/*****************************************************/
static int __init meson_uart_init(void)
{
	int ret;
	int error;
	ret = uart_register_driver(&meson_uart_driver);
	if (ret){
		printk(KERN_ERR"meson_uart_driver register failed\n");
		return ret;
	}
    meson_uart_driver.tty_driver->init_termios.c_cflag = B115200 | CS8 | CREAD | HUPCL | CLOCAL;
	meson_uart_driver.tty_driver->init_termios.c_ispeed = meson_uart_driver.tty_driver->init_termios.c_ospeed = 115200;
	//meson_uart_driver.tty_driver->init_termios  = meson_std_termios;

	ret = platform_driver_register(&meson_uart_platform_driver);
	if (ret){
		printk(KERN_ERR"meson_uart_platform_driver register failed\n");
		uart_unregister_driver(&meson_uart_driver);
	}
	error = driver_create_file(&meson_uart_platform_driver.driver, &driver_attr_printkmode);
	INIT_LIST_HEAD(&cur_col_management.list_head);
	spin_lock_init(&cur_col_management.lock);
	data_cache = vmalloc(MESON_SERIAL_BUFFER_SIZE);
	if(!data_cache){
		printk("buffer alloc failed for uart\n");
		platform_driver_unregister(&meson_uart_platform_driver);
		uart_unregister_driver(&meson_uart_driver);
		return -ENOMEM;
	}

	return ret;
}
static void __exit meson_uart_exit(void)
{
	vfree(data_cache);
	driver_remove_file(&meson_uart_platform_driver.driver, &driver_attr_printkmode);
	platform_driver_unregister(&meson_uart_platform_driver);
	uart_unregister_driver(&meson_uart_driver);
}

module_init(meson_uart_init);
module_exit(meson_uart_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Amlogic Meson Uart driver");
