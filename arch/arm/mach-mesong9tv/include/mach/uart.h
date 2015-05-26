/*
 * arch/arm/mach-mesong9tv/include/mach/uart.h
 *
 * Copyright (C) 2014 Amlogic, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MACH_MESONG9TV_UART_H
#define __MACH_MESONG9TV_UART_H

#define UART_AO			0
#define UART_A			1
#define UART_B			2
#define UART_C			3
#define UART_D			4
#define MESON_UART_PORT_NUM	5

#define MESON_UART_NAME		"uart_ao","uart_a","uart_b","uart_c","uart_d"
#define MESON_UART_LINE		UART_AO,UART_A,UART_B,UART_C,UART_D
#define MESON_UART_FIFO		64,128,64,64,64

#define MESON_UART_ADDRS	((void *)P_AO_UART_WFIFO),((void *)P_UART0_WFIFO), \
				((void *)P_UART1_WFIFO),((void *)P_UART2_WFIFO),   \
				((void *)P_UART3_WFIFO)

#define MESON_UART_IRQS		INT_UART_AO, INT_UART_0, INT_UART_1, INT_UART_2    \
				, INT_UART_3

#define MESON_UART_CLK_NAME	"NULL","uart0","uart1","uart2","uart3"

#endif //__MACH_MESONG9TV_UART_H
