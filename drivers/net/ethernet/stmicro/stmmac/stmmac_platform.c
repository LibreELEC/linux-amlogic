/*******************************************************************************
  This contains the functions to handle the platform driver.

  Copyright (C) 2007-2011  STMicroelectronics Ltd

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Author: Giuseppe Cavallaro <peppe.cavallaro@st.com>
*******************************************************************************/

#include <linux/platform_device.h>
#include <linux/io.h>
#include <mach/am_regs.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/of_device.h>
#include "stmmac.h"
#ifdef CONFIG_DWMAC_MESON
#include <mach/mod_gate.h>
#endif
static const struct of_device_id stmmac_dt_ids[] = {
#ifdef CONFIG_DWMAC_MESON
	{ .compatible = "amlogic,meson6-dwmac", /*.data = &meson6_dwmac_data*/},
	{ .compatible = "amlogic,meson8-rmii-dwmac", /*s802 100m mode this chip have no gmac not support 1000m*/},
	{ .compatible = "amlogic,meson8m2-rgmii-dwmac",},// s812 chip 1000m mode
	{ .compatible = "amlogic,meson8m2-rmii-dwmac", .data = &meson6_dwmac_data },// s812 chip 100m mode
	{ .compatible = "amlogic,meson8b-rgmii-dwmac", },// s805 chip 1000m mode
	{ .compatible = "amlogic,meson8b-rmii-dwmac", .data = &meson6_dwmac_data },// s805 chip 100m mode
	{ .compatible = "amlogic,meson6-rmii-dwmac",.data = &meson6_dwmac_data },// defined
	{ .compatible = "amlogic,mesong9tv-rmii-dwmac",},// defined
#endif
	/* SoC specific glue layers should come before generic bindings */
	{ .compatible = "st,spear600-gmac"},
	{ .compatible = "snps,dwmac-3.610"},
	{ .compatible = "snps,dwmac-3.70a"},
	{ .compatible = "snps,dwmac-3.710"},
	{ .compatible = "snps,dwmac"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, stmmac_dt_ids);
#ifdef CONFIG_DWMAC_MESON
static char DEFMAC[] = "\x00\x01\x23\xcd\xee\xaf";
static unsigned int g_mac_addr_setup = 0;
static unsigned char inline chartonum(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'A' && c <= 'F') {
		return (c - 'A') + 10;
	}
	if (c >= 'a' && c <= 'f') {
		return (c - 'a') + 10;
	}
	return 0;

}
static int __init mac_addr_set(char *line)
{
	unsigned char mac[6];
	int i = 0;
	for (i = 0; i < 6 && line[0] != '\0' && line[1] != '\0'; i++) {
		mac[i] = chartonum(line[0]) << 4 | chartonum(line[1]);
		line += 3;
	}
	memcpy(DEFMAC, mac, 6);
	printk("******** uboot setup mac-addr: %x:%x:%x:%x:%x:%x\n",
			DEFMAC[0], DEFMAC[1], DEFMAC[2], DEFMAC[3], DEFMAC[4], DEFMAC[5]);
	g_mac_addr_setup++;

	return 1;
}

__setup("mac=", mac_addr_set);
#endif
#ifdef CONFIG_OF

/* This function validates the number of Multicast filtering bins specified
 * by the configuration through the device tree. The Synopsys GMAC supports
 * 64 bins, 128 bins, or 256 bins. "bins" refer to the division of CRC
 * number space. 64 bins correspond to 6 bits of the CRC, 128 corresponds
 * to 7 bits, and 256 refers to 8 bits of the CRC. Any other setting is
 * invalid and will cause the filtering algorithm to use Multicast
 * promiscuous mode.
 */
static int dwmac1000_validate_mcast_bins(int mcast_bins)
{
	int x = mcast_bins;

	switch (x) {
	case HASH_TABLE_SIZE:
	case 128:
	case 256:
		break;
	default:
		x = 0;
		pr_info("Hash table entries set to unexpected value %d",
			mcast_bins);
		break;
	}
	return x;
}

/* This function validates the number of Unicast address entries supported
 * by a particular Synopsys 10/100/1000 controller. The Synopsys controller
 * supports 1, 32, 64, or 128 Unicast filter entries for it's Unicast filter
 * logic. This function validates a valid, supported configuration is
 * selected, and defaults to 1 Unicast address if an unsupported
 * configuration is selected.
 */
static int dwmac1000_validate_ucast_entries(int ucast_entries)
{
	int x = ucast_entries;

	switch (x) {
	case 1:
	case 32:
	case 64:
	case 128:
		break;
	default:
		x = 1;
		pr_info("Unicast table entries set to unexpected value %d\n",
			ucast_entries);
		break;
	}
	return x;
}
#if defined (CONFIG_AML_NAND_KEY) || defined (CONFIG_SECURITYKEY)
extern int get_aml_key_kernel(const char* key_name, unsigned char* data, int ascii_flag);
extern int extenal_api_key_set_version(char *devvesion);
static char print_buff[1025];
#endif
static int stmmac_probe_config_dt(struct platform_device *pdev,
				  struct plat_stmmacenet_data *plat,
				  const char **mac)
{
	struct device_node *np = pdev->dev.of_node;
	struct stmmac_dma_cfg *dma_cfg;
	const struct of_device_id *device;
#if defined (CONFIG_AML_NAND_KEY) || defined (CONFIG_SECURITYKEY)
	int ret;
	char *addr =NULL;
#endif
	if (!np)
		return -ENODEV;

	device = of_match_device(stmmac_dt_ids, &pdev->dev);
	if (!device)
		return -ENODEV;

	if (device->data) {
		const struct stmmac_of_data *data = device->data;
		plat->has_gmac = data->has_gmac;
		plat->enh_desc = data->enh_desc;
		plat->tx_coe = data->tx_coe;
		plat->rx_coe = data->rx_coe;
		plat->bugged_jumbo = data->bugged_jumbo;
		plat->pmt = data->pmt;
		plat->riwt_off = data->riwt_off;
		plat->fix_mac_speed = data->fix_mac_speed;
		plat->bus_setup = data->bus_setup;
		plat->setup = data->setup;
		plat->free = data->free;
		plat->init = data->init;
		plat->exit = data->exit;
	}

#if defined (CONFIG_AML_NAND_KEY) || defined (CONFIG_SECURITYKEY)
	ret = get_aml_key_kernel("mac", print_buff, 0);
	extenal_api_key_set_version("auto");
	printk("ret = %d\nprint_buff=%s\n", ret, print_buff);
	if (ret >= 0) {
		strcpy(addr, print_buff);
		*mac = addr;
	}
	else
	{
		if(g_mac_addr_setup){
			*mac = DEFMAC;
		}
		else{
			*mac = of_get_mac_address(np);
		}
	}

#else
	
	*mac = of_get_mac_address(np);
#endif
	plat->interface = of_get_phy_mode(np);

	/* Get max speed of operation from device tree */
	if (of_property_read_u32(np, "max-speed", &plat->max_speed))
		plat->max_speed = -1;

	plat->bus_id = of_alias_get_id(np, "ethernet");
	if (plat->bus_id < 0)
		plat->bus_id = 0;

	/* Default to phy auto-detection */
	plat->phy_addr = -1;

	/* "snps,phy-addr" is not a standard property. Mark it as deprecated
	 * and warn of its use. Remove this when phy node support is added.
	 */
	if (of_property_read_u32(np, "snps,phy-addr", &plat->phy_addr) == 0)
		dev_warn(&pdev->dev, "snps,phy-addr property is deprecated\n");

	if (plat->phy_bus_name){
		plat->mdio_bus_data = NULL;
	}
	else
		plat->mdio_bus_data =
			devm_kzalloc(&pdev->dev,
				     sizeof(struct stmmac_mdio_bus_data),
				     GFP_KERNEL);

	plat->force_sf_dma_mode =
		of_property_read_bool(np, "snps,force_sf_dma_mode");

	/* Set the maxmtu to a default of JUMBO_LEN in case the
	 * parameter is not present in the device tree.
	 */

	/* Set default value for multicast hash bins */
	plat->multicast_filter_bins = HASH_TABLE_SIZE;

	/* Set default value for unicast filter entries */
	plat->unicast_filter_entries = 1;

	/*
	 * Currently only the properties needed on SPEAr600
	 * are provided. All other properties should be added
	 * once needed on other platforms.
	 */
#ifdef CONFIG_DWMAC_MESON
#if 0
	if(of_device_is_compatible(np,"amlogic,meson8m2-dwmac")){
	 	aml_write_reg32(P_PERIPHS_PIN_MUX_6,0xffff);
		aml_write_reg32(P_PREG_ETH_REG0,0x7d21);
		aml_set_reg32_mask(P_HHI_MPLL_CNTL6,1<<27);
        	aml_set_reg32_mask(P_HHI_GEN_CLK_CNTL,0xb803);
        	aml_set_reg32_mask(P_HHI_MPLL_CNTL9,(1638<<0)| (0<<14)|(1<<15) | (1<<14) | (5<<16) | (0<<25) | (0<<26) |(0<<30) | (0<<31));
		        /* setup ethernet mode */
     		aml_clr_reg32_mask(P_HHI_MEM_PD_REG0, (1 << 3) | (1<<2));
        /* hardware reset ethernet phy : gpioz14 connect phyreset pin*/
        	aml_clr_reg32_mask(P_PREG_PAD_GPIO2_EN_N, 1 << 28);
       	aml_clr_reg32_mask(P_PREG_PAD_GPIO2_O, 1 << 28);
        	mdelay(10);
        	aml_set_reg32_mask(P_PREG_PAD_GPIO2_O, 1 << 28);
	}
#endif
#endif
	if (of_device_is_compatible(np, "st,spear600-gmac") ||
		of_device_is_compatible(np, "snps,dwmac-3.70a") ||
		of_device_is_compatible(np,"amlogic,meson8b-rgmii-dwmac")||
		of_device_is_compatible(np,"amlogic,meson8m2-rgmii-dwmac")) {
		/* Note that the max-frame-size parameter as defined in the
		 * ePAPR v1.1 spec is defined as max-frame-size, it's
		 * actually used as the IEEE definition of MAC Client
		 * data, or MTU. The ePAPR specification is confusing as
		 * the definition is max-frame-size, but usage examples
		 * are clearly MTUs
		 */
		of_property_read_u32(np, "max-frame-size", &plat->maxmtu);
		of_property_read_u32(np, "snps,multicast-filter-bins",
				     &plat->multicast_filter_bins);
		of_property_read_u32(np, "snps,perfect-filter-entries",
				     &plat->unicast_filter_entries);
		plat->unicast_filter_entries = dwmac1000_validate_ucast_entries(
					       plat->unicast_filter_entries);
		plat->multicast_filter_bins = dwmac1000_validate_mcast_bins(
					      plat->multicast_filter_bins);
		plat->has_gmac = 1;
		plat->pmt = 1;
	}

	if (of_device_is_compatible(np, "snps,dwmac-3.610") ||
		of_device_is_compatible(np,"amlogic,meson6-dwmac")||
		of_device_is_compatible(np, "snps,dwmac-3.710")) {
		plat->enh_desc = 1;
		plat->bugged_jumbo = 1;
		plat->force_sf_dma_mode = 1;
	}

	if (of_find_property(np, "snps,pbl", NULL)) {
		dma_cfg = devm_kzalloc(&pdev->dev, sizeof(*dma_cfg),
				       GFP_KERNEL);
		if (!dma_cfg)
			return -ENOMEM;
		plat->dma_cfg = dma_cfg;
		of_property_read_u32(np, "snps,pbl", &dma_cfg->pbl);
		dma_cfg->fixed_burst =
			of_property_read_bool(np, "snps,fixed-burst");
		dma_cfg->mixed_burst =
			of_property_read_bool(np, "snps,mixed-burst");
	}
	plat->force_thresh_dma_mode = of_property_read_bool(np, "snps,force_thresh_dma_mode");
	if (plat->force_thresh_dma_mode) {
		plat->force_sf_dma_mode = 0;
		pr_warn("force_sf_dma_mode is ignored if force_thresh_dma_mode is set.");
	}

	return 0;
}
#else
static int stmmac_probe_config_dt(struct platform_device *pdev,
				  struct plat_stmmacenet_data *plat,
				  const char **mac)
{
	return -ENOSYS;
}
#endif /* CONFIG_OF */

/**
 * stmmac_pltfr_probe
 * @pdev: platform device pointer
 * Description: platform_device probe function. It allocates
 * the necessary resources and invokes the main to init
 * the net device, register the mdio bus etc.
 */
static int stmmac_pltfr_probe(struct platform_device *pdev)
{
	int ret = 0;
	//addr =phys_to_virt();
	   struct resource *res;
	 struct device *dev = &pdev->dev;
        void __iomem *addr = NULL;
        struct stmmac_priv *priv = NULL;
        struct plat_stmmacenet_data *plat_dat = NULL;
        const char *mac = NULL;
#ifdef CONFIG_DWMAC_MESON
	switch_mod_gate_by_name("ethernet",1);
#endif
        res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        if (!res)
                return -ENODEV;

        addr = devm_ioremap_resource(dev, res);
	//addr =  ( void* )(0xfe0c0000);
	printk("ethernet base addr is %x\n", (unsigned int)addr);
	plat_dat = dev_get_platdata(&pdev->dev);
	if (pdev->dev.of_node) {
		if (!plat_dat)
			plat_dat = devm_kzalloc(&pdev->dev,
					sizeof(struct plat_stmmacenet_data),
					GFP_KERNEL);
		if (!plat_dat) {
			pr_err("%s: ERROR: no memory", __func__);
			return  -ENOMEM;
		}

		ret = stmmac_probe_config_dt(pdev, plat_dat, &mac);
		if (ret) {
			pr_err("%s: main dt probe failed", __func__);
			return ret;
		}
	}

	/* Custom setup (if needed) */
	if (plat_dat->setup) {
		plat_dat->bsp_priv = plat_dat->setup(pdev);
		if (IS_ERR(plat_dat->bsp_priv))
			return PTR_ERR(plat_dat->bsp_priv);
	}

	/* Custom initialisation (if needed)*/
	if (plat_dat->init) {
		ret = plat_dat->init(pdev, plat_dat->bsp_priv);
		if (unlikely(ret))
			return ret;
	}

	priv = stmmac_dvr_probe(&(pdev->dev), plat_dat, addr);
	if (IS_ERR(priv)) {
		pr_err("%s: main driver probe failed", __func__);
		return PTR_ERR(priv);
	}

	/* Get MAC address if available (DT) */
	if (mac)
		memcpy(priv->dev->dev_addr, mac, ETH_ALEN);

	/* Get the MAC information */
	priv->dev->irq = platform_get_irq_byname(pdev, "macirq");
	if (priv->dev->irq < 0) {
		if (priv->dev->irq != -EPROBE_DEFER) {
			netdev_err(priv->dev,
				   "MAC IRQ configuration information not found\n");
		}
		return priv->dev->irq;
	}

	/*
	 * On some platforms e.g. SPEAr the wake up irq differs from the mac irq
	 * The external wake up irq can be passed through the platform code
	 * named as "eth_wake_irq"
	 *
	 * In case the wake up interrupt is not passed from the platform
	 * so the driver will continue to use the mac irq (ndev->irq)
	 */
	priv->wol_irq = platform_get_irq_byname(pdev, "eth_wake_irq");
	if (priv->wol_irq < 0) {
		if (priv->wol_irq == -EPROBE_DEFER)
			return -EPROBE_DEFER;
		priv->wol_irq = priv->dev->irq;
	}

	priv->lpi_irq = platform_get_irq_byname(pdev, "eth_lpi");
	if (priv->lpi_irq == -EPROBE_DEFER)
		return -EPROBE_DEFER;

	platform_set_drvdata(pdev, priv->dev);

	pr_debug("STMMAC platform driver registration completed");

	return 0;
}

/**
 * stmmac_pltfr_remove
 * @pdev: platform device pointer
 * Description: this function calls the main to free the net resources
 * and calls the platforms hook and release the resources (e.g. mem).
 */
static int stmmac_pltfr_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	int ret = stmmac_dvr_remove(ndev);

	if (priv->plat->exit)
		priv->plat->exit(pdev, priv->plat->bsp_priv);

	if (priv->plat->free)
		priv->plat->free(pdev, priv->plat->bsp_priv);

	return ret;
}

#ifdef CONFIG_PM
static int stmmac_pltfr_suspend(struct device *dev)
{
        struct net_device *ndev = dev_get_drvdata(dev);

        return stmmac_suspend(ndev);
}

static int stmmac_pltfr_resume(struct device *dev)
{
        struct net_device *ndev = dev_get_drvdata(dev);

        return stmmac_resume(ndev);
}

int stmmac_pltfr_freeze(struct device *dev)
{
        int ret;
        struct plat_stmmacenet_data *plat_dat = dev_get_platdata(dev);
        struct net_device *ndev = dev_get_drvdata(dev);
        struct platform_device *pdev = to_platform_device(dev);

        ret = stmmac_freeze(ndev);
        if (plat_dat->exit)
		plat_dat->exit(pdev, plat_dat->bsp_priv);

        return ret;
}

int stmmac_pltfr_restore(struct device *dev)
{
        struct plat_stmmacenet_data *plat_dat = dev_get_platdata(dev);
        struct net_device *ndev = dev_get_drvdata(dev);
        struct platform_device *pdev = to_platform_device(dev);

        if (plat_dat->init)
                plat_dat->init(pdev,plat_dat->bsp_priv);

        return stmmac_restore(ndev);
}

static const struct dev_pm_ops stmmac_pltfr_pm_ops = {
        .suspend = stmmac_pltfr_suspend,
        .resume = stmmac_pltfr_resume,
        .freeze = stmmac_pltfr_freeze,
        .thaw = stmmac_pltfr_restore,
        .restore = stmmac_pltfr_restore,
};
#else
static const struct dev_pm_ops stmmac_pltfr_pm_ops;
#endif /* CONFIG_PM */

struct platform_driver stmmac_pltfr_driver = {
	.probe = stmmac_pltfr_probe,
	.remove = stmmac_pltfr_remove,
	.driver = {
		   .name = STMMAC_RESOURCE_NAME,
		   .owner = THIS_MODULE,
		   .pm = &stmmac_pltfr_pm_ops,
		   .of_match_table = of_match_ptr(stmmac_dt_ids),
		   },
};

MODULE_DESCRIPTION("STMMAC 10/100/1000 Ethernet PLATFORM driver");
MODULE_AUTHOR("Giuseppe Cavallaro <peppe.cavallaro@st.com>");
MODULE_LICENSE("GPL");
