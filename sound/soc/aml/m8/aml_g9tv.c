/*
 * aml_g9tv.c  --  amlogic  G9TV sound card machine driver code.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/jack.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>

#include <linux/switch.h>
#include <linux/amlogic/saradc.h>
#include <linux/clk.h>
#include <mach/register.h>
#include <sound/tas57xx.h>

#include "aml_i2s_dai.h"
#include "aml_i2s.h"
#include "aml_g9tv.h"
#include "aml_audio_hw.h"

#ifdef CONFIG_USE_OF
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/amlogic/aml_audio_codec_probe.h>
#include <linux/of_gpio.h>
#include <mach/pinmux.h>
#include <plat/io.h>
#endif

#define DRV_NAME "aml_snd_g9tv"

extern int ext_codec;
extern struct device *spdif_dev;
codec_info_t codec_info_aux;
static struct codec_probe_priv prob_priv;
struct codec_probe_priv {
    int num_eq;
    struct tas57xx_eq_cfg *eq_configs;
};

static int aml_asoc_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *params) {
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *codec_dai = rtd->codec_dai;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
    int ret;

    printk(KERN_DEBUG "enter %s stream: %s rate: %d format: %d\n", __func__,
            (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture",
            params_rate(params), params_format(params));

    /* set codec DAI configuration */
    ret = snd_soc_dai_set_fmt(codec_dai,
            SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
    if (ret < 0) {
        printk(KERN_ERR "%s: set codec dai fmt failed!\n", __func__);
        return ret;
    }

    /* set cpu DAI configuration */
    ret = snd_soc_dai_set_fmt(cpu_dai,
            SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_IB_NF | SND_SOC_DAIFMT_CBM_CFM);
    if (ret < 0) {
        printk(KERN_ERR "%s: set cpu dai fmt failed!\n", __func__);
        return ret;
    }

    if (!strncmp(codec_info.name_bus, "dummy_codec", 11)) {
        goto cpu_dai;
    }

    /* set codec DAI clock */
    ret = snd_soc_dai_set_sysclk(codec_dai, 0, params_rate(params) * 256,
            SND_SOC_CLOCK_IN);
    if (ret < 0) {
        printk(KERN_ERR "%s: set codec dai sysclk failed (rate: %d)!\n", __func__, params_rate(params));
        return ret;
    }

    cpu_dai:
    /* set cpu DAI clock */
    ret = snd_soc_dai_set_sysclk(cpu_dai, 0, params_rate(params) * 256,
            SND_SOC_CLOCK_OUT);
    if (ret < 0) {
        printk(KERN_ERR "%s: set cpu dai sysclk failed (rate: %d)!\n", __func__, params_rate(params));
        return ret;
    }

    return 0;
}

static struct snd_soc_ops aml_asoc_ops = {
    .hw_params = aml_asoc_hw_params,
};

struct aml_audio_private_data *p_audio;
static bool aml_audio_i2s_mute_flag = 0;
static bool aml_audio_spdif_mute_flag = 0;
static int aml_audio_Hardware_resample = 0;

static int aml_audio_set_i2s_mute(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol) {
    aml_audio_i2s_mute_flag = ucontrol->value.integer.value[0];
    printk("aml_audio_i2s_mute_flag: flag=%d\n", aml_audio_i2s_mute_flag);
    if (aml_audio_i2s_mute_flag) {
        aml_audio_i2s_mute();
    } else {
        aml_audio_i2s_unmute();
    }
    return 0;
}

static int aml_audio_get_i2s_mute(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol) {
    ucontrol->value.integer.value[0] = aml_audio_i2s_mute_flag;
    return 0;
}

static int aml_audio_set_spdif_mute(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol) {

    aml_audio_spdif_mute_flag = ucontrol->value.integer.value[0];
    printk("aml_audio_set_spdif_mute: flag=%d\n", aml_audio_spdif_mute_flag);
    if (aml_audio_spdif_mute_flag) {
        aml_spdif_pinmux_deinit(spdif_dev);
    } else {
        aml_spdif_pinmux_init(spdif_dev);
    }
    return 0;
}

static int aml_audio_get_spdif_mute(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol) {
    ucontrol->value.integer.value[0] = aml_audio_spdif_mute_flag;
    return 0;
}

static const char *audio_in_source_texts[] = { "LINEIN", "ATV", "HDMI" };

static const struct soc_enum audio_in_source_enum = SOC_ENUM_SINGLE(
        SND_SOC_NOPM, 0, ARRAY_SIZE(audio_in_source_texts),
        audio_in_source_texts);

static int aml_audio_get_in_source(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol) {
    int value = READ_MPEG_REG(AUDIN_SOURCE_SEL) & 0x3;

    if (value == 0)
    ucontrol->value.enumerated.item[0] = 0; // linein
    else if (value == 1)
    ucontrol->value.enumerated.item[0] = 1;//ATV
    else if (value == 2)
    ucontrol->value.enumerated.item[0] = 2;//hdmi
    else
    printk(KERN_INFO "unknown source\n");

    return 0;
}

static int aml_audio_set_in_source(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol) {
    if (ucontrol->value.enumerated.item[0] == 0) { // select external codec output as I2S source
        WRITE_MPEG_REG(AUDIN_SOURCE_SEL, 0);
        WRITE_MPEG_REG(AUDIN_I2SIN_CTRL,
                (1 << I2SIN_CHAN_EN) | (3 << I2SIN_SIZE)
                        | (1 << I2SIN_LRCLK_INVT) | (1 << I2SIN_LRCLK_SKEW)
                        | (0 << I2SIN_POS_SYNC) | (1 << I2SIN_LRCLK_SEL)
                        | (1 << I2SIN_CLK_SEL) | (1 << I2SIN_DIR));
        WRITE_MPEG_REG_BITS(AUDIN_I2SIN_CTRL, 1, I2SIN_EN, 1);
    } else if (ucontrol->value.enumerated.item[0] == 1) { // select ATV output as I2S source
        WRITE_MPEG_REG(AUDIN_SOURCE_SEL, 1);
        WRITE_MPEG_REG(AUDIN_I2SIN_CTRL,
                (1 << I2SIN_CHAN_EN) | (0 << I2SIN_SIZE)
                        | (0 << I2SIN_LRCLK_INVT) | (0 << I2SIN_LRCLK_SKEW)
                        | (0 << I2SIN_POS_SYNC) | (0 << I2SIN_LRCLK_SEL)
                        | (0 << I2SIN_CLK_SEL) | (0 << I2SIN_DIR));
        WRITE_MPEG_REG_BITS(AUDIN_I2SIN_CTRL, 1, I2SIN_EN, 1);
    } else if (ucontrol->value.enumerated.item[0] == 2) { // select HDMI-rx as I2S source
        WRITE_MPEG_REG(AUDIN_SOURCE_SEL, (0 << 12) | // [14:12]cntl_hdmirx_chsts_sel: 0=Report chan1 status; 1=Report chan2 status; ...;
                (0xf << 8) | // [11:8] cntl_hdmirx_chsts_en
                (1 << 4) | // [5:4]  spdif_src_sel: 1=Select HDMIRX SPDIF output as AUDIN source
                (2 << 0)); // [1:0]  i2sin_src_sel: 2=Select HDMIRX I2S output as AUDIN source
        WRITE_MPEG_REG(AUDIN_I2SIN_CTRL,
                (1 << I2SIN_CHAN_EN) | (3 << I2SIN_SIZE)
                        | (1 << I2SIN_LRCLK_INVT) | (1 << I2SIN_LRCLK_SKEW)
                        | (0 << I2SIN_POS_SYNC) | (1 << I2SIN_LRCLK_SEL)
                        | (1 << I2SIN_CLK_SEL) | (1 << I2SIN_DIR));
        WRITE_MPEG_REG_BITS(AUDIN_I2SIN_CTRL, 1, I2SIN_EN, 1);
    }
    return 0;
}

/* HDMI audio format detect: LPCM or NONE-LPCM */
static const char *hdmi_audio_type_texts[] = { "LPCM", "NONE-LPCM", "UN-KNOWN" };
static const struct soc_enum hdmi_audio_type_enum = SOC_ENUM_SINGLE(
        SND_SOC_NOPM, 0, ARRAY_SIZE(hdmi_audio_type_texts),
        hdmi_audio_type_texts);

static int aml_hdmi_audio_type_get_enum(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol) {
    int ch_status = 0;
    if ((READ_MPEG_REG(AUDIN_DECODE_CONTROL_STATUS) >> 24) & 0x1) {
        ch_status = READ_MPEG_REG(AUDIN_DECODE_CHANNEL_STATUS_A_0);
        if (ch_status & 2) //NONE-LPCM
            ucontrol->value.enumerated.item[0] = 1;
        else
            //LPCM
            ucontrol->value.enumerated.item[0] = 0;
    } else
        ucontrol->value.enumerated.item[0] = 2; //un-stable. un-known

    return 0;
}

static int aml_hdmi_audio_type_set_enum(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol) {
    return 0;
}

/* spdif in audio format detect: LPCM or NONE-LPCM */
static const char *spdif_audio_type_texts[] =
        { "LPCM", "NONE-LPCM", "UN-KNOWN" };
static const struct soc_enum spdif_audio_type_enum = SOC_ENUM_SINGLE(
        SND_SOC_NOPM, 0, ARRAY_SIZE(spdif_audio_type_texts),
        spdif_audio_type_texts);

static int aml_spdif_audio_type_get_enum(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol) {
    int ch_status = 0;
    //if ((READ_MPEG_REG(AUDIN_SPDIF_MISC)>>0x7)&0x1){
    ch_status = READ_MPEG_REG(AUDIN_SPDIF_CHNL_STS_A) & 0x3;
    if (ch_status & 2) //NONE-LPCM
        ucontrol->value.enumerated.item[0] = 1;
    else
        //LPCM
        ucontrol->value.enumerated.item[0] = 0;
    //}
    //else
    //    ucontrol->value.enumerated.item[0] = 2; //un-stable. un-known
    return 0;
}

static int aml_spdif_audio_type_set_enum(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol) {
    return 0;
}

#define RESAMPLE_BUFFER_SOURCE 1 //audioin fifo0
#define RESAMPLE_CNT_CONTROL 255 //Cnt_ctrl = mclk/fs_out-1 ; fest 256fs
static int hardware_resample_enable(int input_sr) {
    struct clk* clk_src;
    unsigned long clk_rate = 0;
    unsigned short Avg_cnt_init = 0;

    if (input_sr < 8000 || input_sr > 48000) {
        printk(KERN_INFO "Error input sample rate, you must set sr first!\n");
        return -1;
    }

    clk_src = clk_get_sys("clk81", NULL); // get clk81 clk_rate
    clk_rate = clk_get_rate(clk_src);
    Avg_cnt_init = (unsigned short) (clk_rate * 4 / input_sr);

    printk(KERN_INFO "clk_rate = %ld,input_sr = %d,Avg_cnt_init = %d\n",clk_rate,input_sr,Avg_cnt_init);

    WRITE_MPEG_REG(AUD_RESAMPLE_CTRL0, 0);
    WRITE_MPEG_REG(AUD_RESAMPLE_CTRL0, (0 << 29) //select the source [30:29]
    | (1 << 28) //enable the resample [28]
            | (0 << 26) //select the method 0 [27:26]
            | (RESAMPLE_CNT_CONTROL << 16) //calculate the cnt_ctrl [25:16]
            | Avg_cnt_init); //calculate the avg_cnt_init [15:0]

    return 0;
}

static int hardware_resample_disable(void) {

    WRITE_MPEG_REG(AUD_RESAMPLE_CTRL0, 0);
    return 0;
}

static const char *hardware_resample_texts[] = { "Disable", "Enable:48K",
        "Enable:44K", "Enable:32K" };

static const struct soc_enum hardware_resample_enum = SOC_ENUM_SINGLE(
        SND_SOC_NOPM, 0, ARRAY_SIZE(hardware_resample_texts),
        hardware_resample_texts);

static int aml_hardware_resample_get_enum(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol) {
    ucontrol->value.enumerated.item[0] = aml_audio_Hardware_resample;
    return 0;
}

static int aml_hardware_resample_set_enum(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol) {
    if (ucontrol->value.enumerated.item[0] == 0) {
        hardware_resample_disable();
        aml_audio_Hardware_resample = 0;
    } else if (ucontrol->value.enumerated.item[0] == 1) {
        hardware_resample_enable(48000);
        aml_audio_Hardware_resample = 1;
    } else if (ucontrol->value.enumerated.item[0] == 2) {
        hardware_resample_enable(44100);
        aml_audio_Hardware_resample = 2;
    } else if (ucontrol->value.enumerated.item[0] == 3) {
        hardware_resample_enable(32000);
        aml_audio_Hardware_resample = 3;
    }
    return 0;
}

static int aml_set_bias_level(struct snd_soc_card *card,
        struct snd_soc_dapm_context *dapm, enum snd_soc_bias_level level) {
    //printk(KERN_INFO "enter %s\n", __func__);
    return 0;
}

static const struct snd_soc_dapm_widget aml_asoc_dapm_widgets[] = {
        SND_SOC_DAPM_INPUT("LINEIN"), SND_SOC_DAPM_OUTPUT("LINEOUT"), };

static const struct snd_kcontrol_new aml_g9tv_controls[] = {

SOC_SINGLE_BOOL_EXT("Audio i2s mute", 0, aml_audio_get_i2s_mute,
        aml_audio_set_i2s_mute),

SOC_SINGLE_BOOL_EXT("Audio spdif mute", 0, aml_audio_get_spdif_mute,
        aml_audio_set_spdif_mute),

SOC_ENUM_EXT("Audio In Source", audio_in_source_enum, aml_audio_get_in_source,
        aml_audio_set_in_source),

SOC_ENUM_EXT("HDMI Audio Type", hdmi_audio_type_enum,
        aml_hdmi_audio_type_get_enum, aml_hdmi_audio_type_set_enum),

SOC_ENUM_EXT("SPDIFIN Audio Type", spdif_audio_type_enum,
        aml_spdif_audio_type_get_enum, aml_spdif_audio_type_set_enum),

SOC_ENUM_EXT("Hardware resample enable", hardware_resample_enum,
        aml_hardware_resample_get_enum, aml_hardware_resample_set_enum),

};

static int aml_asoc_init(struct snd_soc_pcm_runtime *rtd) {
    struct snd_soc_card *card = rtd->card;
    struct snd_soc_codec *codec = rtd->codec;
    struct snd_soc_dapm_context *dapm = &codec->dapm;
    struct aml_audio_private_data * p_aml_audio;
    int ret = 0;

    printk(KERN_DEBUG "enter %s \n", __func__);
    p_aml_audio = snd_soc_card_get_drvdata(card);
    ret = snd_soc_add_card_controls(codec->card, aml_g9tv_controls,
            ARRAY_SIZE(aml_g9tv_controls));
    if (ret)
        return ret;
    /* Add specific widgets */
    snd_soc_dapm_new_controls(dapm, aml_asoc_dapm_widgets,
            ARRAY_SIZE(aml_asoc_dapm_widgets));

    return 0;
}

static struct snd_soc_dai_link aml_codec_dai_link[] = {
    {
        .name = "SND_G9TV",
        .stream_name = "AML PCM",
        .cpu_dai_name = "aml-i2s-dai.0",
        .init = aml_asoc_init,
        .platform_name = "aml-i2s.0",
        .ops = &aml_asoc_ops,
    },
#ifdef CONFIG_SND_SOC_PCM2BT
    {
        .name = "BT Voice",
        .stream_name = "Voice PCM",
        .cpu_dai_name = "aml-pcm-dai.0",
        .codec_dai_name = "pcm2bt-pcm",
        .platform_name = "aml-pcm.0",
        .codec_name = "pcm2bt.0",
    },
#endif
    {
        .name = "AML-SPDIF",
        .stream_name = "SPDIF PCM",
        .cpu_dai_name = "aml-spdif-dai.0",
        .codec_dai_name = "dit-hifi",
        .init = NULL,
        .platform_name = "aml-i2s.0",
        .codec_name = "spdif-dit.0",
        .ops = NULL,
    },
};

struct snd_soc_aux_dev g9tv_audio_aux_dev;
static struct snd_soc_codec_conf g9tv_audio_codec_conf[] = {
    {
        .name_prefix = "AMP",
    },
};

static struct snd_soc_card aml_snd_soc_card = {
    .driver_name = "SOC-Audio",
    .dai_link = &aml_codec_dai_link[0],
    .num_links = ARRAY_SIZE(aml_codec_dai_link),
    .set_bias_level = aml_set_bias_level,
};

static int get_audio_codec_i2c_info(struct device_node* p_node,
        aml_audio_codec_info_t* audio_codec_dev) {
    const char* str;
    int ret = 0;
    unsigned i2c_addr;

    ret = of_property_read_string(p_node, "codec_name", &audio_codec_dev->name);
    if (ret) {
        printk("get audio codec name failed!\n");
        goto err_out;
    }

    ret = of_property_match_string(p_node, "status", "okay");
    if (ret) {
        printk("%s:this audio codec is disabled!\n", audio_codec_dev->name);
        goto err_out;
    }

    if (strcmp(audio_codec_dev->name, "tas5707"))
        return -1;

    printk("use audio aux codec %s\n", audio_codec_dev->name);

    ret = of_property_read_string(p_node, "i2c_bus", &str);
    if (ret) {
        printk("%s: faild to get i2c_bus str,use default i2c bus!\n",
                audio_codec_dev->name);
        audio_codec_dev->i2c_bus_type = AML_I2C_BUS_D;
    } else {
        if (!strncmp(str, "i2c_bus_a", 9))
            audio_codec_dev->i2c_bus_type = AML_I2C_BUS_A;
        else if (!strncmp(str, "i2c_bus_b", 9))
            audio_codec_dev->i2c_bus_type = AML_I2C_BUS_B;
        else if (!strncmp(str, "i2c_bus_c", 9))
            audio_codec_dev->i2c_bus_type = AML_I2C_BUS_C;
        else if (!strncmp(str, "i2c_bus_d", 9))
            audio_codec_dev->i2c_bus_type = AML_I2C_BUS_D;
        else if (!strncmp(str, "i2c_bus_ao", 10))
            audio_codec_dev->i2c_bus_type = AML_I2C_BUS_AO;
        else
            audio_codec_dev->i2c_bus_type = AML_I2C_BUS_D;
    }

    ret = of_property_read_u32(p_node, "i2c_addr", &i2c_addr);
    if (!ret) {
        audio_codec_dev->i2c_addr = i2c_addr;
    }
    printk("audio aux codec addr: 0x%x, audio codec i2c bus: %d\n",
            audio_codec_dev->i2c_addr, audio_codec_dev->i2c_bus_type);

    err_out: return ret;
}

static int of_get_eq_pdata(struct tas57xx_platform_data *pdata,
        struct device_node* p_node) {
    int i, ret = 0, length = 0;
    const char *str = NULL;
    char *regs = NULL;

    prob_priv.num_eq = of_property_count_strings(p_node, "eq_name");
    if (prob_priv.num_eq <= 0) {
        printk("no of eq_name config\n");
        ret = -ENODEV;
        goto exit;
    }

    pdata->num_eq_cfgs = prob_priv.num_eq;

    prob_priv.eq_configs = kzalloc(
            prob_priv.num_eq * sizeof(struct tas57xx_eq_cfg), GFP_KERNEL);

    for (i = 0; i < prob_priv.num_eq; i++) {
        ret = of_property_read_string_index(p_node, "eq_name", i, &str);

        if (of_find_property(p_node, str, &length) == NULL) {
            printk("%s fail to get of eq_table\n", __func__);
            goto exit1;
        }

        regs = kzalloc(length * sizeof(char *), GFP_KERNEL);
        if (!regs) {
            printk("ERROR, NO enough mem for eq_table!\n");
            return -ENOMEM;
        }

        ret = of_property_read_u8_array(p_node, str, regs, length);

        strncpy(prob_priv.eq_configs[i].name, str, NAME_SIZE);
        prob_priv.eq_configs[i].regs = regs;
    }

    pdata->eq_cfgs = prob_priv.eq_configs;

    return 0;
    exit1: kfree(prob_priv.eq_configs);
    exit: return ret;
}

static void *alloc_and_get_data_array(struct device_node *p_node, char *str,
        int *lenp) {
    int ret = 0, length = 0;
    char *p = NULL;

    if (of_find_property(p_node, str, &length) == NULL) {
        printk("DTD of %s not found!\n", str);
        goto exit;
    }
    printk("prop name=%s,length=%d\n", str, length);
    p = kzalloc(length * sizeof(char *), GFP_KERNEL);
    if (p == NULL) {
        printk("ERROR, NO enough mem for %s!\n", str);
        length = 0;
        goto exit;
    }

    ret = of_property_read_u8_array(p_node, str, p, length);
    if (ret) {
        printk("no of property %s!\n", str);
        kfree(p);
        p = NULL;
        goto exit;
    }

    *lenp = length;

    exit: return p;
}

static int of_get_subwoofer_pdata(struct tas57xx_platform_data *pdata,
        struct device_node *p_node) {
    int length = 0;
    char *pd = NULL;

    pd = alloc_and_get_data_array(p_node, "sub_bq_table", &length);

    if (pd == NULL) {
        return -1;
    }

    pdata->custom_sub_bq_table_len = length;
    pdata->custom_sub_bq_table = pd;

    return 0;
}

static int of_get_drc_pdata(struct tas57xx_platform_data *pdata,
        struct device_node* p_node) {
    int length = 0;
    char *pd = NULL;

    //get drc1 table
    pd = alloc_and_get_data_array(p_node, "drc1_table", &length);
    if (pd == NULL) {
        return -2;
    }
    pdata->custom_drc1_table_len = length;
    pdata->custom_drc1_table = pd;

    //get drc1 tko table
    length = 0;
    pd = NULL;

    pd = alloc_and_get_data_array(p_node, "drc1_tko_table", &length);
    if (pd == NULL) {
        return -2;
    }
    pdata->custom_drc1_tko_table_len = length;
    pdata->custom_drc1_tko_table = pd;
    pdata->enable_ch1_drc = 1;

    //get drc2 table
    length = 0;
    pd = NULL;
    pd = alloc_and_get_data_array(p_node, "drc2_table", &length);
    if (pd == NULL) {
        return -1;
    }
    pdata->custom_drc2_table_len = length;
    pdata->custom_drc2_table = pd;

    //get drc2 tko table
    length = 0;
    pd = NULL;
    pd = alloc_and_get_data_array(p_node, "drc2_tko_table", &length);
    if (pd == NULL) {
        return -1;
    }
    pdata->custom_drc2_tko_table_len = length;
    pdata->custom_drc2_tko_table = pd;
    pdata->enable_ch2_drc = 1;

    return 0;
}

static int of_get_init_pdata(struct tas57xx_platform_data *pdata,
        struct device_node* p_node) {
    int length = 0;
    char *pd = NULL;

    pd = alloc_and_get_data_array(p_node, "input_mux_reg_buf", &length);
    if (pd == NULL) {
        printk("%s : can't get input_mux_reg_buf \n", __func__);
        return -1;
    }

    /*Now only support 0x20 input mux init*/
    pdata->num_init_regs = length;
    pdata->init_regs = pd;

    if (of_property_read_u32(p_node, "master_vol", &pdata->custom_master_vol)) {
        printk("%s fail to get master volume\n", __func__);
        return -1;
    }

    return 0;

}

static int of_get_resetpin_pdata(struct tas57xx_platform_data *pdata,
        struct device_node* p_node) {
    int reset_pin = -1;
    const char *str = NULL;
    int ret = -1;

    ret = of_property_read_string(p_node, "reset_pin", &str);
    if (ret < 0) {
        return -1;
    }
    reset_pin = amlogic_gpio_name_map_num(str);
    pdata->reset_pin = reset_pin;
    return 0;

}

static int codec_get_of_pdata(struct tas57xx_platform_data *pdata,
        struct device_node* p_node) {
    int ret = 0;

    ret = of_get_resetpin_pdata(pdata, p_node);
    if (ret) {
        printk("codec reset pin is not found in dtd\n");
    }

    ret = of_get_drc_pdata(pdata, p_node);
    if (ret == -2) {
        printk("no platform codec drc config found\n");
    }

    ret = of_get_eq_pdata(pdata, p_node);
    if (ret) {
        printk("no platform codec EQ config found\n");
    }

    ret = of_get_subwoofer_pdata(pdata, p_node);
    if (ret) {
        printk("no platform codec subwoofer config found\n");
    }

    ret = of_get_init_pdata(pdata, p_node);
    if (ret) {
        printk("no platform codec init config found\n");
    }
    return ret;
}

static void aml_g9tv_pinmux_init(struct snd_soc_card *card) {
    struct aml_audio_private_data *p_aml_audio;
    const char *str = NULL;
    int ret;
    p_aml_audio = snd_soc_card_get_drvdata(card);

    printk(KERN_INFO "Set audio codec pinmux!\n");

//    if (!IS_MESON_MG9TV_CPU_REVA)
        p_aml_audio->pin_ctl = devm_pinctrl_get_select(card->dev,
                "aml_snd_g9tv");

    p_audio = p_aml_audio;

    ret = of_property_read_string(card->dev->of_node, "mute_gpio", &str);
    if (ret < 0) {
        printk("aml_snd_g9tv: faild to get mute_gpio!\n");
    } else {
        p_aml_audio->gpio_mute = amlogic_gpio_name_map_num(str);
        p_aml_audio->mute_inv = of_property_read_bool(card->dev->of_node,
                "mute_inv");
        amlogic_gpio_request_one(p_aml_audio->gpio_mute, GPIOF_OUT_INIT_LOW,
                "mute_spk");
        amlogic_set_value(p_aml_audio->gpio_mute, (!(p_aml_audio->mute_inv)), "mute_spk");
        }
}

static void aml_g9tv_pinmux_deinit(struct snd_soc_card *card) {
    struct aml_audio_private_data *p_aml_audio;

    p_aml_audio = snd_soc_card_get_drvdata(card);
    if (p_aml_audio->pin_ctl)
        devm_pinctrl_put(p_aml_audio->pin_ctl);
}

static int aml_g9tv_audio_probe(struct platform_device *pdev) {
    struct snd_soc_card *card = &aml_snd_soc_card;
    struct aml_audio_private_data *p_aml_audio;
    struct device_node* audio_codec_node = pdev->dev.of_node;
    struct device_node* child;
    struct i2c_board_info board_info;
    struct i2c_adapter *adapter;
    struct i2c_client *client;
    aml_audio_codec_info_t temp_audio_codec;
    struct tas57xx_platform_data *pdata;
    char tmp[NAME_SIZE];
    int ret = 0;

#ifdef CONFIG_USE_OF
    p_aml_audio = devm_kzalloc(&pdev->dev,
            sizeof(struct aml_audio_private_data), GFP_KERNEL);
    if (!p_aml_audio) {
        dev_err(&pdev->dev, "Can't allocate aml_audio_private_data\n");
        ret = -ENOMEM;
        goto err;
    }

    card->dev = &pdev->dev;
    platform_set_drvdata(pdev, card);
    snd_soc_card_set_drvdata(card, p_aml_audio);
    if (!(pdev->dev.of_node)) {
        dev_err(&pdev->dev, "Must be instantiated using device tree\n");
        ret = -EINVAL;
        goto err;
    }

    ret = snd_soc_of_parse_card_name(card, "aml,sound_card");
    if (ret)
    goto err;

    ret = of_property_read_string_index(pdev->dev.of_node, "aml,codec_dai",
            codec_info.codec_index, &aml_codec_dai_link[0].codec_dai_name);
    if (ret)
    goto err;

    printk("g9tv_codec_name = %s\n", codec_info.name_bus);

    aml_codec_dai_link[0].codec_name = codec_info.name_bus;

    snprintf(tmp, NAME_SIZE, "%s-%s", "aml,audio-routing", codec_info.name);

    ret = snd_soc_of_parse_audio_routing(card, tmp);
    if (ret)
    goto err;

    for_each_child_of_node(audio_codec_node, child) {
        memset(&temp_audio_codec, 0, sizeof(aml_audio_codec_info_t));
        if (get_audio_codec_i2c_info(child, &temp_audio_codec ) == 0) {

            memset(&board_info, 0, sizeof(board_info));
            strncpy(board_info.type, temp_audio_codec.name, I2C_NAME_SIZE);
            adapter = i2c_get_adapter(temp_audio_codec.i2c_bus_type);
            board_info.addr = temp_audio_codec.i2c_addr;
            board_info.platform_data = &temp_audio_codec;
            client = i2c_new_device(adapter, &board_info);

            snprintf(tmp, I2C_NAME_SIZE, "%s", temp_audio_codec.name);
            strlcpy(codec_info_aux.name, tmp, I2C_NAME_SIZE);
            snprintf(tmp, I2C_NAME_SIZE, "%s.%s", temp_audio_codec.name, dev_name(&client->dev));
            strlcpy(codec_info_aux.name_bus, tmp, I2C_NAME_SIZE);

            g9tv_audio_aux_dev.name = codec_info_aux.name;
            g9tv_audio_aux_dev.codec_name = codec_info_aux.name_bus;
            g9tv_audio_codec_conf[0].dev_name = codec_info_aux.name_bus;

            card->aux_dev = &g9tv_audio_aux_dev,
            card->num_aux_devs = 1,
            card->codec_conf = g9tv_audio_codec_conf,
            card->num_configs = ARRAY_SIZE(g9tv_audio_codec_conf),

            pdata = kzalloc(sizeof(struct tas57xx_platform_data), GFP_KERNEL);
            if (!pdata) {
                printk("ERROR, NO enough mem for tas57xx_platform_data!\n");
                return -ENOMEM;
            }

            codec_get_of_pdata(pdata, child);
            client->dev.platform_data = pdata;

            //i2c_put_adapter(adapter);
            break;
        }
    }

    ret = snd_soc_register_card(card);
    if (ret) {
        dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n",
                ret);
        goto err;
    }

    aml_g9tv_pinmux_init(card);
    return 0;
#endif

    err: kfree(p_aml_audio);
    return ret;
}

static int aml_g9tv_audio_remove(struct platform_device *pdev) {
    int ret = 0;
    struct snd_soc_card *card = platform_get_drvdata(pdev);
    struct aml_audio_private_data *p_aml_audio;

    p_aml_audio = snd_soc_card_get_drvdata(card);
    snd_soc_unregister_card(card);
    aml_g9tv_pinmux_deinit(card);
    kfree(p_aml_audio);
    return ret;
}

#ifdef CONFIG_USE_OF
static const struct of_device_id amlogic_audio_dt_match[]= {
    {   .compatible = "sound_card, aml_snd_g9tv",},
    {},
};
#else
#define amlogic_audio_dt_match NULL
#endif

static struct platform_driver aml_g9tv_audio_driver = {
    .probe = aml_g9tv_audio_probe,
    .remove = aml_g9tv_audio_remove,
    .driver = {
        .name = DRV_NAME,
        .owner = THIS_MODULE,
        .pm = &snd_soc_pm_ops,
        .of_match_table = amlogic_audio_dt_match,
    },
};

static int __init aml_g9tv_audio_init(void)
{
    return platform_driver_register(&aml_g9tv_audio_driver);
}

static void __exit aml_g9tv_audio_exit(void)
{
    platform_driver_unregister(&aml_g9tv_audio_driver);
}

module_init (aml_g9tv_audio_init);
module_exit (aml_g9tv_audio_exit);

/* Module information */
MODULE_AUTHOR("AMLogic, Inc.");
MODULE_DESCRIPTION("AML_G9TV audio machine Asoc driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);
MODULE_DEVICE_TABLE(of, amlogic_audio_dt_match);

