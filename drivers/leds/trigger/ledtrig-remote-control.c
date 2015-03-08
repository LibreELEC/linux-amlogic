/*
 * LED Remote Control Activity Trigger
 *
 * Copyright 2015 Memphiz (memphiz@kodi.tv)
 * Copyright 2013 Rene van Dorst
 * Copyright 2006 Openedhand Ltd.
 *
 * Author: Richard Purdie <rpurdie@openedhand.com>
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

/* Delay on in mSec */
 #define delay_on 20

static void ledtrig_rc_timerfunc(unsigned long data);


static DEFINE_TIMER(ledtrig_rc_timer, ledtrig_rc_timerfunc, 0, 0);
static int rc_activity;
static int rc_lastactivity;

void ledtrig_rc_activity(struct led_classdev *led_cdev)
{
	rc_activity++;
	if (!timer_pending(&ledtrig_rc_timer))
		mod_timer(&ledtrig_rc_timer, jiffies + msecs_to_jiffies(delay_on));
}
EXPORT_SYMBOL(ledtrig_rc_activity);

static struct led_trigger ledtrig_rc = {
	.name     = "rc",
	.activate = ledtrig_rc_activity,
};


static void ledtrig_rc_timerfunc(unsigned long data)
{
	if (rc_lastactivity != rc_activity) {
		rc_lastactivity = rc_activity;
		/* INT_MAX will set each LED to its maximum brightness */
		led_trigger_event(&ledtrig_rc, LED_OFF);
		mod_timer(&ledtrig_rc_timer, jiffies + msecs_to_jiffies(delay_on));
	} else {
		led_trigger_event(&ledtrig_rc, INT_MAX);
	}
}

static int __init ledtrig_rc_init(void)
{
	led_trigger_register(&ledtrig_rc);
	return 0;
}

static void __exit ledtrig_rc_exit(void)
{
	led_trigger_unregister(&ledtrig_rc);
}

module_init(ledtrig_rc_init);
module_exit(ledtrig_rc_exit);

MODULE_AUTHOR("Memphiz <memphiz@kodi.tv>");
MODULE_DESCRIPTION("LED Remote Control Activity Trigger");
MODULE_LICENSE("GPL");
