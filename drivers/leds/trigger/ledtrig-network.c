/*
 * LED Network Trigger
 *
 * Copyright 2015 Memphiz (memphiz@kodi.tv)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/leds.h>

void ledtrig_eth_linkup(struct led_classdev *led_cdev);
void ledtrig_eth_linkdown(struct led_classdev *led_cdev);
void ledtrig_wifi_linkup(struct led_classdev *led_cdev);
void ledtrig_wifi_linkdown(struct led_classdev *led_cdev);

static struct led_trigger ledtrig_eth = {
	.name     = "ethlink",
	.activate = ledtrig_eth_linkup,
	.deactivate = ledtrig_eth_linkdown,
};

static struct led_trigger ledtrig_wifi = {
	.name     = "wifilink",
	.activate = ledtrig_wifi_linkup,
	.deactivate = ledtrig_wifi_linkdown,
};

void ledtrig_eth_linkup(struct led_classdev *led_cdev)
{
  led_trigger_event(&ledtrig_eth, INT_MAX);
}
EXPORT_SYMBOL(ledtrig_eth_linkup);

void ledtrig_eth_linkdown(struct led_classdev *led_cdev)
{
  led_trigger_event(&ledtrig_eth, LED_OFF);
}
EXPORT_SYMBOL(ledtrig_eth_linkdown);

void ledtrig_wifi_linkup(struct led_classdev *led_cdev)
{
  led_trigger_event(&ledtrig_wifi, INT_MAX);
}
EXPORT_SYMBOL(ledtrig_wifi_linkup);

void ledtrig_wifi_linkdown(struct led_classdev *led_cdev)
{
  led_trigger_event(&ledtrig_wifi, LED_OFF);
}
EXPORT_SYMBOL(ledtrig_wifi_linkdown);

static int __init ledtrig_network_init(void)
{
	led_trigger_register(&ledtrig_eth);
	led_trigger_register(&ledtrig_wifi);
	return 0;
}

static void __exit ledtrig_network_exit(void)
{
	led_trigger_unregister(&ledtrig_eth);
	led_trigger_unregister(&ledtrig_wifi);
}

module_init(ledtrig_network_init);
module_exit(ledtrig_network_exit);

MODULE_AUTHOR("Memphiz <memphiz@kodi.tv>");
MODULE_DESCRIPTION("LED Network link trigger");
MODULE_LICENSE("GPL");
