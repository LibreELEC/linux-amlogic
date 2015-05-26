/* linux/drivers/usb/phy/phy-aml-usb.c
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <mach/am_regs.h>
#include "phy-aml-usb.h"

int amlogic_usbphy_reset(void)
{
	static int	init_count = 0;

	if(!init_count)
	{
		init_count++;
		aml_set_reg32_bits(P_RESET1_REGISTER, 1, 2, 1);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(amlogic_usbphy_reset);

