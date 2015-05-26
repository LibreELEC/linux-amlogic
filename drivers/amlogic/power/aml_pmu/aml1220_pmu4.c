#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <mach/am_regs.h>
#include <mach/gpio.h>
#include <linux/amlogic/aml_pmu.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>




#define AML1220_ADDR    0x35
#define AML1220_ANALOG_ADDR  0x20

struct i2c_client *g_aml1220_client = NULL;


#define CHECK_DRIVER()      \
    if (!g_aml1220_client) {        \
        printk("driver is not ready right now, wait...\n");   \
        dump_stack();       \
        return -ENODEV;     \
    }

unsigned int pmu4_analog_reg[15] = {0x00, 0x00, 0x00, 0x00, 0x00, //Reg   0x20 - 0x24
	                                0x00, 0x00, 0x00, 0x00, 0x51, // Reg  0x25 - 0x29
                                    0x42, 0x00, 0x42, 0x41, 0x02  //Reg   0x2a - 0x2e             
                                   };

#define PMU4_ANALOG_REG_LEN ARRAY_SIZE(pmu4_analog_reg)



int aml1220_write(int32_t add, uint8_t val)
{
    int ret;
    uint8_t buf[3] = {};
    struct i2c_client *pdev;
    struct i2c_msg msg[] = {
        {
            .addr  = AML1220_ADDR,
            .flags = 0,
            .len   = sizeof(buf),
            .buf   = buf,
        }
    };

    CHECK_DRIVER();
    pdev = g_aml1220_client;

    buf[0] = add & 0xff;
    buf[1] = (add >> 8) & 0x0f;
    buf[2] = val & 0xff;
    ret = i2c_transfer(pdev->adapter, msg, 1);
    if (ret < 0) {
        printk("%s: i2c transfer failed, ret:%d\n", __FUNCTION__, ret);
        return ret;
    }
    return 0;
}
EXPORT_SYMBOL_GPL(aml1220_write);

int aml1220_write16(int32_t add, uint16_t val)
{
    int ret;
    uint8_t buf[4] = {};
    struct i2c_client *pdev;
    struct i2c_msg msg[] = {
        {
            .addr  = AML1220_ADDR,
            .flags = 0,
            .len   = sizeof(buf),
            .buf   = buf,
        }
    };

    CHECK_DRIVER();
    pdev = g_aml1220_client;

    buf[0] = add & 0xff;
    buf[1] = (add >> 8) & 0x0f;
    buf[2] = val & 0xff;
    buf[3] = (val >> 8) & 0xff;
    ret = i2c_transfer(pdev->adapter, msg, 1);
    if (ret < 0) {
        printk("%s: i2c transfer failed, ret:%d\n", __FUNCTION__, ret);
        return ret;
    }
    return 0;
}
EXPORT_SYMBOL_GPL(aml1220_write16);

int aml1220_read(int add, uint8_t *val)
{
    int ret;
    uint8_t buf[2] = {};
    struct i2c_client *pdev;
    struct i2c_msg msg[] = {
        {
            .addr  = AML1220_ADDR,
            .flags = 0,
            .len   = sizeof(buf),
            .buf   = buf,
        },
        {
            .addr  = AML1220_ADDR,
            .flags = I2C_M_RD,
            .len   = 1,
            .buf   = val,
        }
    };

    CHECK_DRIVER();
    pdev = g_aml1220_client;

    buf[0] = add & 0xff;
    buf[1] = (add >> 8) & 0x0f;
    ret = i2c_transfer(pdev->adapter, msg, 2);
    if (ret < 0) {
        printk("%s: i2c transfer failed, ret:%d\n", __FUNCTION__, ret);
        return ret;
    }
    return 0;
}
EXPORT_SYMBOL_GPL(aml1220_read);

int aml1220_read16(int add, uint16_t *val)
{
    int ret;
    uint8_t buf[2] = {};
    struct i2c_client *pdev;
    struct i2c_msg msg[] = {
        {
            .addr  = AML1220_ADDR,
            .flags = 0,
            .len   = sizeof(buf),
            .buf   = buf,
        },
        {
            .addr  = AML1220_ADDR,
            .flags = I2C_M_RD,
            .len   = 2, 
            .buf   = (uint8_t *)val,
        }
    };

    CHECK_DRIVER();
    pdev = g_aml1220_client; 

    buf[0] = add & 0xff;
    buf[1] = (add >> 8) & 0x0f;
    ret = i2c_transfer(pdev->adapter, msg, 2);
    if (ret < 0) {
        printk("%s: i2c transfer failed, ret:%d\n", __FUNCTION__, ret);
        return ret;
    }
    return 0;
}

EXPORT_SYMBOL_GPL(aml1220_read16);

#ifdef CONFIG_DEBUG_FS

static struct dentry *debugfs_root;
static struct dentry *debugfs_regs;


static ssize_t aml1220_pmu4_reg_read_file(struct file *file, char __user *user_buf,
			       size_t count, loff_t *ppos)
{
	ssize_t ret;
	char *buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ret = sprintf(buf, "Usage: \n"
										 "	echo reg val >pmu4_reg\t(set the register)\n"
										 "	echo reg >pmu4_reg\t(show the register)\n"
										 "	echo a >pmu4_reg\t(show all register)\n"
									);

	if (ret >= 0)
		ret = simple_read_from_buffer(user_buf, count, ppos, buf, ret);
	kfree(buf);

	return ret;
}

static int read_regs(int reg)
{
	uint8_t val = 0;
	aml1220_read(reg,&val);
	printk("\tReg %x = %x\n", reg, val);
	return val;
}

static void write_regs(int reg, int val)
{
	aml1220_write(reg,val);
	printk("Write reg:%x = %x\n", reg, val);
}

static void dump_all_register(void)
{
	int i = 0;
	uint8_t val = 0;

	printk(" dump aml1220 pmu4 all register:\n");

	for(i = 0; i< 0xb0; i++){
		aml1220_read(i,&val);
		msleep(5);
		printk("0x%02x: 0x%02x \n",i, val);
	}

	return;
}
static ssize_t aml1220_pmu4_reg_write_file(struct file *file,
		const char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[32];
	int buf_size = 0;
	char *start = buf;
	unsigned long reg, value;
	char all_reg;

	buf_size = min(count, (sizeof(buf)-1));

	if (copy_from_user(buf, user_buf, buf_size))
		return -EFAULT;
	buf[buf_size] = 0;
	
	while (*start == ' ')
		start++;

	all_reg = *start;
	start ++;
	if(all_reg =='a'){

		dump_all_register();
		return buf_size;
	}else{
		start --;
	}

	while (*start == ' ')
		start++;

	reg = simple_strtoul(start, &start, 16);

	while (*start == ' ')
		start++;

	if (strict_strtoul(start, 16, &value))
	{
			read_regs(reg);
			return -EINVAL;
	}

	write_regs(reg, value);

	return buf_size;
}


static const struct file_operations aml1220_pmu4_reg_fops = {
	.open = simple_open,
	.read = aml1220_pmu4_reg_read_file,
	.write = aml1220_pmu4_reg_write_file,
};
#endif


static void aml_pmu4_reg_init(unsigned int reg_base, unsigned int *val, 
	            unsigned int reg_len)
{
	unsigned int i = 0;
	
	for(i=0; i< reg_len; i++){
		aml1220_write(reg_base + i, val[i]);
	}

}

static int aml_pmu4_power_init(void)
{
	printk("enter %s\n",__func__);

	//pmu4 analog register init
	aml_pmu4_reg_init(AML1220_ANALOG_ADDR, &pmu4_analog_reg[0], 
		   PMU4_ANALOG_REG_LEN);
	
	return 0;

}
static int aml_pmu4_i2c_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
    printk("enter %s\n",__func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_INFO "i2c check error, dev_id=%s--\n",id->name);
        return -ENODEV;
    }
    i2c_set_clientdata(client, NULL);
	g_aml1220_client = client;

	//aml1220 power init
	aml_pmu4_power_init();

#ifdef CONFIG_DEBUG_FS
	//add debug interface
	debugfs_regs = debugfs_create_file("pmu4_reg", 0644,
						 debugfs_root,
						 NULL, &aml1220_pmu4_reg_fops);
#endif

	return 0;
}

static int aml_pmu4_i2c_remove(struct i2c_client *client)
{
	printk("enter %s\n",__func__);
	kfree(i2c_get_clientdata(client));
	
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id aml_pmu4_match_id = {
        .compatible = "amlogic,aml_pmu4",
};
#endif

static const struct i2c_device_id aml_pmu4_id_table[] = {
    { "aml_pmu4", 0 },
    { }
};


static struct i2c_driver aml_pmu4_i2c_driver = {
	.driver	= {
		.name	= "aml_pmu4",
		.owner	= THIS_MODULE,
    #ifdef CONFIG_OF
        .of_match_table = &aml_pmu4_match_id,
    #endif
	},
	.probe		= aml_pmu4_i2c_probe,
	.remove		= aml_pmu4_i2c_remove,
	.id_table	= aml_pmu4_id_table,
};

static int __init aml_pmu4_modinit(void)
{
    printk("%s, %d\n", __func__, __LINE__);
	return i2c_add_driver(&aml_pmu4_i2c_driver);
}
arch_initcall(aml_pmu4_modinit);

static void __exit aml_pmu4_modexit(void)
{
	i2c_del_driver(&aml_pmu4_i2c_driver);
}
module_exit(aml_pmu4_modexit);

MODULE_DESCRIPTION("Amlogic PMU4 device driver");
MODULE_AUTHOR("Chengshun Wang <chengshun.wang@amlogic.com>");
MODULE_LICENSE("GPL");

