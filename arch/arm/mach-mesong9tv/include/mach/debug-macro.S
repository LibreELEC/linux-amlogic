/* arch/arm/mach-mesong9tv/include/mach/debug-macro.S
 *
 * Debugging macro include header
 *
 *  Copyright (C) 1994-1999 Russell King
 *  Moved from linux/arch/arm/kernel/debug.S by Ben Dooks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define UART_PADDR                  0xc81004c0

#define G9TV_IO_VAUART	            0xFE6004c0
#define M6TV_IO_VAUART	            0xf11004c0

#define UART_TX_FULL                0x00200000
#define UART_TX_BUSY                0x02000000

#define UART_VADDR	                G9TV_IO_VAUART


#ifndef UART_PADDR
#error "CONFIG_DEBUG_LL is incompatible with multiple archs"
#endif


		.macro	addruart, rp, rv, tmp
		ldr	\rp, =UART_PADDR	    @ physical
		ldr	\rv, =UART_VADDR	    @ virtual
		.endm

		.macro	senduart,rd,rx
		str	\rd, [\rx, #0x0]	    @ TXDATA
		.endm

		.macro	waituart,rd,rx
		.endm

		.macro	busyuart,rd,rx
1002:	ldr	\rd, [\rx, #0xc]	    @ SR2
		tst	\rd, #UART_TX_FULL		@ TXDC
		bne	1002b			        @ wait until transmit done
		.endm
