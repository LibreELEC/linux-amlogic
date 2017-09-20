/* Keytable for ODROID IR Remote Controller
 *
 * Copyright (c) 2017 Hardkernel co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <media/rc-map.h>
#include <linux/module.h>

static struct rc_map_table odroid[] = {
	{ 0xb2dc, KEY_POWER },
	{ 0xb288, KEY_MUTE },
	{ 0xb282, KEY_HOME },
	{ 0xb2ce, KEY_OK },
	{ 0xb2ca, KEY_UP },
	{ 0xb299, KEY_LEFT },
	{ 0xb2c1, KEY_RIGHT },
	{ 0xb2d2, KEY_DOWN },
	{ 0xb2c5, KEY_MENU },
	{ 0xb29a, KEY_BACK },
	{ 0xb281, KEY_VOLUMEDOWN },
	{ 0xb280, KEY_VOLUMEUP },
};

static struct rc_map_list odroid_map = {
	.map = {
		.scan    = odroid,
		.size    = ARRAY_SIZE(odroid),
		.rc_type = RC_TYPE_NEC,
		.name    = RC_MAP_ODROID,
	}
};

static int __init init_rc_map_odroid(void)
{
	return rc_map_register(&odroid_map);
}

static void __exit exit_rc_map_odroid(void)
{
	rc_map_unregister(&odroid_map);
}

module_init(init_rc_map_odroid)
module_exit(exit_rc_map_odroid)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hardkernel co., Ltd.");
