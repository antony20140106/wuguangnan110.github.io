#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/sys.h>
#include <linux/sysfs.h>
#include <linux/miscdevice.h>
#include <linux/cdev.h>
#include <linux/kobject.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
#include <linux/slab.h>

#include "asw5480.h"

struct audio_info {
	struct i2c_client *i2c_client;
	struct mutex lock;

	u32 fault_status;
	u32 i2c_fault_cnt;
#define MAX_I2C_FAULT_CNT 60
};

struct audio_info *g_audio_info;

#define AWS5480_DEVICE_NAME     "aws5480"

#define MONITOR_DBG
#ifdef MONITOR_DBG
#define AUDIO_MONITOR_DBG(fmt, arg...) printk(KERN_ERR "[PAX_AUDIO_SWITCH]:%s " fmt , __func__, ## arg)
#else
#define AUDIO_MONITOR_DBG(fmt, arg...)
#endif
#define AUDIO_MONITOR_ERR(fmt, arg...) printk(KERN_ERR "[PAX_AUDIO_SWITCH] %s: " fmt , __func__ , ## arg)
#define AUDIO_MONITOR_WARN(fmt, arg...) printk(KERN_WARNING "[PAX_AUDIO_SWITCH] %s: " fmt , __func__ , ## arg)

static void aws5480_set_i2c_fault_status(void)
{
	if (g_audio_info->i2c_fault_cnt++ >= MAX_I2C_FAULT_CNT) {
		g_audio_info->i2c_fault_cnt = MAX_I2C_FAULT_CNT;
		g_audio_info->fault_status = 1;
	}
}

static void aws5480_reset_i2c_fault_cnt(void)
{
	g_audio_info->i2c_fault_cnt = 0;
}

static int __maybe_unused aws5480_reg_write_byte_data(u8 reg, u8 data)
{
	struct i2c_client *client = g_audio_info->i2c_client;
	unsigned char buf[2] = {0};

	mutex_lock(&g_audio_info->lock);

	buf[0] = reg;
	buf[1] = data;

	if (2 != i2c_master_send(client, buf, 2)) {
		aws5480_reset_i2c_fault_cnt();
		AUDIO_MONITOR_ERR("%s, reg[%x] fail\n", __func__, reg);
		return -1;
	}

	aws5480_reset_i2c_fault_cnt();


	AUDIO_MONITOR_DBG("reg: 0x%x, data: 0x%x\n", reg, data);

	mutex_unlock(&g_audio_info->lock);

	return 0;
}

static int aws5480_reg_read_byte_data(u8 *data, u8 reg)
{
	struct i2c_client *client = g_audio_info->i2c_client;
	unsigned char buf[2] = {0};

	mutex_lock(&g_audio_info->lock);

	buf[0] = reg;

	if (1 != i2c_master_send(client, buf, 1)) {
		aws5480_set_i2c_fault_status();
		AUDIO_MONITOR_ERR("%s, send reg[%x] fail\n", __func__, reg);
		return -1;
	}

	aws5480_reset_i2c_fault_cnt();

	if (1 != i2c_master_recv(client, buf, 1)) {
		aws5480_set_i2c_fault_status();
		AUDIO_MONITOR_ERR("%s, recv reg[%x] fail\n", __func__, reg);
		return -1;
	}

	*data = buf[0];
	AUDIO_MONITOR_DBG("read reg: %x data: %02x\n", reg, buf[0]);

	mutex_unlock(&g_audio_info->lock);

	return 0;
}

static int aws5480_get_dev_type(unsigned short *type)
{
	u8 val = 0;
	int ret = 0;

	ret = aws5480_reg_read_byte_data(&val, AWS5480_reg_DeviceType);
	if (ret < 0) {
		AUDIO_MONITOR_ERR("get device type failed: %d\n", ret);
		return ret;
	}

	*type = val;
	pr_err("device type: 0x%x\n", *type);

	return 0;
}

int pax_set_audio_switch(bool on)
{
	int ret = 0;

	if (g_audio_info) {
		if (on) {
			ret = aws5480_reg_write_byte_data(ASW5480_SwitchSelect, 0x00);
			if (ret < 0) {
				AUDIO_MONITOR_ERR("get device type failed: %d\n", ret);
				return ret;
			}
		}
		else {
			ret = aws5480_reg_write_byte_data(ASW5480_SwitchSelect, 0x18);
			if (ret < 0) {
				AUDIO_MONITOR_ERR("get device type failed: %d\n", ret);
				return ret;
			}	
		}
		pr_err("pax_set_audio_switch: %d\n", on);
	}
	return 0;
}

int pax_get_audio_switch(void)
{
	u8 type = 0;
	int ret = 0;

	if (g_audio_info) {
		ret = aws5480_reg_read_byte_data(&type, ASW5480_SwitchSelect);
		if (ret < 0) {
			AUDIO_MONITOR_ERR("get device type failed: %d\n", ret);
			return ret;
		}
		
		if (type == 0x18)
			return 0;
		else if (type == 0x00)
			return 1;

		pr_err("pax_get_audio_switch: %d\n", type);
	}
	return 0;
}

static void aws5480_parse_dts(void)
{
	if (!g_audio_info || g_audio_info->i2c_client)
		return;
}

static int aws5480_i2c_check(void)
{
	int ret = 0;
	int try_cnt = 3;
	unsigned short type = 0;

	do {
		ret = aws5480_get_dev_type(&type);
		if (!ret) {
			if (type == ASW5480_DeviceType) {
				return 0;
			}
		}
	} while(try_cnt-- > 0);


	return -1;
}

/* attr for debug */

static ssize_t monitor_show_switch_select(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	unsigned int status = 0;
	int ret = 0;

	status = pax_get_audio_switch();
	
	ret = sprintf(buf, "status: 0x%x\n", status);

	return ret;
}

static ssize_t monitor_store_switch_select(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val;
	int ret;

	ret = sscanf(buf, "%d", &val);
	if (ret != 1)
		return count;
	
	if (val)
		pax_set_audio_switch(true);
	else
		pax_set_audio_switch(false);

	return count;
}

static ssize_t monitor_show_control_ic(struct device *dev,
						struct device_attribute *attr, char *buf)
{
	return sprintf((char *)buf, "%s\n", AWS5480_DEVICE_NAME);
}

static DEVICE_ATTR(switch_select, 0660, monitor_show_switch_select, monitor_store_switch_select);
static DEVICE_ATTR(control_ic, 0440, monitor_show_control_ic, NULL);


/* cdev file ops */
static int cdev_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int cdev_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static long cdev_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return cdev_ioctl(file, cmd, arg);
}

static const struct file_operations cdev_file_operations = {
	.owner          = THIS_MODULE,
	.open           = cdev_open,
	.release        = cdev_release,
	.unlocked_ioctl = cdev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = cdev_compat_ioctl
#endif
};

static int aws5480_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct audio_info *audio_info;
	dev_t c_devno;
	struct device *dev;
	struct cdev *c_dev;
	struct class *audio_class;

	pr_err("aws5480_i2c_probe enter\n");
	audio_info = (struct audio_info *)kzalloc(sizeof(*audio_info), GFP_KERNEL);
	if (!audio_info) {
		AUDIO_MONITOR_ERR("kzmalloc audio info failed.\n");
		ret = -ENOMEM;
		goto out;
	}

	g_audio_info = audio_info;
	g_audio_info->i2c_client = client;
	mutex_init(&g_audio_info->lock);

	aws5480_parse_dts();

	if (aws5480_i2c_check() != 0) {
		ret = -1;
		goto i2c_check_failed;
	}

	/* cdev */
	{
		if (alloc_chrdev_region(&c_devno, 0, 1, AWS5480_DEVICE_NAME)) {
			AUDIO_MONITOR_ERR("alloc chrdev region fail!");
			ret = -1;
			goto cdev_fail_1;
		}

		c_dev = cdev_alloc();
		if (IS_ERR_OR_NULL(c_dev)) {
			AUDIO_MONITOR_ERR("cdev alloc fail!");
			ret = -1;
			goto cdev_fail_2;
		}

		cdev_init(c_dev, &cdev_file_operations);
		ret = cdev_add(c_dev, c_devno, 1);
		if (ret < 0) {
			AUDIO_MONITOR_ERR("cdev add fail!");
			ret = -1;
			goto cdev_fail_2;
		}
	}
	pr_err(" aws5480_i2c_probe enter\n");
	/* class/pax */
	{
		audio_class = class_create(THIS_MODULE, "audio_switch_info");

		if (IS_ERR_OR_NULL(audio_class)) {
			ret = PTR_ERR(audio_class);
			AUDIO_MONITOR_ERR("Unable to create class, err = %d", ret);
			goto class_fail;
		}

		dev = device_create(audio_class, NULL, c_devno, NULL, AWS5480_DEVICE_NAME);

		/* device attributes for debugging */
		device_create_file(dev, &dev_attr_switch_select);
		device_create_file(dev, &dev_attr_control_ic);
	}

	AUDIO_MONITOR_ERR("success\n");
	return 0;

class_fail:
	cdev_del(c_dev);
cdev_fail_2:
	unregister_chrdev_region(c_devno, 1);
cdev_fail_1:
i2c_check_failed:
	mutex_destroy(&audio_info->lock);
	kzfree(audio_info);
	audio_info = NULL;
out:
	return ret;
}

static int aws5480_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static const struct of_device_id aws5480_match_table[] = {
	{.compatible = "grand,aws5480",},
	{ },
};

static const struct i2c_device_id aws5480_i2c_id[] = {
	{AWS5480_DEVICE_NAME, 0},
	{ },
};

static struct i2c_driver aws5480_i2c_driver = {
	.driver = {
		.name = AWS5480_DEVICE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = aws5480_match_table,
	},
	.probe = aws5480_i2c_probe,
	.remove = aws5480_i2c_remove,
	.id_table = aws5480_i2c_id,
};


static int __init aws5480_init(void)
{
	int ret = 0;

	pr_err("enter\n");
	ret = i2c_add_driver(&aws5480_i2c_driver);

	return ret;
}
module_init(aws5480_init);


static void __exit aws5480_exit(void)
{
	i2c_del_driver(&aws5480_i2c_driver);
}
module_exit(aws5480_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("PAX");
MODULE_DESCRIPTION("pax aws5480 audio switch control ic driver");
