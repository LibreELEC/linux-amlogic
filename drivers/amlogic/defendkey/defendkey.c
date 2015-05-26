#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <linux/of.h>






#define DEFENDKEY_DEVICE_NAME	"defendkey"
#define DEFENDKEY_CLASS_NAME "defendkey"



typedef struct 
{
    struct cdev cdev;
    unsigned int flags;
}defendkey_dev_t;

static defendkey_dev_t *defendkey_devp=NULL;
static dev_t defendkey_devno;
static struct device * defendkey_device= NULL;


static int defendkey_open(struct inode *inode, struct file *file)
{
	defendkey_dev_t *devp;
	devp = container_of(inode->i_cdev, defendkey_dev_t, cdev);
	file->private_data = devp;
	return 0;
}
static int defendkey_release(struct inode *inode, struct file *file)
{
	defendkey_dev_t *devp;
	devp = file->private_data;
	return 0;
}

static loff_t defendkey_llseek(struct file *filp, loff_t off, int whence)
{
	return 0;
}

static long defendkey_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg )
{
	return 0;
}

static ssize_t defendkey_read( struct file *file, char __user *buf, size_t count, loff_t *ppos )
{
	return 0;
}

static ssize_t defendkey_write( struct file *file, const char __user *buf, size_t count, loff_t *ppos )
{
	int ret = -EINVAL;
	char *local_buf;

	local_buf = vmalloc(count);
	if(!local_buf){
		printk(KERN_INFO "memory not enough,%s:%d,count:%d\n",__func__,__LINE__,count);
		return -ENOMEM;
	}

	if (copy_from_user(local_buf, buf, count)){
		ret =  -EFAULT;
		goto exit;
	}
	ret = aml_img_key_check((unsigned char *)local_buf,(int)count);
	//if(ret < 0){
	//	printk("write defendkey error,%s:%d\n",__func__,__LINE__);
	//	goto exit;
	//}
exit:
	vfree(local_buf);
	return ret;
}

static ssize_t version_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	ssize_t n=0;
	n += sprintf(&buf[n], "version:1.00");
	buf[n] = 0;
	return n;
}
static ssize_t secure_check_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	ssize_t n=0;
	int ret;
	ret = aml_is_secure_set();
	if(ret < 0){
		n += sprintf(&buf[n], "fail");
	}
	else if(ret == 0){
		n += sprintf(&buf[n], "raw");
	}
	else if(ret > 0){
		n += sprintf(&buf[n], "encrypt");
	}
	return n;
}
static ssize_t secure_verify_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	ssize_t n=0;
	return n;
}

static const struct file_operations defendkey_fops = {
	.owner      = THIS_MODULE,
	.llseek     = defendkey_llseek,
	.open       = defendkey_open,
	.release    = defendkey_release,
	.read       = defendkey_read,
	.write      = defendkey_write,
	.unlocked_ioctl      = defendkey_unlocked_ioctl,
};

static struct device_attribute defendkey_class_attrs[] ={
	__ATTR_RO(version),
	__ATTR_RO(secure_check), 
	__ATTR_RO(secure_verify), 
	__ATTR_NULL 
};
static struct class defendkey_class ={
	.name = DEFENDKEY_CLASS_NAME,
	.dev_attrs = defendkey_class_attrs, 
};


#ifdef CONFIG_OF
static const struct of_device_id meson6_defendkey_dt_match[];
static char *get_defendkey_drv_data(struct platform_device *pdev)
{
	char *key_dev = NULL;
	if (pdev->dev.of_node) {
		const struct of_device_id *match;
		match = of_match_node(meson6_defendkey_dt_match, pdev->dev.of_node);
		if(match){
			key_dev = (char*)match->data;
		}
	}
	return key_dev;
}
#endif

static int aml_defendkey_probe(struct platform_device *pdev)
{
	int ret=-1;
	struct device *devp;

	ret = alloc_chrdev_region(&defendkey_devno, 0, 1, DEFENDKEY_DEVICE_NAME);
	if(ret<0){
		printk(KERN_ERR "defendkey: failed to allocate major number\n ");
		ret = -ENODEV;
		goto out;
	}
	printk("%s:%d=============defendkey_devno:%x\n",__func__,__LINE__,defendkey_devno);
    ret = class_register(&defendkey_class);
    if (ret){
        goto error1;
	}
	defendkey_devp = kzalloc(sizeof(defendkey_dev_t), GFP_KERNEL);
	if(!defendkey_devp){
		printk(KERN_ERR "defendkey: failed to allocate memory\n ");
		ret = -ENOMEM;
		goto error2;
	}
	/* connect the file operations with cdev */
	cdev_init(&defendkey_devp->cdev, &defendkey_fops);
	defendkey_devp->cdev.owner = THIS_MODULE;
	/* connect the major/minor number to the cdev */
	ret = cdev_add(&defendkey_devp->cdev, defendkey_devno, 1);
	if(ret){
		printk(KERN_ERR "defendkey: failed to add device\n");
		goto error3;
	}
	devp = device_create(&defendkey_class, NULL, defendkey_devno, NULL, DEFENDKEY_DEVICE_NAME);
	if (IS_ERR(devp))
	{
		printk(KERN_ERR "defendkey: failed to create device node\n");
		ret = PTR_ERR(devp);
		goto error4;
	}
#ifdef CONFIG_OF
	devp->platform_data = get_defendkey_drv_data(pdev);
#else
    if (pdev->dev.platform_data)
        devp->platform_data = pdev->dev.platform_data;
    else
        devp->platform_data = NULL;
#endif	
	defendkey_device = devp;
	printk(KERN_INFO "defendkey: device %s created ok\n", DEFENDKEY_DEVICE_NAME);
	return 0;

error4:
	cdev_del(&defendkey_devp->cdev);
error3:
	kfree(defendkey_devp);
error2:
	class_unregister(&defendkey_class);
error1:
	unregister_chrdev_region(defendkey_devno, 1);
out:
	return ret;
}

static int aml_defendkey_remove(struct platform_device *pdev)
{
	//if (pdev->dev.of_node) {
	//	unifykey_dt_release(pdev);
	//}
	unregister_chrdev_region(defendkey_devno, 1);
	device_destroy(&defendkey_class, defendkey_devno);
	//device_destroy(&defend_class, defendkey_devno);
	cdev_del(&defendkey_devp->cdev);
	kfree(defendkey_devp);
	class_unregister(&defendkey_class);
	return 0;
}

#ifdef CONFIG_OF
//static char * secure_device[3]={"nand_key","emmc_key",NULL};

static const struct of_device_id meson6_defendkey_dt_match[]={
	{	.compatible = "amlogic,defendkey",
		.data		= NULL,
		//.data		= (void *)&secure_device[0],
	},
	{},
};
//MODULE_DEVICE_TABLE(of,meson6_rtc_dt_match);
#else
#define meson6_defendkey_dt_match NULL
#endif

static struct platform_driver aml_defendkey_driver =
{
	.probe = aml_defendkey_probe, 
	.remove = aml_defendkey_remove, 
	.driver =
     {
     	.name = DEFENDKEY_DEVICE_NAME, 
        .owner = THIS_MODULE, 
        .of_match_table = meson6_defendkey_dt_match,
     }, 
};

static int __init defendkey_init(void)
{
	int ret = -1;
	ret = platform_driver_register(&aml_defendkey_driver);
	 if (ret != 0)
    {
        printk(KERN_ERR "failed to register efuse bootkey driver, error %d\n", ret);
        return -ENODEV;
    }
    printk(KERN_INFO "platform_driver_register--efuse bootkey  driver--------------------\n");

    return ret;
}
static void __exit defendkey_exit(void)
{
	platform_driver_unregister(&aml_defendkey_driver);
}

module_init(defendkey_init);
module_exit(defendkey_exit);

MODULE_DESCRIPTION("AMLOGIC defendkey driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhou benlong <benlong.zhou@amlogic.com>");



