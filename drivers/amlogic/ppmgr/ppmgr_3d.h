/*
 * ../drivers/amlogic/ppmgr/ppmgr_3d.h
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
*/

#ifndef _PPMGR_TV_H_
#define _PPMGR_TV_H_

extern void ppmgr_vf_put_dec(struct vframe_s *vf);
extern u32 index2canvas(u32 index);
extern struct vfq_s q_ready;
extern struct vfq_s q_free;
extern int get_bypass_mode(void);

#endif

