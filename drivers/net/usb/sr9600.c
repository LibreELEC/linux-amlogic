/*
 * Davicom DM9601 USB 1.1 10/100Mbps ethernet devices
 *
 * Peter Korsgaard <jacmet@sunsite.dk>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

//#define DEBUG
#include <linux/version.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/stddef.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/usb.h>
#include <linux/crc32.h>
#include <linux/usb/usbnet.h>

#include "sr9600.h"

/* ------------------------------------------------------------------------------------------ */
/* sr9600 mac and phy operations */
/* sr9600 read some registers from MAC */
static int sr_read(struct usbnet *dev, u8 reg, u16 length, void *data)
{
	void *buf;
	int err = -ENOMEM;

	netdev_dbg(dev->net, "sr_read() reg=0x%02x length=%d", reg, length);

	buf = kmalloc(length, GFP_KERNEL);
	if (!buf)
		goto out;

	err = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0), 
				SR_RD_REGS, SR_REQ_RD_REG,
			    0, reg, buf, length, USB_CTRL_SET_TIMEOUT);
	if (err == length)
		memcpy(data, buf, length);
	else if (err >= 0)
		err = -EINVAL;
	kfree(buf);

 out:
	return err;
}

/* sr9600 write some registers to MAC */
static int sr_write(struct usbnet *dev, u8 reg, u16 length, void *data)
{
	void *buf = NULL;
	int err = -ENOMEM;

	netdev_dbg(dev->net, "sr_write() reg=0x%02x, length=%d", reg, length);

	if (data) {
		buf = kmalloc(length, GFP_KERNEL);
		if (!buf)
			goto out;
		memcpy(buf, data, length);
	}

	err = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
			      SR_WR_REGS, SR_REQ_WR_REG,
			      0, reg, buf, length, USB_CTRL_SET_TIMEOUT);
	kfree(buf);
	if (err >= 0 && err < length)
		err = -EINVAL;
 out:
	return err;
}

/* sr9600 read one register from MAC */
static int sr_read_reg(struct usbnet *dev, u8 reg, u8 *value)
{
	return sr_read(dev, reg, 1, value);
}

/* sr9600 write one register to MAC */
static int sr_write_reg(struct usbnet *dev, u8 reg, u8 value)
{
	netdev_dbg(dev->net, "sr_write_reg() reg=0x%02x, value=0x%02x", reg, value);
	return usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
			       SR_WR_REG, SR_REQ_WR_REG,
			       value, reg, NULL, 0, USB_CTRL_SET_TIMEOUT);
}

/* async mode for writing registers or reg blocks */
static void sr_write_async_callback(struct urb *urb)
{
	struct usb_ctrlrequest *req = (struct usb_ctrlrequest *)urb->context;

	if (urb->status < 0)
		printk(KERN_DEBUG "sr_write_async_callback() failed with %d\n", urb->status);

	kfree(req);
	usb_free_urb(urb);
}

static void sr_write_async_helper(struct usbnet *dev, u8 reg, u8 value, u16 length, void *data)
{
	struct usb_ctrlrequest *req;
	struct urb *urb;
	int status;

	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!urb) {
		netdev_err(dev->net, "Error allocating URB in sr_write_async_helper!");
		return;
	}

	req = kmalloc(sizeof(struct usb_ctrlrequest), GFP_ATOMIC);
	if (!req) {
		netdev_err(dev->net, "Failed to allocate memory for control request");
		usb_free_urb(urb);
		return;
	}

	req->bRequestType = SR_REQ_WR_REG;
	req->bRequest = length ? SR_WR_REGS : SR_WR_REG;
	req->wValue = cpu_to_le16(value);
	req->wIndex = cpu_to_le16(reg);
	req->wLength = cpu_to_le16(length);

	usb_fill_control_urb(urb, dev->udev, usb_sndctrlpipe(dev->udev, 0),
			     (void *)req, data, length,
			     sr_write_async_callback, req);

	status = usb_submit_urb(urb, GFP_ATOMIC);
	if (status < 0) {
		netdev_err(dev->net, "Error submitting the control message: status=%d",
		       status);
		kfree(req);
		usb_free_urb(urb);
	}

	return;
}

static void sr_write_async(struct usbnet *dev, u8 reg, u16 length, void *data)
{
	netdev_dbg(dev->net, "sr_write_async() reg=0x%02x length=%d", reg, length);

	sr_write_async_helper(dev, reg, 0, length, data);
}

static void sr_write_reg_async(struct usbnet *dev, u8 reg, u8 value)
{
	netdev_dbg(dev->net, "sr_write_reg_async() reg=0x%02x value=0x%02x", reg, value);

	sr_write_async_helper(dev, reg, value, 0, NULL);
}

/* sr9600 read one word from phy or eeprom  */
static int sr_share_read_word(struct usbnet *dev, int phy, u8 reg, __le16 *value)
{
	int ret, i;

	mutex_lock(&dev->phy_mutex);

	sr_write_reg(dev, EPAR, phy ? (reg | 0x40) : reg);
	sr_write_reg(dev, EPCR, phy ? 0xc : 0x4);

	for (i = 0; i < SR_SHARE_TIMEOUT; i++) {
		u8 tmp;

		udelay(1);
		ret = sr_read_reg(dev, EPCR, &tmp);
		if (ret < 0)
			goto out;

		/* ready */
		if ((tmp & 1) == 0)
			break;
	}

	if (i >= SR_SHARE_TIMEOUT) {
		netdev_err(dev->net, "%s read timed out!", phy ? "phy" : "eeprom");
		ret = -EIO;
		goto out;
	}

	sr_write_reg(dev, EPCR, 0x0);
	ret = sr_read(dev, EPDR, 2, value);

	netdev_dbg(dev->net, "read shared %d 0x%02x returned 0x%04x, %d",
	       phy, reg, *value, ret);

 out:
	mutex_unlock(&dev->phy_mutex);
	return ret;
}

/* write one word to phy or eeprom */
static int sr_share_write_word(struct usbnet *dev, int phy, u8 reg, __le16 value)
{
	int ret, i;

	mutex_lock(&dev->phy_mutex);

	ret = sr_write(dev, EPDR, 2, &value);
	if (ret < 0)
		goto out;

	sr_write_reg(dev, EPAR, phy ? (reg | 0x40) : reg);
	sr_write_reg(dev, EPCR, phy ? 0x1a : 0x12);

	for (i = 0; i < SR_SHARE_TIMEOUT; i++) {
		u8 tmp;

		udelay(1);
		ret = sr_read_reg(dev, EPCR, &tmp);
		if (ret < 0)
			goto out;

		/* ready */
		if ((tmp & 1) == 0)
			break;
	}

	if (i >= SR_SHARE_TIMEOUT) {
		netdev_err(dev->net, "%s write timed out!", phy ? "phy" : "eeprom");
		ret = -EIO;
		goto out;
	}

	sr_write_reg(dev, EPCR, 0x0);

out:
	mutex_unlock(&dev->phy_mutex);
	return ret;
}

static int sr_read_eeprom_word(struct usbnet *dev, u8 offset, void *value)
{
	return sr_share_read_word(dev, 0, offset, value);
}


static int sr9600_get_eeprom_len(struct net_device *dev)
{
	return SR_EEPROM_LEN;
}

/* get sr9600 eeprom information */
static int sr9600_get_eeprom(struct net_device *net, struct ethtool_eeprom *eeprom, u8 * data)
{
	struct usbnet *dev = netdev_priv(net);
	__le16 *ebuf = (__le16 *) data;
	int i;

	/* access is 16bit */
	if ((eeprom->offset % 2) || (eeprom->len % 2))
		return -EINVAL;

	for (i = 0; i < eeprom->len / 2; i++) {
		if (sr_read_eeprom_word(dev, eeprom->offset / 2 + i, &ebuf[i]) < 0)
			return -EINVAL;
	}
	return 0;
}

/* sr9600 mii-phy register read by word */
static int sr9600_mdio_read(struct net_device *netdev, int phy_id, int loc)
{
	struct usbnet *dev = netdev_priv(netdev);

	__le16 res;

	if (phy_id) {
		netdev_dbg(dev->net, "Only internal phy supported");
		return 0;
	}

	sr_share_read_word(dev, 1, loc, &res);

	netdev_dbg(dev->net,
	       "sr9600_mdio_read() phy_id=0x%02x, loc=0x%02x, returns=0x%04x",
	       phy_id, loc, le16_to_cpu(res));

	return le16_to_cpu(res);
}

/* sr9600 mii-phy register write by word */
static void sr9600_mdio_write(struct net_device *netdev, int phy_id, int loc, int val)
{
	struct usbnet *dev = netdev_priv(netdev);
	__le16 res = cpu_to_le16(val);

	if (phy_id) {
		netdev_dbg(dev->net, "Only internal phy supported");
		return;
	}

	netdev_dbg(dev->net,"sr9600_mdio_write() phy_id=0x%02x, loc=0x%02x, val=0x%04x",
	       phy_id, loc, val);

	sr_share_write_word(dev, 1, loc, res);
}

/*-------------------------------------------------------------------------------------------*/

static void sr9600_get_drvinfo(struct net_device *net, struct ethtool_drvinfo *info)
{
	/* Inherit standard device info */
	usbnet_get_drvinfo(net, info);
	info->eedump_len = SR_EEPROM_LEN;
}

static u32 sr9600_get_link(struct net_device *net)
{
	struct usbnet *dev = netdev_priv(net);

	return mii_link_ok(&dev->mii);
}

static int sr9600_ioctl(struct net_device *net, struct ifreq *rq, int cmd)
{
	struct usbnet *dev = netdev_priv(net);

	return generic_mii_ioctl(&dev->mii, if_mii(rq), cmd, NULL);
}

static struct ethtool_ops sr9600_ethtool_ops = {
	.get_drvinfo	= sr9600_get_drvinfo,
	.get_link	= sr9600_get_link,
	.get_msglevel	= usbnet_get_msglevel,
	.set_msglevel	= usbnet_set_msglevel,
	.get_eeprom_len	= sr9600_get_eeprom_len,
	.get_eeprom	= sr9600_get_eeprom,
	.get_settings	= usbnet_get_settings,
	.set_settings	= usbnet_set_settings,
	.nway_reset	= usbnet_nway_reset,
};

static void sr9600_set_multicast(struct net_device *net)
{
	struct usbnet *dev = netdev_priv(net);
	/* We use the 20 byte dev->data for our 8 byte filter buffer
	 * to avoid allocating memory that is tricky to free later */
	u8 *hashes = (u8 *) & dev->data;
	u8 rx_ctl = 0x31;	// enable, disable_long, disable_crc

    int mc_count;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
	mc_count = net->mc_count;
#else
	mc_count = netdev_mc_count (net);
#endif
    
	memset(hashes, 0x00, SR_MCAST_SIZE);
	hashes[SR_MCAST_SIZE - 1] |= 0x80;	/* broadcast address */

	if (net->flags & IFF_PROMISC) {
		rx_ctl |= 0x02;
	} else if (net->flags & IFF_ALLMULTI || mc_count > SR_MCAST_MAX) {
		rx_ctl |= 0x04;
	} else if (mc_count) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
		struct dev_mc_list *mc_list = net->mc_list;
		int i;

		//memset(hashes, 0, AX_MCAST_FILTER_SIZE);

		/* Build the multicast hash filter. */
		for (i = 0; i < net->mc_count; i++) {
			u32 crc_bits =
			    ether_crc(ETH_ALEN,
				      mc_list->dmi_addr) >> 26;
			hashes[crc_bits >> 3] |=
			    1 << (crc_bits & 7);
			mc_list = mc_list->next;
		}
#else
		struct netdev_hw_addr *ha;
		//memset(hashes, 0, AX_MCAST_FILTER_SIZE);
		netdev_for_each_mc_addr (ha, net) {
			u32 crc_bits = ether_crc(ETH_ALEN, ha->addr) >> 26;
			hashes[crc_bits >> 3] |=
				1 << (crc_bits & 7);
		}
#endif
	}

	sr_write_async(dev, MAR, SR_MCAST_SIZE, hashes);
	sr_write_reg_async(dev, RCR, rx_ctl);
}

static int sr9600_set_mac_address(struct net_device *net, void *p)
{
	struct sockaddr *addr = p;
	struct usbnet *dev = netdev_priv(net);

	if(netif_running(net))
		return -EBUSY;

	if(!is_valid_ether_addr(addr->sa_data)){
		dev_err(&net->dev, "not setting invalid mac address %pM\n", addr->sa_data);
		return -EADDRNOTAVAIL;
	}

	memcpy(net->dev_addr, addr->sa_data, ETH_ALEN);
	sr_write_async(dev, PAR, ETH_ALEN, addr->sa_data);

	return 0;
}

static const struct net_device_ops sr9600_netdev_ops = {
	.ndo_open		= usbnet_open,
	.ndo_stop		= usbnet_stop,
	.ndo_start_xmit		= usbnet_start_xmit,
	.ndo_tx_timeout		= usbnet_tx_timeout,
	.ndo_change_mtu		= usbnet_change_mtu,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_do_ioctl		= sr9600_ioctl,
	.ndo_set_mac_address 	= sr9600_set_mac_address,
	.ndo_set_rx_mode = sr9600_set_multicast,
};
static int sr9600_bind(struct usbnet *dev, struct usb_interface *intf)
{
	int ret;

	ret = usbnet_get_endpoints(dev, intf);
	if (ret)
		goto out;

	//dev->net->do_ioctl = sr9600_ioctl;
	//dev->net->set_multicast_list = sr9600_set_multicast;
	dev->net->ethtool_ops = &sr9600_ethtool_ops;
	dev->net->netdev_ops  = &sr9600_netdev_ops;
	dev->net->hard_header_len += SR_TX_OVERHEAD;
	dev->hard_mtu = dev->net->mtu + dev->net->hard_header_len;
	dev->rx_urb_size = 4096; /*dev->net->mtu + ETH_HLEN + SR_RX_OVERHEAD;*/

	dev->mii.dev = dev->net;
	dev->mii.mdio_read = sr9600_mdio_read;
	dev->mii.mdio_write = sr9600_mdio_write;
	dev->mii.phy_id_mask = 0x1f;
	dev->mii.reg_num_mask = 0x1f;

	/* reset the sr9600 */
	sr_write_reg(dev, NCR, 1);
	udelay(20);

	/* read MAC */
	if (sr_read(dev, PAR, ETH_ALEN, dev->net->dev_addr) < 0) {
		printk(KERN_ERR "Error reading MAC address\n");
		ret = -ENODEV;
		goto out;
	}

	/* power up and reset phy */
	sr_write_reg(dev, PRR, 1);
	mdelay(20 );	// at least 10ms, here 20ms for safe
	sr_write_reg(dev, PRR, 0);
	mdelay(2);	// at least 1ms, here 2ms for reading right register

	/* receive broadcast packets */
	sr9600_set_multicast(dev->net);

	sr9600_mdio_write(dev->net, dev->mii.phy_id, MII_BMCR, BMCR_RESET);
	sr9600_mdio_write(dev->net, dev->mii.phy_id, MII_ADVERTISE, ADVERTISE_ALL | ADVERTISE_CSMA | ADVERTISE_PAUSE_CAP);
	mii_nway_restart(&dev->mii);

out:
	return ret;
}

static int sr9600_rx_fixup(struct usbnet *dev, struct sk_buff *skb)
{
	struct net_device *netdev = dev->net;
	u8 status;
	int len;

	/* format:
	   b0: rx status
	   b1: packet length (incl crc) low
	   b2: packet length (incl crc) high
	   b3..n-4: packet data
	   bn-3..bn: ethernet crc
	 */

	if (unlikely(skb->len < SR_RX_OVERHEAD)) {
		dev_err(&dev->udev->dev, "unexpected tiny rx frame\n");
		return 0;
	}

	status = skb->data[0];
	len = (skb->data[1] | (skb->data[2] << 8)) - 4;

	if (unlikely(status & 0xbf)) {
		if (status & 0x01) netdev->stats.rx_fifo_errors++;
		if (status & 0x02) netdev->stats.rx_crc_errors++;
		if (status & 0x04) netdev->stats.rx_frame_errors++;
		if (status & 0x20) netdev->stats.rx_missed_errors++;
		if (status & 0x90) netdev->stats.rx_length_errors++;
		return 0;
	}

	skb_pull(skb, 3);
	skb_trim(skb, len);

	return 1;
}

static struct sk_buff *sr9600_tx_fixup(struct usbnet *dev, struct sk_buff *skb, gfp_t flags)
{
	int len;

	/* format:
	   b0: packet length low
	   b1: packet length high
	   b3..n: packet data
	*/

	len = skb->len;

	if (skb_headroom(skb) < SR_TX_OVERHEAD) {
		struct sk_buff *skb2;

		skb2 = skb_copy_expand(skb, SR_TX_OVERHEAD, 0, flags);
		dev_kfree_skb_any(skb);
		skb = skb2;
		if (!skb)
			return NULL;
	}

	__skb_push(skb, SR_TX_OVERHEAD);

	/* usbnet adds padding if length is a multiple of packet size
	   if so, adjust length value in header */
	if ((skb->len % dev->maxpacket) == 0)
		len++;

	skb->data[0] = len;
	skb->data[1] = len >> 8;

	return skb;
}

static void sr9600_status(struct usbnet *dev, struct urb *urb)
{
	int link;
	u8 *buf;

	/* format:
	   b0: net status
	   b1: tx status 1
	   b2: tx status 2
	   b3: rx status
	   b4: rx overflow
	   b5: rx count
	   b6: tx count
	   b7: gpr
	*/

	if (urb->actual_length < 8)
		return;

	buf = urb->transfer_buffer;

	link = !!(buf[0] & 0x40);
	if (netif_carrier_ok(dev->net) != link) {
		if (link) {
			netif_carrier_on(dev->net);
			usbnet_defer_kevent (dev, EVENT_LINK_RESET);
		}
		else
			netif_carrier_off(dev->net);
		netdev_dbg(dev->net, "Link Status is: %d", link);
	}
}

static int sr9600_link_reset(struct usbnet *dev)
{
	struct ethtool_cmd ecmd;

	mii_check_media(&dev->mii, 1, 1);
	mii_ethtool_gset(&dev->mii, &ecmd);

	netdev_dbg(dev->net, "link_reset() speed: %d duplex: %d",
	       ecmd.speed, ecmd.duplex);

	return 0;
}

static const struct driver_info sr9600_info = {
	.description	= "Supereal SR9600 USB Ethernet",
	.flags		= FLAG_ETHER,
	.bind		= sr9600_bind,
	.rx_fixup	= sr9600_rx_fixup,
	.tx_fixup	= sr9600_tx_fixup,
	.status		= sr9600_status,
	.link_reset	= sr9600_link_reset,
	.reset		= sr9600_link_reset,
};

static const struct usb_device_id products[] = {
	{
	 USB_DEVICE(0x0fe6, 0x8101),	/* Supereal SR9600 */
	 .driver_info = (unsigned long)&sr9600_info,
	 },
	{},			// END
};

MODULE_DEVICE_TABLE(usb, products);

static struct usb_driver sr9600_driver = {
	.name = "sr9600",
	.id_table = products,
	.probe = usbnet_probe,
	.disconnect = usbnet_disconnect,
	.suspend = usbnet_suspend,
	.resume = usbnet_resume,
};

static int __init sr9600_init(void)
{
	return usb_register(&sr9600_driver);
}

static void __exit sr9600_exit(void)
{
	usb_deregister(&sr9600_driver);
}

module_init(sr9600_init);
module_exit(sr9600_exit);

MODULE_AUTHOR("jokeliu <jokeliu@163.com>");
MODULE_DESCRIPTION("Supereal SR9600 and SR8201 USB 1.1 ethernet devices");
MODULE_LICENSE("GPL");
