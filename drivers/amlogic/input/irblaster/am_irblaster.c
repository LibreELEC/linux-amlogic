/*
 * IRBLASTER Driver
 *
 * Copyright (C) 2014 Amlogic Corporation
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * author :   platform-beijing
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <mach/am_regs.h>
#include <linux/slab.h>
#include <linux/major.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <mach/pinmux.h>
#include <mach/gpio.h>
#include "am_irblaster.h"

//#define DEBUG
#ifdef DEBUG
#define irblaster_dbg(fmt, args...) printk(KERN_INFO "ir_blaster: " fmt, ##args)
#else
#define irblaster_dbg(fmt, args...)
#endif

#define DEVICE_NAME "meson_ibr"
#define DEIVE_COUNT 32
static dev_t amirblaster_id;
static struct class *irblaster_class;
static struct device *irblaster_dev;
static struct cdev amirblaster_device;
static struct aml_blaster *irblaster = NULL;
static DEFINE_MUTEX(irblaster_file_mutex);
static void aml_consumerir_transmit(struct aml_blaster * aml_transmit){
	int i,k;
	unsigned int consumerir_freqs = 1000/(irblaster->consumerir_freqs/1000);
	unsigned int high_level_modulation_enable = 1<<12;
	unsigned int high_level_modulation_disable = ~(1<<12);
	if(irblaster->fisrt_pulse_width == fisrt_low){
		high_level_modulation_enable = 1<<12;
		high_level_modulation_disable = ~(1<<12);
	}
	if(irblaster->fisrt_pulse_width == fisrt_high){
		high_level_modulation_disable = 1<<12;
		high_level_modulation_enable = ~(1<<12);
	}
	// set init_high valid and enable the ir_blaster
#if(MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON8)
	if(!IS_MESON_M8_CPU){
        	//code
		aml_write_reg32( P_AO_IR_BLASTER_ADDR0, aml_read_reg32(P_AO_IR_BLASTER_ADDR0)&(~(1<<0)));
		udelay(1);
		aml_write_reg32( P_AO_IR_BLASTER_ADDR2,0x10000);
    	}
#endif
	irblaster_dbg("Enable the ir_blaster, and create a new format !!  consumerir_freqs %d \n",consumerir_freqs);
	aml_write_reg32( P_AO_IR_BLASTER_ADDR0, aml_read_reg32(P_AO_IR_BLASTER_ADDR0)| (2<<12));     //set the modulator_tb = 2'10; 1us
	aml_write_reg32( P_AO_IR_BLASTER_ADDR1, aml_read_reg32(P_AO_IR_BLASTER_ADDR1)| (((consumerir_freqs/2)-1)<<16));    //set mod_high_count = 13;  
	aml_write_reg32( P_AO_IR_BLASTER_ADDR1, aml_read_reg32(P_AO_IR_BLASTER_ADDR1)| (((consumerir_freqs/2)-1)<<0));     //set mod_low_count = 13;
	aml_write_reg32( P_AO_IR_BLASTER_ADDR0, aml_read_reg32(P_AO_IR_BLASTER_ADDR0)| (1<<2) | (1<<0));
	udelay(1);
	aml_write_reg32( P_AO_IR_BLASTER_ADDR0, aml_read_reg32(P_AO_IR_BLASTER_ADDR0)| (1<<2) | (1<<0));
	k = aml_transmit->pattern_len;	
	if(aml_transmit->pattern_len){
		for(i =0; i < k/2; i++){
			aml_write_reg32( P_AO_IR_BLASTER_ADDR2, (0x10000 &  high_level_modulation_disable)    //timeleve = 0;
					| (3<<10)     //[11:10] = 2'b01,then set the timebase 10us. 
					| ((((aml_transmit->pattern[2*i])-1)/consumerir_freqs)<<0)    //[9:0] = 10'd,the timecount = N+1;
				       );
			udelay((aml_transmit->pattern[2*i]));
			aml_write_reg32( P_AO_IR_BLASTER_ADDR2, (0x10000| high_level_modulation_enable)     //timeleve = 1;    //[11:10] = 2'b11,then set the timebase 26.5us. 
					|(1<<10)
					| ((((aml_transmit->pattern[2*i+1]/10)-1))<<0)     //[9:0] = 10'd,the timecount = N+1;
				       );
			udelay((aml_transmit->pattern[2*i+1]));
			//irblaster_dbg("aml_transmit->pattern[%d] : %d    aml_transmit->pattern[%d] : %d  \n",2*i,aml_transmit->pattern[2*i],(2*i)+1,aml_transmit->pattern[2*i+1]/10);
		}                                        
	}
	/*for(j=0;j<72-k;j++){
		aml_write_reg32( P_AO_IR_BLASTER_ADDR2,0x10000);
		}*/
	irblaster_dbg("The all frame finished !!\n");    

}

/**
  * Function to set the irblaster Carrier Frequency, The modulator is typically run between 32khz and 56khz. 
  *
  * @param[in] pointer to irblaster structure.
  * @param[in] carrirer freqs value.
  * \return Reuturns 0 on success else return the error value.
 */

int set_consumerir_freqs(struct aml_blaster *irblaster,int consumerir_freqs){
	return	((irblaster->consumerir_freqs = consumerir_freqs) >= 32000 
	&& (irblaster->consumerir_freqs <= 56000)) ? 0 : -1;
}

/**
  * Function to set the irblaster High or low modulation. 
  *
  * @param[in] pointer to irblaster structure.
  * @param[in] level 0  mod low level 1 mod high.
  * \return Reutrn set value.
 */


int set_consumerir_mod_level(struct aml_blaster *irblaster,int modulation_level){
	irblaster->fisrt_pulse_width = modulation_level;
	return 0;
}


/**
  * Function to get the irblaster cur Carrier Frequency. 
  *
  * @param[in] pointer to irblaster structure.
  * \return Reuturns freqs.
 */

static int  get_consumerir_freqs(struct aml_blaster *irblaster){
	return irblaster->consumerir_freqs;
}


static int aml_irblaster_open(struct inode *inode, struct file *file)
{
	irblaster_dbg("aml_irblaster_open()\n");
	aml_set_reg32_mask(P_AO_RTI_PIN_MUX_REG, (1 << 31));
	return 0;
}

static long aml_irblaster_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{

	int consumerir_freqs = 0;
	int modulation_level = 0; 
	s32 r = 0;
	unsigned long flags;
	static struct aml_blaster consumerir_transmit;
	void __user *argp = (void __user *)args;
	irblaster_dbg("aml_irblaster_ioctl()  0x%4x \n ",cmd);
	switch(cmd)
	{
		case CONSUMERIR_TRANSMIT:
			if (copy_from_user(&consumerir_transmit, argp, sizeof(struct aml_blaster)))
				return -EFAULT;

		/*	for(i=0; i<consumerir_transmit.pattern_len; i++)
				irblaster_dbg("idx[%d]:[%d]\n", i, consumerir_transmit.pattern[i]);*/
			irblaster_dbg("Transmit [%d]\n", consumerir_transmit.pattern_len);
			local_irq_save(flags);
			aml_consumerir_transmit(&consumerir_transmit);
			local_irq_restore(flags);
			break;

		case GET_CARRIER:
			consumerir_freqs = get_consumerir_freqs(irblaster);
			if(copy_to_user(argp, &consumerir_freqs, sizeof(int)))
				return -EFAULT;
			
			break;
		case SET_CARRIER:
			if (copy_from_user(&consumerir_freqs, argp, sizeof(int)))
				return -EFAULT;
			return set_consumerir_freqs(irblaster,consumerir_freqs);

		case SET_MODLEVEL:  //Modulation level 
			if (copy_from_user(&consumerir_freqs, argp, sizeof(int)))
				return -EFAULT;
			return set_consumerir_mod_level(irblaster,modulation_level);

		default:
			r = -ENOIOCTLCMD;
			break;
	}

	return r;
}
static int aml_irblaster_release(struct inode *inode, struct file *file)
{
	irblaster_dbg("aml_ir_irblaster_release()\n");
	file->private_data = NULL;
	aml_clr_reg32_mask(P_AO_RTI_PIN_MUX_REG, (1 << 31));
	return 0;

}
static const struct file_operations aml_irblaster_fops = {
	.owner		= THIS_MODULE,
	.open		= aml_irblaster_open,
	.unlocked_ioctl = aml_irblaster_ioctl,
	.release	= aml_irblaster_release,
};

static int  aml_irblaster_probe(struct platform_device *pdev)
{
	int r;
	printk("irblaster probe\n");
	irblaster = kmalloc(sizeof(struct aml_blaster),GFP_KERNEL);
        if(irblaster == NULL)
		return -1;
	memset(irblaster,0,sizeof(struct aml_blaster));
	
	if (pdev->dev.of_node) {
		printk("aml_irblaster: pdev->dev.of_node == NULL!\n");
		return -1;
	}
	r = alloc_chrdev_region(&amirblaster_id, 0, DEIVE_COUNT, DEVICE_NAME);
	if (r < 0) {
		printk(KERN_ERR "Can't register major for ir irblaster device\n");
		return r;
	}
	cdev_init(&amirblaster_device, &aml_irblaster_fops);
	amirblaster_device.owner = THIS_MODULE;
	cdev_add(&(amirblaster_device), amirblaster_id, DEIVE_COUNT);
	irblaster_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(irblaster_class)) {
		unregister_chrdev_region(amirblaster_id, DEIVE_COUNT);
		printk(KERN_ERR "Can't create class for ir irblaster device\n");
		return -1;
	}


	irblaster_dev = device_create(irblaster_class, NULL, amirblaster_id, NULL, "irblaster%d", 1);

	if (irblaster_dev == NULL) {
		printk("irblaster_dev create error\n");
		class_destroy(irblaster_class);
		return -EEXIST;
	}

	return 0;
}

static int aml_irblaster_remove(struct platform_device *pdev)
{
	irblaster_dbg("remove IRBLASTER \n");

	if(irblaster)
		kfree(irblaster);
	irblaster = NULL;

	cdev_del(&amirblaster_device);

	device_destroy(irblaster_class, amirblaster_id);

	class_destroy(irblaster_class);
	unregister_chrdev_region(amirblaster_id, DEIVE_COUNT);
	return 0;
}
static const struct of_device_id irblaster_dt_match[]={
	{	.compatible 	= "amlogic,am_irblaster",
	},
	{},
};
static struct platform_driver aml_irblaster_driver = {
	.probe		= aml_irblaster_probe,
	.remove		= aml_irblaster_remove,
	.suspend	= NULL,
	.resume		= NULL,
	.driver = {
		.name = "meson_ibr",
		.owner  = THIS_MODULE,
		.of_match_table = irblaster_dt_match,	
	},
};


static struct platform_device* aml_irblaster_device = NULL;

static int __init aml_irblaster_init(void)
{
	irblaster_dbg("IR BLASTER Driver Init\n");
	aml_irblaster_device = platform_device_alloc(DEVICE_NAME,-1);
	if (!aml_irblaster_device) {
		irblaster_dbg("failed to alloc aml_ir_irblaster_device\n");
		return -ENOMEM;
	}
	if(platform_device_add(aml_irblaster_device)){
		platform_device_put(aml_irblaster_device);
		irblaster_dbg("failed to add aml_ir_irblaster_device\n");
		return -ENODEV;
	}
	if (platform_driver_register(&aml_irblaster_driver)) {
		irblaster_dbg("failed to register aml_ir_irblaster_driver module\n");
		platform_device_del(aml_irblaster_device);
		platform_device_put(aml_irblaster_device);
		return -ENODEV;
	}

	return 0;
}

static void __exit aml_irblaster_exit(void)
{
	irblaster_dbg("IR IRBLASTER Driver exit \n");
	platform_driver_unregister(&aml_irblaster_driver);
	platform_device_unregister(aml_irblaster_device);
	aml_irblaster_device = NULL;
}
module_init(aml_irblaster_init);
module_exit(aml_irblaster_exit);

MODULE_AUTHOR("platform-beijing");
MODULE_DESCRIPTION("AML IR BLASTER Driver");
MODULE_LICENSE("GPL");
