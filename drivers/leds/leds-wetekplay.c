/*
 * LEDs driver for the "User LED" on WeTek.Play
 *
 * Copyright (C) 2015 Memphiz <memphiz@kodi.tv>
 *
 * Based on leds.rb532
 */

#include <linux/leds.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/amlogic/aml_gpio_consumer.h>

#include <mach/am_regs.h>
#include <plat/regops.h>

#define GPIO_OWNER_WIFILED "WIFILED"
#define GPIO_OWNER_ETHLED "ETHLED"

static void wetekplay_powerled_set(struct led_classdev *cdev,
			  enum led_brightness brightness)
{

	if (brightness) {
		//printk(KERN_INFO "%s() LED BLUE\n", __FUNCTION__);
		aml_set_reg32_bits(SECBUS2_REG_ADDR(0), 1, 0, 1);	// set TEST_n output mode
		aml_set_reg32_bits(AOBUS_REG_ADDR(0x24), 1, 31, 1);  // set TEST_n pin H
	}
	else {
		//printk(KERN_INFO "%s() LED RED\n", __FUNCTION__);
		aml_set_reg32_bits(SECBUS2_REG_ADDR(0), 1, 0, 1);	// set TEST_n output mode
		aml_set_reg32_bits(AOBUS_REG_ADDR(0x24), 0, 31, 1);  // set TEST_n pin L
	}
}

static enum led_brightness wetekplay_powerled_get(struct led_classdev *cdev)
{
	if (aml_get_reg32_bits(AOBUS_REG_ADDR(0x24), 31, 1))
	    return 255;
	else
	    return 0;
}

static void wetekplay_wifiled_set(struct led_classdev *cdev,
			  enum led_brightness brightness)
{
	int pin;
#ifdef CONFIG_ARCH_MESON6
	pin = GPIOC_8;
#else
	pin = GPIOAO_10;
#endif
	if (brightness) {
		//printk(KERN_INFO "%s() LED BLUE\n", __FUNCTION__);
#ifdef CONFIG_ARCH_MESON6			
		amlogic_gpio_direction_output(pin, 1, GPIO_OWNER_WIFILED);
#else
		amlogic_set_pull_up_down(pin, 1, GPIO_OWNER_ETHLED);
#endif		
	}
	else {
		//printk(KERN_INFO "%s() LED OFF\n", __FUNCTION__);
#ifdef CONFIG_ARCH_MESON6			
		amlogic_gpio_direction_output(pin, 0, GPIO_OWNER_WIFILED);
#else
		amlogic_disable_pullup(pin, GPIO_OWNER_ETHLED);
#endif		
	}
}

static enum led_brightness wetekplay_wifiled_get(struct led_classdev *cdev)
{
	int pin;
#ifdef CONFIG_ARCH_MESON6
	pin = GPIOC_8;
#else
	pin = GPIOAO_10;
#endif	
	if (amlogic_get_value(pin, GPIO_OWNER_WIFILED))
	    return 255;
	else
	    return 0;
}

static void wetekplay_ethled_set(struct led_classdev *cdev,
			  enum led_brightness brightness)
{
	int pin;
#ifdef CONFIG_ARCH_MESON6
	pin = GPIOC_14;
#else
	pin = GPIOAO_11;
#endif	
	if (brightness) {
		//printk(KERN_INFO "%s() LED BLUE\n", __FUNCTION__);
#ifdef CONFIG_ARCH_MESON6		
		amlogic_gpio_direction_output(pin, 1, GPIO_OWNER_ETHLED);
#else
		amlogic_set_pull_up_down(pin, 1, GPIO_OWNER_ETHLED);
#endif		
	}
	else {
		//printk(KERN_INFO "%s() LED OFF\n", __FUNCTION__);
#ifdef CONFIG_ARCH_MESON6			
		amlogic_gpio_direction_output(pin, 0, GPIO_OWNER_ETHLED);
#else
		amlogic_disable_pullup(pin, GPIO_OWNER_ETHLED);
#endif		
	}

}

static enum led_brightness wetekplay_ethled_get(struct led_classdev *cdev)
{
	int pin;
#ifdef CONFIG_ARCH_MESON6
	pin = GPIOC_14;
#else
	pin = GPIOAO_11;
#endif
	if (amlogic_get_value(pin, GPIO_OWNER_ETHLED))
	    return 255;
	else
	    return 0;
}



static struct led_classdev wetekplay_powerled = {
	.name = "wetek:blue:powerled",
	.brightness_set = wetekplay_powerled_set,
	.brightness_get = wetekplay_powerled_get,
	.default_trigger = "default-on",
};

static struct led_classdev wetekplay_wifiled = {
	.name = "wetek:blue:wifiled",
	.brightness_set = wetekplay_wifiled_set,
	.brightness_get = wetekplay_wifiled_get,
	.default_trigger = "wifilink",
};

static struct led_classdev wetekplay_ethled = {
	.name = "wetek:blue:ethled",
	.brightness_set = wetekplay_ethled_set,
	.brightness_get = wetekplay_ethled_get,
	.default_trigger = "ethlink",
};

static int wetekplay_led_probe(struct platform_device *pdev)
{
#ifdef CONFIG_ARCH_MESON6
	amlogic_gpio_request(GPIOC_8, GPIO_OWNER_WIFILED);
	amlogic_gpio_request(GPIOC_14, GPIO_OWNER_ETHLED);
#else
	amlogic_gpio_request(GPIOAO_10, GPIO_OWNER_WIFILED);
	amlogic_gpio_direction_input(GPIOAO_10, GPIO_OWNER_WIFILED);
	amlogic_gpio_request(GPIOAO_11, GPIO_OWNER_ETHLED);
	amlogic_gpio_direction_input(GPIOAO_11, GPIO_OWNER_ETHLED);
#endif	
	led_classdev_register(&pdev->dev, &wetekplay_powerled);
	led_classdev_register(&pdev->dev, &wetekplay_wifiled);
	return led_classdev_register(&pdev->dev, &wetekplay_ethled);
}

static int wetekplay_led_remove(struct platform_device *pdev)
{
#ifdef CONFIG_ARCH_MESON6
	amlogic_gpio_free(GPIOC_8, GPIO_OWNER_WIFILED);
	amlogic_gpio_free(GPIOC_14, GPIO_OWNER_ETHLED);
#else
	amlogic_gpio_free(GPIOAO_10, GPIO_OWNER_WIFILED);
	amlogic_gpio_free(GPIOAO_11, GPIO_OWNER_ETHLED);
#endif
	led_classdev_unregister(&wetekplay_powerled);
	led_classdev_unregister(&wetekplay_wifiled);
	led_classdev_unregister(&wetekplay_ethled);
	return 0;
}

#ifdef CONFIG_USE_OF
static const struct of_device_id amlogic_wetekplayled_dt_match[]={
	{	.compatible = "amlogic,wetekplay-led",
	},
	{},
};
#else
#define amlogic_wetekplayled_dt_match, NULL
#endif


static struct platform_driver wetekplay_led_driver = {
	.probe = wetekplay_led_probe,
	.remove = wetekplay_led_remove,
	.driver = {
		.name = "wetekplay-led",
		.of_match_table = amlogic_wetekplayled_dt_match,
	},
};

static int __init
wetekplay_led_init_module(void)
{
    int err;

    printk("wetekplay_led_init_module\n");
    if ((err = platform_driver_register(&wetekplay_led_driver))) {
        return err;
    }

    return err;

}

static void __exit
wetekplay_led_remove_module(void)
{
    platform_driver_unregister(&wetekplay_led_driver);
    printk("wetekplay-led module removed.\n");
}


//module_platform_driver(wetekplay_led_driver);
module_init(wetekplay_led_init_module);
module_exit(wetekplay_led_remove_module);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Power LED support for WeTek.Play");
MODULE_AUTHOR("Memphiz <memphiz@kodi.tv>");
MODULE_ALIAS("platform:wetekplay-led");
