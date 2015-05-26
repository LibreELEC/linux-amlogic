/*
 * AMLOGIC sysled driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Author:  wind <wind.zhang@amlogic.com>
 *
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <mach/power_gate.h>
#include <linux/of.h>
#include <mach/gpio.h>
#include <plat/io.h>
#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/of_gpio.h>
#include <mach/pinmux.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static struct early_suspend early_suspend;
#endif

#define SYSLED_MODULE_NAME   "aml-sysled"

struct aml_sysled_platform_data {
	const char *name;
	unsigned int  red_led_mode;
	unsigned int  blue_led_mode;
	int red_led_gpio;
	int blue_led_gpio;
};

//#define AML_sysled_DBG

struct aml_sysled {
	struct aml_sysled_platform_data	*pdata;
	struct platform_device			*pdev;	
};

typedef enum led_mode_e {
	LED_RED = 0,
	LED_BLUE,
	LED_OFF,
}led_mode_t;

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct aml_sysled *Myamlsysled = NULL;
#endif
static int sleep = 0;

static int led_control(unsigned int led_mode )
{
	struct aml_sysled *amlsysled = Myamlsysled;	

	switch(led_mode)
	{
		case LED_RED:
			if(amlsysled->pdata[0].red_led_gpio){
				amlogic_gpio_request(amlsysled->pdata[0].red_led_gpio, SYSLED_MODULE_NAME);
				amlogic_gpio_direction_output(amlsysled->pdata[0].red_led_gpio, amlsysled->pdata[0].red_led_mode, SYSLED_MODULE_NAME);
			}
			if(amlsysled->pdata[0].blue_led_gpio){
				amlogic_gpio_request(amlsysled->pdata[0].blue_led_gpio, SYSLED_MODULE_NAME);
				amlogic_gpio_direction_output(amlsysled->pdata[0].blue_led_gpio, !amlsysled->pdata[0].blue_led_mode, SYSLED_MODULE_NAME);
			}
    		break;
		case LED_BLUE:
			if(amlsysled->pdata[0].red_led_gpio){
				amlogic_gpio_request(amlsysled->pdata[0].red_led_gpio, SYSLED_MODULE_NAME);
				amlogic_gpio_direction_output(amlsysled->pdata[0].red_led_gpio, !amlsysled->pdata[0].red_led_mode, SYSLED_MODULE_NAME);
			}
			if(amlsysled->pdata[0].blue_led_gpio){
				amlogic_gpio_request(amlsysled->pdata[0].blue_led_gpio, SYSLED_MODULE_NAME);
				amlogic_gpio_direction_output(amlsysled->pdata[0].blue_led_gpio, amlsysled->pdata[0].blue_led_mode, SYSLED_MODULE_NAME);
			}
  			break;
  			break;
		case LED_OFF:
			if(amlsysled->pdata[0].red_led_gpio){
				amlogic_gpio_request(amlsysled->pdata[0].red_led_gpio, SYSLED_MODULE_NAME);
				amlogic_gpio_direction_output(amlsysled->pdata[0].red_led_gpio, !amlsysled->pdata[0].red_led_mode, SYSLED_MODULE_NAME);
			}
			if(amlsysled->pdata[0].blue_led_gpio){
				amlogic_gpio_request(amlsysled->pdata[0].blue_led_gpio, SYSLED_MODULE_NAME);
				amlogic_gpio_direction_output(amlsysled->pdata[0].blue_led_gpio, !amlsysled->pdata[0].blue_led_mode, SYSLED_MODULE_NAME);
			}
    		break;
  }
  return 0;
}
////////////////////////////////////for sysfs///////////////////////////////////////////////////////////////
static ssize_t sysled_val(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int led_mode=0;

	if (!strcmp(attr->attr.name, "io_val")) {        
		printk(KERN_INFO"----%s\n", buf);
		sscanf(buf, "%d", &led_mode);       
	}
	led_control(led_mode);
	return count;
}
static ssize_t getio_val(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_INFO"read failed\n");
	return 0;
}

static DEVICE_ATTR(io_val, S_IRWXUGO, getio_val, sysled_val);
///////////////////////////////////////////////////////////////////////////////////////////////////////
static const struct of_device_id amlogic_led_match[] =
{
	{
		.compatible = "amlogic-led",
	},
};
///////////////////////////////////////////early suspend////////////////////////////////////////////
#ifdef CONFIG_HAS_EARLYSUSPEND
static void aml_sysled_early_suspend(struct early_suspend *dev)
{
	printk(KERN_INFO "enter aml_sysled_early_suspend\n");
	led_control(LED_RED);
	sleep = 1;
}
static void aml_sysled_late_resume(struct early_suspend *dev)
{
	printk(KERN_INFO "enter aml_sysled_late_resume\n");
	if(sleep == 1){
		printk("dgt, aml_sysled_late_resume blue led\n");
		led_control(LED_BLUE);
	}
	
	sleep = 0;
}
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////
static int aml_sysled_suspend(struct platform_device *pdev,pm_message_t state)
{
	printk(KERN_INFO "enter aml_sysled_suspend\n");
	sleep = 2;
	return 0;
}

static int aml_sysled_resume(struct platform_device *pdev)
{
	printk(KERN_INFO "enter aml_sysled_resume\n");
	if (READ_AOBUS_REG(AO_RTI_STATUS_REG2) == 0x1234abcd) {
		led_control(LED_BLUE);
	}
	sleep = 0;
	return 0;
}

static int aml_sysled_probe(struct platform_device *pdev)
{
	struct aml_sysled_platform_data *pdata;
	struct aml_sysled *amlsysled;
	int retval,ret;
	int ret_dts = -1;
	struct device_node *np = pdev->dev.of_node;
	int value = -1;
	const char *str;

	printk(KERN_INFO "enter aml_sysled_probe\n");
	amlsysled = kzalloc(sizeof(struct aml_sysled), GFP_KERNEL);
	if (!amlsysled)
	{   
		printk(KERN_ERR "kzalloc error\n");
		return -ENOMEM;
	}
	pdata=kzalloc(sizeof(struct aml_sysled_platform_data),GFP_KERNEL);
	if(!pdata){
		goto err;
	}
	memset((void* )pdata,0,sizeof(*pdata));
	amlsysled->pdev = pdev;
    
	//dts init
	if(np == NULL ){
		printk(KERN_ERR "np == null \n");
		goto err;
	}
	printk("start read sedio dts \n");
	
	//read dev_name
	ret_dts=of_property_read_string(pdev->dev.of_node,"dev_name",&pdata->name);
	if (ret_dts){
		dev_err(&pdev->dev, "read %s  error\n","dev_name");
		goto err;
	}
	printk("sysled pdata->name:%s\n",pdata->name);
	
	//read red_led_mode
	value=-1;
	ret_dts=of_property_read_u32(pdev->dev.of_node,"red_led_mode",&value);
	if (ret_dts){
		dev_err(&pdev->dev, "read %s  error\n","red_led_mode");
		goto err;
	}
	pdata->red_led_mode=value;
	printk("sysled pdata->red_led_mode:%d\n",pdata->red_led_mode);
	
	//read blue_led_mode
	value=-1;
	ret_dts=of_property_read_u32(pdev->dev.of_node,"blue_led_mode",&value);
	if (ret_dts){
		dev_err(&pdev->dev, "read %s  error\n","blue_led_mode");
		goto err;
	}
	pdata->blue_led_mode=value;
	printk("sysled pdata->blue_led_mode:%d\n",pdata->blue_led_mode);
	
	//read red_led_gpio
	ret_dts = of_property_read_string(pdev->dev.of_node, "red_led_gpio", &str);
	if(ret_dts)
	{
	    printk("Error: can not get red_led_gpio name------%s %d\n",__func__,__LINE__);
		pdata->red_led_gpio = 0;
	}else{
	    pdata->red_led_gpio = amlogic_gpio_name_map_num(str);
	    printk("red_led_gpio is %d\n",pdata->red_led_gpio);
	}

	//read blue_led_gpio
	ret_dts = of_property_read_string(pdev->dev.of_node, "blue_led_gpio", &str);
	if(ret_dts)
	{
	    printk("Error: can not get blue_led_gpio name------%s %d\n",__func__,__LINE__);
		pdata->blue_led_gpio = 0;
	}else{
	    pdata->blue_led_gpio = amlogic_gpio_name_map_num(str);
	    printk("blue_led_gpio is %d\n",pdata->blue_led_gpio);
		if(pdata->blue_led_gpio == pdata->red_led_gpio){
			pdata->blue_led_gpio = 0;
		}
	}
	//end dts init
	
	if (!pdata) {
		printk(KERN_ERR "missing platform data\n");
		retval = -ENODEV;
		goto err;
	}	
	amlsysled->pdata = pdata;
	platform_set_drvdata(pdev, amlsysled);
	
#ifdef CONFIG_HAS_EARLYSUSPEND
	early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 9;
	early_suspend.suspend = aml_sysled_early_suspend;
	early_suspend.resume = aml_sysled_late_resume;
	register_early_suspend(&early_suspend);
  
	Myamlsysled = amlsysled;
#endif
	ret = device_create_file(&pdev->dev, &dev_attr_io_val);
	if (ret < 0)
		printk(KERN_WARNING "asoc: failed to add io_val sysfs files\n");
	
	return 0;

err:
	kfree(amlsysled);
	kfree(pdata);
	return retval;
}

static int __exit aml_sysled_remove(struct platform_device *pdev)
{
	struct aml_sysled *amlsysled = platform_get_drvdata(pdev);

	printk(KERN_INFO "enter aml_sysled_remove\n");
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&early_suspend);
#endif
	platform_set_drvdata(pdev, NULL);
	kfree(amlsysled);

	return 0;
}

static struct platform_driver aml_sysled_driver = {
	.driver = {
		.name = "aml-sysled",
	.owner = THIS_MODULE,
	.of_match_table = of_match_ptr(amlogic_led_match),
	},
	.probe = aml_sysled_probe,
	.remove = __exit_p(aml_sysled_remove),
	.suspend = aml_sysled_suspend,
	.resume  = aml_sysled_resume,
};

static int __init aml_sysled_init(void)
{
	int ret = -1;
  
	printk(KERN_INFO "enter aml_sysled_init\n");
	ret = platform_driver_register(&aml_sysled_driver);
  
	if (ret != 0) {
		printk(KERN_ERR "failed to register sysled driver, error %d\n", ret);
		return -ENODEV;
	}
	return ret;    
}
module_init(aml_sysled_init);

static void __exit aml_sysled_exit(void)
{
	//printk(KERN_INFO "enter aml_sysled_exit\n");
	platform_driver_unregister(&aml_sysled_driver);
}
module_exit(aml_sysled_exit);

MODULE_DESCRIPTION("Amlogic sysled driver");
MODULE_AUTHOR("dianzhong.huo <dianzhong.huo@amlogic.com>");
MODULE_LICENSE("GPL");
