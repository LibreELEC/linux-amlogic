#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/mach/flash.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>
#include <asm/dma.h>
//#include <asm/io.h>
#include <linux/ioctl.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/system.h>
//#include <mach/platform.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/keychord.h>

#include <mach/gpio.h>
#include <linux/gpio.h>
#include <linux/pm.h>

//#include "keyled_sm1628.h"
#include "venus_vfd.h"

void keyled_gpio_dirset(void)
{
//    hi_gpio_dirset_bit(GROUP, CLK, 0);  //output
//    hi_gpio_dirset_bit(GROUP, DAT, 0);  //output
 //   hi_gpio_dirset_bit(GROUP, KEY, 1);  //input
//    hi_gpio_dirset_bit(GROUP, STB, 0);  //output
 //   hi_gpio_dirset_bit(GROUP, LED, 0);  //output
        gpio_direction_output(LED_CLK,0);
        gpio_direction_output(LED_DAT,0);
	 gpio_direction_output(LED_STB,0);
}

void keyled_sm1628_tx_byte(HI_U8 val) 
{
	HI_U8 i = 0;
	HI_U8 tmp = val;
	//hi_gpio_dirset_bit(GROUP, DAT, 0);
	gpio_direction_output(LED_DAT,0);
	GPIO_DATA_SET(0);
	for (i = 0; i < 8; i++)
	{
		GPIO_CLOCK_SET(0);
		GPIO_DATA_SET(tmp & 0x1);
		tmp = (tmp >> 1);
		GPIO_CLOCK_SET(1);
	}
}

void keyled_sm1628_write(HI_U8 val) 
{    
	/*send single command byte*/ 
	GPIO_STB_SET(0);
	keyled_sm1628_tx_byte(val);
	GPIO_STB_SET(1);
	return;
}

void keyled_sm1628_led_update(void) 
{
	HI_U8 i = 0;
	/*command 1:set display mode */ 
	keyled_sm1628_write(CMD1_DIP_OFF);	
	keyled_sm1628_write(CMD1_DISP_MODE_1);
	/*command 2:set write command  */ 
	keyled_sm1628_write(CMD1_LED_WRITE_INC);
	/*command 3:set the initialization address and send data */
	GPIO_STB_SET(0);
	keyled_sm1628_tx_byte(CMD1_LED_ADDR_BASE);
	for (i = 0; i < SM1628_DISPLAY_REG_NUM; i++)
	{
		keyled_sm1628_tx_byte(v_LedCode[i]);
	}
	GPIO_STB_SET(1);
	//command 4 :open LED display control
	keyled_sm1628_write(CMD1_DIP_ON);	
}

HI_S32 keyled_init(void)
{
	//printk(KERN_INFO "keyled_init() is called!\n");

	//HI_REG_WRITE(IO_ADDRESS(0x10203000 + 0x78), 0);  //5_2
	//HI_REG_WRITE(IO_ADDRESS(0x10203000 + 0x7c), 0);  //5_3
	//HI_REG_WRITE(IO_ADDRESS(0x10203000 + 0x80), 0);  //5_4
	//HI_REG_WRITE(IO_ADDRESS(0x10203000 + 0x84), 0);  //5_5
	//HI_REG_WRITE(IO_ADDRESS(0x10203000 + 0x88), 0);  //5_6
	keyled_gpio_dirset();
	return HI_SUCCESS;
}

void sm1628_led(bool i)
{	//if i=0 , led blue  else led red ;
	//hi_gpio_dirset_bit(GROUP, LED, 0);  //output
	//GPIO_LED_SET(i);
}

void no_display_usr(void)
{
	int i;
	keyled_init();
	for(i=0;i<14;(i+=2)){
		v_LedCode[i] = 0x0;
	}
	keyled_sm1628_led_update();
}

void display_all_zero(void)
{

	//printk(KERN_INFO "display_all_zero() is called!\n");

	int i;
	keyled_init();
	for(i=0;i<12;(i+=2)){
		v_LedCode[i] = 0x1f;
	}
	v_LedCode[12] = 0x0;
	keyled_sm1628_led_update();

	return;
}

HI_S32 display_sm1628_time(ZEKO_TIME keyled_time)
{
	//printk(KERN_INFO "display_sm1628_time() is called!\n");

	int index = 0;
	int i = 0;
	int b = 0;
	//	printk("hour=%d,min=%d,sec=%d\n",keyled_time.hour,keyled_time.minute,keyled_time.second);
	index = keyled_time.hour%10;
	CHANGE_Code[2] = LedDigDisDot_sm1628[index];


	index = keyled_time.hour/10;
	CHANGE_Code[0] = LedDigDisDot_sm1628[index];
	index = keyled_time.hour%10;
	CHANGE_Code[1] = LedDigDisDot_sm1628[index];

	index = keyled_time.minute/10;
	CHANGE_Code[3] = LedDigDisDot_sm1628[index];
	index = keyled_time.minute%10;
	CHANGE_Code[4] = LedDigDisDot_sm1628[index];

	//index = keyled_time.minute/10;
	//CHANGE_Code[0] = LedDigDisDot_sm1628[index];
	//index = keyled_time.minute%10;
	//CHANGE_Code[1] = LedDigDisDot_sm1628[index];

	//index = keyled_time.second/10;
	//CHANGE_Code[3] = LedDigDisDot_sm1628[index];
	//index = keyled_time.second%10;
	//CHANGE_Code[4] = LedDigDisDot_sm1628[index];
	//	printk("%x,%x,%x,%x,%x\n",CHANGE_Code[0],CHANGE_Code[1],CHANGE_Code[2],CHANGE_Code[3],CHANGE_Code[4]);
	for(i = 0 ,b = 0 ;i <7 ;i++,b++){
		v_LedCode[i+b]=0x0;
		for(index = 0 ;index <7 ;index ++){
			v_LedCode[i+b] += ((CHANGE_Code[index] & (0x1<<i))>>i)<< index	;
		}
		//屏蔽第一个数字和：
		v_LedCode[i+b] = v_LedCode[i+b]&0xfb;
        if(i<3){
           v_LedCode[i+b] = v_LedCode[i+b]&0xdf;}
           else{
         v_LedCode[i+b] = v_LedCode[i+b]&0xff;}

	}
	udelay_sm1628(10);
	keyled_sm1628_led_update();
	return HI_SUCCESS;
}


void point_config(bool val){
	if(val){
		//point display
		v_LedCode[4]|=0x20;
		v_LedCode[8]|=0x20;
	}else{
		//no point display
		v_LedCode[4]&=0xdf;
		v_LedCode[8]&=0xdf;
	}
}


ssize_t hi_sm1628_write(struct file *filp, const char __user *buf, size_t size, loff_t *off)
{
	//printk(KERN_ERR "hi_sm1628_write() is called!\n");

	ZEKO_TIME  zeko;
	int ret;
	ret=copy_from_user(&zeko, buf, sizeof(zeko));
	display_sm1628_time(zeko);
	return HI_SUCCESS;
}

long hi_sm_ioctl(struct file *filp, unsigned int  cmd, unsigned long arg)
{

	switch(cmd)
	{
		case SM1628_LIGHT_ON_ZERO :
			display_all_zero();
			break ;

		case SM1628_LIGHT_OFF :
			no_display_usr();
			break ;

		default:
			no_display_usr();
			break;
	}
	return HI_SUCCESS;
}


static const struct file_operations sm1628_fops = {
	.owner			= THIS_MODULE ,
	.unlocked_ioctl 	= hi_sm_ioctl ,
	.write			= hi_sm1628_write ,
};

static struct miscdevice sm1628_misc_device = {
	.minor    = HI_SM1628_MINOR,
	.name     = "pf_Dzeko",
	.fops     = &sm1628_fops,
};


static int sm1628_keypad_probe(struct platform_device *pdev)
{

	//printk(KERN_ERR "sm1628_keypad_probe is called!\n");


	int ret = misc_register(&sm1628_misc_device);
	if (ret) {
		printk(KERN_ERR "unable to register a misc device\n");
		goto fail__;
		return ret;
	}
	keyled_init();
	sm1628_led(1);
	//printk(KERN_INFO "initialized OK\n");
	display_all_zero();
	//by zhangh
	//ZEKO_TIME keyled_time1;
//	keyled_time1.hour =2;
//	keyled_time1.minute =30;
//	keyled_time1.second =22;
//	display_sm1628_time(keyled_time1);
	return 0;

fail__:
	misc_deregister(&sm1628_misc_device);
	return -EINVAL;
}


static int sm1628_keypad_remove(struct platform_device *dev)
{
	misc_deregister(&sm1628_misc_device);
	return 0;
}

static int sm1628_keypad_suspend(struct platform_device *dev,pm_message_t state)
{
	keyled_init();
	no_display_usr();
	sm1628_led(0);
	return 0;
}

static int sm1628_keypad_resume(struct platform_device * dev)
{
	keyled_init();
	display_all_zero();
	sm1628_led(1);
	return 0;
}

static void sm1628_platform_device_release(struct device* dev){

}

static struct platform_driver sm1628_keypad_driver = {
	.probe   = sm1628_keypad_probe,
	.remove  = sm1628_keypad_remove,
	.suspend = sm1628_keypad_suspend,
	.resume  = sm1628_keypad_resume,
	.driver  = {
		.name  = "mv300_Dzeko_keypad",
		.owner = THIS_MODULE,
	},
};


static struct platform_device sm1628_keypad_dev = {
	.name = "mv300_Dzeko_keypad",
	.id   = -1,
	.dev = {
		.platform_data  = NULL,
		.dma_mask = (u64*)~0,
		.coherent_dma_mask = (u64)~0,
		.release = sm1628_platform_device_release,
	},
};

static int __init gpio_init(void)
{
	int retval;

	retval = platform_device_register(&sm1628_keypad_dev);
	if(retval) {
		return retval;
	}

	retval = platform_driver_register(&sm1628_keypad_driver);
	if(retval) {
		goto fail2;
	}

	return retval;

fail2:
	platform_device_unregister(&sm1628_keypad_dev);
	return -1;
}

static void __exit gpio_exit (void)
{
	platform_driver_unregister(&sm1628_keypad_driver);
	
	platform_device_unregister(&sm1628_keypad_dev);
}

module_init(gpio_init);
module_exit(gpio_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("wkf29248 <wangjian97@huawei.com>");
MODULE_DESCRIPTION("MV300 sm1628 keyboad driver");


