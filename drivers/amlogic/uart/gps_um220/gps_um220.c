#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/platform_device.h>

struct um220_gps_data {
    void (*gps_power_enable)(void);
    void (*gps_power_disable)(void);

    int power_flag;
    int gpio_power;
    struct cdev gps_cdev;
};

static dev_t gps_devno;
struct um220_gps_data *gps;

#define POWER_OFF    0
#define POWER_ON      1

#define GPS_POWER_OFF   0
#define GPS_POWER_ON     1

#define CLASS_NAME      "gps"
#define DEVICE_NAME    "gps"
#define MOD_NAME "gps_common"

//um220 gps power enable/disable
static void um220_gps_power_disable(void)
{
    int ret = -1;
    if(gps->gpio_power > 0) {
        //ret = amlogic_gpio_direction_output(gps->gpio_power, 0, MOD_NAME);
        ret = amlogic_gpio_direction_output(gps->gpio_power, 1, MOD_NAME);
        printk("%s:return status:%d\n", __func__, ret);
    }
}


static void um220_gps_power_enable(void)
{
    int ret = -1;
    if(gps->gpio_power > 0) {
        //ret = amlogic_gpio_direction_output(gps->gpio_power, 1, MOD_NAME);
        ret = amlogic_gpio_direction_output(gps->gpio_power, 0, MOD_NAME);
        printk("%s:return status:%d\n", __func__, ret);
    }
}

static int um220_gps_open(struct inode *node, struct file *filp)
{
    struct um220_gps_data *g = container_of(node->i_cdev, struct um220_gps_data, gps_cdev);
    filp->private_data = g;

    printk("%s\n",__func__);
    return 0;
}

static int um220_gps_release(struct inode *node, struct file *filp)
{
    printk("%s\n",__func__);
    return 0;
}

static ssize_t um220_gps_read(struct file *filp, char __user *buf, size_t count, loff_t *offset)
{
    printk("%s\n", __func__);
    struct um220_gps_data *g = (struct um220_gps_data *)filp->private_data;
    /* do something */
    return count;
}

static ssize_t um220_gps_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{
    printk("%s\n", __func__);
    struct um220_gps_data *g = (struct um220_gps_data *)filp->private_data;
    /* do something */
    return count;
}

static long um220_gps_ioctl(struct file *filp, unsigned int cmd, unsigned long data)
{
    int ret = 0;
    struct um220_gps_data *g = (struct um220_gps_data *)filp->private_data;
    printk("%s\n", __func__);
    
    switch(cmd) {  
        case GPS_POWER_OFF:
                g->gps_power_disable();
                g->power_flag = POWER_OFF;
                break;

        case GPS_POWER_ON:
                g->gps_power_enable();  
                g->power_flag = POWER_ON;
                break;

        default:
                printk("%s:unknown ioctl cmd!\n", __func__);
                ret = -EINVAL;
                break;
    }

    return ret;
}

static struct file_operations gps_fops = {
    .owner      =  THIS_MODULE,
   	.open       =  um220_gps_open,
	.read       =  um220_gps_read,
	.write      =  um220_gps_write,
    .release    =  um220_gps_release,
    .unlocked_ioctl = um220_gps_ioctl,
};



static ssize_t get_um220_gps_powerEnableState(struct class *class, struct class_attribute *attr,char *buf)
{
    printk("gps power enable state is %s\n", gps->power_flag?"ON":"OFF");
    return sprintf(buf, "%d\n", gps->power_flag);
}

static ssize_t set_um220_gps_powerEnableState(struct class *class, struct class_attribute *attr,const char *buf, size_t count)
{
    int old_powerState = gps->power_flag;

    if(!strlen(buf)) {
         printk("gps power enable state parameter is required!\n");
         return -1;
    }

    if(!strncmp(buf, "0", 1) || !strncmp(buf, "off", 3) || !strncmp(buf, "1", 1) || !strncmp(buf, "on", 2)) {
        if(!strncmp(buf, "0", 1) || !strncmp(buf, "off", 3)) {
            if(old_powerState == POWER_OFF) {
                printk("gps power enable state is already %s now\n", gps->power_flag?"ON":"OFF");
                goto out;
            }
            gps->gps_power_disable(); 
            gps->power_flag = POWER_OFF;
            printk("gps power enable state is set to %s\n", gps->power_flag?"ON":"OFF");
        } 
        else {
            if(old_powerState == POWER_ON) {
                printk("gps power enable state is already %s now\n", gps->power_flag?"ON":"OFF");
                goto out;
            }
            gps->gps_power_enable();
            gps->power_flag = POWER_ON;
            printk("gps power enable state is set to %s\n", gps->power_flag?"ON":"OFF");
        }            
    }
    else {
        printk("gps power enable state parameter is not correct. your choice: 0 or off and 1 or on\n");
        return -1;
    }

out:
    return count;
}

static struct class_attribute gps_class_attrs[] = {   
    __ATTR(gps_power, S_IRUGO|S_IWUGO, get_um220_gps_powerEnableState, set_um220_gps_powerEnableState),
    __ATTR_NULL
};

static struct class gps_class = {
    .name = CLASS_NAME,
    .class_attrs = gps_class_attrs,
};


static int get_um220_gps_dt_data(struct device_node *node,  int *gpio)
{
    int ret = -1; 
    const char *status, *str, *gpio_str;

    ret = of_property_read_string(node, "status", &status);
    if(ret< 0){
        printk("%s: failed to read status from device tree for dev\n", __func__);
        return -1;
    }

    if(!strncmp("ok", status, 2)) {
        ret = of_property_read_string(node, "gpio_power", &gpio_str);
        if(ret < 0){
            printk("%s: failed to read gpio_power from device tree for dev\n", __func__);
            *gpio = -1;
            return -1;
        }
        else {
            *gpio = amlogic_gpio_name_map_num(gpio_str);
            if(*gpio <= 0) {
                *gpio = 0;
            }
        }

        ret = 0;
    }
    else {
        ret = -1;
    }

    return ret;
}

static int um220_gps_probe(struct platform_device *pdev)
{
    int ret = -1, gpio = -1;
    struct device *devp;
    struct device_node* child;
    struct device_node* gps_node = pdev->dev.of_node;
    
    printk("%s\n", __func__);

	ret = get_um220_gps_dt_data(gps_node, &gpio);
    if(ret) {
        printk("%s:failed to get um220 gps dt data\n", __func__);
        return -1;
    }

	if(gpio > 0) {
		amlogic_gpio_request(gpio, MOD_NAME);
	}

    gps = kmalloc(sizeof(struct um220_gps_data), GFP_KERNEL);
    if(gps == NULL) {
        printk("%s:No memory for gps driver\n", __func__);
        ret = -ENOMEM;
        goto out;
    }

    ret = alloc_chrdev_region(&gps_devno, 0, 1, DEVICE_NAME);
    if(ret < 0) {
        printk(KERN_ERR "%s:failed to allocate gps major number\n", __func__);
        ret = -ENODEV;
        goto error1;
    }    

    cdev_init(&gps->gps_cdev, &gps_fops);
    ret = cdev_add(&gps->gps_cdev, gps_devno, 1);
    if(ret) {
        printk(KERN_ERR "%s:failed to add gps device\n", __func__);
        goto error2;
    }
    printk("%s:major=%d, minor=%d\n", __func__, MAJOR(gps_devno), MINOR(gps_devno));

    ret = class_register(&gps_class);
    if(ret) {
        printk(KERN_ERR "%s:error creating gps class\n", __func__);
        goto error3;
    }

    devp = device_create(&gps_class, NULL, gps_devno, NULL, DEVICE_NAME);
    if(IS_ERR(devp)) {
        printk(KERN_ERR "%s:failed to create gps device node\n", __func__);
        ret = PTR_ERR(devp);
        goto error4;
    }

    gps->gps_power_enable = um220_gps_power_enable;
    gps->gps_power_disable= um220_gps_power_disable;
    gps->gpio_power = gpio;
    gps->power_flag = POWER_OFF;

    return 0;

error4:  
    class_unregister(&gps_class);
error3:    
    cdev_del(&gps->gps_cdev);
error2:
    unregister_chrdev_region(gps_devno, 1);
error1:
    kfree(gps);

out:
    return ret;

}


static int um220_gps_remove(struct platform_device *pdev)
{
    printk("%s\n", __func__);

    device_destroy(&gps_class, gps_devno);
    class_unregister(&gps_class);
    cdev_del(&gps->gps_cdev);
    unregister_chrdev_region(gps_devno, 1);
    kfree(gps);

    return 0;
}

static const struct of_device_id gps_probe_dt_match[]={
    {
        .compatible = "amlogic,gps_um220",
    },
    {},
};

static struct platform_driver um220_gps_driver = {
    .probe	= um220_gps_probe,
    .remove  = um220_gps_remove,
    .driver	= {
        .name	= "gps_um220",
        .owner	= THIS_MODULE,
        .of_match_table = gps_probe_dt_match,
    },
};

static int __init um220_gps_init(void)
{ 
    int ret = -1;
    printk("%s\n", __func__);    
    ret = platform_driver_register(&um220_gps_driver);
    if (ret != 0) {
        printk(KERN_ERR "%s:failed to register gps driver, error:%d\n", __func__, ret);
        return -ENODEV;
    }

    return ret;    
}

static void __exit um220_gps_exit(void)
{
    printk("%s\n", __func__);
    platform_driver_unregister(&um220_gps_driver);
}

module_init(um220_gps_init);
module_exit(um220_gps_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION ("amlogic um220 gps driver");
