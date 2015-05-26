#ifndef AML_G9TV_H
#define AML_G9TV_H

struct aml_audio_private_data {
    int gpio_mute;
    bool mute_inv;
    struct pinctrl *pin_ctl;
};

extern void aml_spdif_pinmux_init(struct device *pdev);
extern void aml_spdif_pinmux_deinit(struct device *pdev);

#endif

