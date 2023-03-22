#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/sys.h>
#include <linux/sysfs.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/pm_wakeup.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/fb.h>
#include <mt-plat/mtk_boot.h>
#include <linux/power_supply.h>

#include "tcpm.h"
#include "charger_class.h"

#ifdef CONFIG_xxx_GPIOS_CONTROL
#include "r15_status_notify.h"
#endif

struct usb_switch_data {
	struct class *xxxxx_class;
	struct device *dev;

	struct tcpc_device *tcpc_dev;
	struct charger_device *chg_dev;
	struct notifier_block tcpc_nb;
	struct notifier_block chg_dev_nb;
	struct delayed_work tcpc_dwork;
	struct delayed_work chg_dev_dwork;
	struct delayed_work pogo_detect_dwork;
	struct notifier_block xxxxx_usb_fb_notifier;
	struct notifier_block r15_nb;
	uint8_t typec_state;
	int lcd_state;
	int usb_type;

	struct mutex lock;
	struct delayed_work usb_switch_work;

	int usb_host_en_gpio;
	int usb_sw1_sel_gpio;
	int usb_sw2_sel_gpio;
	int gl850_en_gpio;
	int pogo_detect_gpio;
	int r15_state;
	int pogo_dev_state;

	u32 default_mode;
};

extern struct class *g_class_xxxxx;
int usb_switch_status = 0;

static void usb_host_switch(struct usb_switch_data *data, int on);

#ifdef CONFIG_OF
struct tag_bootmode {
   u32 size;
   u32 tag;
   u32 bootmode;
   u32 boottype;
};
  
static int check_boot_mode(struct device *dev)
{
	struct device_node *boot_node = NULL;
	struct tag_bootmode *tag = NULL;

	boot_node = of_parse_phandle(dev->of_node, "bootmode", 0);
	if (!boot_node) {
		pr_notice("%s: failed to get boot mode phandle\n", __func__);
		return 0;
	}
	else {
		tag = (struct tag_bootmode *)of_get_property(boot_node,
							"atag,boot", NULL);
		if (!tag) {
			pr_notice("%s: failed to get atag,boot\n", __func__);
			return 0;
		}
		else {
			pr_notice("%s: size:0x%x tag:0x%x bootmode:0x%x boottype:0x%x\n",
				__func__, tag->size, tag->tag,
				tag->bootmode, tag->boottype);
		}
	}
	return tag->bootmode;
}
#endif

static irqreturn_t pogo_detect_irq_handle(int irq, void *dev_id)
{
	struct usb_switch_data *data = (struct usb_switch_data *)dev_id;
	int level = 0;
	int status;

	if (gpio_is_valid(data->pogo_detect_gpio)) {
		level = gpio_get_value(data->pogo_detect_gpio);
		if (level) {
			status = POGO_DEV_STATE_ONLINE;
			SET_BIT(usb_switch_status,0);
		}
		else {
			status = POGO_DEV_STATE_OFFLINE;
			CLR_BIT(usb_switch_status,0);
		}

#ifdef CONFIG_xxx_GPIOS_CONTROL
		r15_status_notify_call_chain(SET_POGO_DEV_STATE, &status);
#endif
		pr_err("level: %d usb_switch_status: %d\n", level,usb_switch_status);
	}
	
	return IRQ_HANDLED;	
}

extern const char *cmdline_get_value(const char *key);

int xxxxx_charger_gpio_init(struct usb_switch_data *data, struct device_node *np)
{
	int ret;
	data->usb_host_en_gpio = of_get_named_gpio(np, "usb_host_en_gpio", 0);
	ret = gpio_request(data->usb_host_en_gpio, "usb_host_en_gpio");	
	if (ret < 0) {
		pr_err("Error: failed to request usb_host_en_gpio%d (ret = %d)\n",
		data->usb_host_en_gpio, ret);
		goto init_alert_err;
	}

	data->usb_sw1_sel_gpio = of_get_named_gpio(np, "usb_sw1_sel_gpio", 0);
	ret = gpio_request(data->usb_sw1_sel_gpio, "usb_sw1_sel_gpio");
	if (ret < 0) {
		pr_err("Error: failed to request usb_sw1_sel_gpio%d (ret = %d)\n",
		data->usb_sw1_sel_gpio, ret);
		goto init_alert_err;
	}

	data->usb_sw2_sel_gpio = of_get_named_gpio(np, "usb_sw2_sel_gpio", 0);
	ret = gpio_request(data->usb_sw2_sel_gpio, "usb_sw2_sel_gpio");
	if (ret < 0) {
		pr_err("Error: failed to request usb_sw2_sel_gpio%d (ret = %d)\n",
		data->usb_sw2_sel_gpio, ret);
		goto init_alert_err;
	}

	data->gl850_en_gpio = of_get_named_gpio(np, "gl850_en_gpio", 0);
	ret = gpio_request(data->gl850_en_gpio, "gl850_en_gpio");
	if (ret < 0) {
		pr_err("Error: failed to request gl850_en_gpio%d (ret = %d)\n",
		data->gl850_en_gpio, ret);
		goto init_alert_err;
	}
	ret = of_property_read_u32(np, "default_mode", (u32 *)&data->default_mode);
	if (ret < 0)
		data->default_mode = 0;

#if 1
	data->pogo_detect_gpio = of_get_named_gpio(np, "pogo_detect_gpio", 0);
	if (gpio_is_valid(data->pogo_detect_gpio)) {
		ret = gpio_request(data->pogo_detect_gpio, "pogo_detect_gpio");
		if (ret < 0) {
			pr_err("pogo_detect_gpio request failed.\n");
			data->pogo_detect_gpio = -1;
		}
	}
#endif

	return 0;

init_alert_err:
	return -EINVAL;
}

static void do_usb_switch_work(struct work_struct *work)
{
	struct usb_switch_data *data = (struct usb_switch_data *)
						container_of(work, struct usb_switch_data, usb_switch_work.work);

	usb_host_switch(data, 0);

}

static ssize_t cur_switch_show_value(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	//struct usb_switch_data *data = dev_get_drvdata(dev->parent);

	return sprintf(buf, "%d\n", usb_switch_status);
}

static ssize_t cur_switch_store_value(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int state;
	ssize_t ret;
	struct usb_switch_data *data = dev_get_drvdata(dev->parent);

	ret = kstrtouint(buf, 10, &state);
	if (ret) {
		pr_notice("set state fail\n");
		return ret;
	}
	
	usb_host_switch(data, state);
	ret = size;
	return ret;
}

static struct device_attribute dev_attr[] = {
	__ATTR(cur_switch, 0660, cur_switch_show_value, cur_switch_store_value),
	__ATTR_NULL
};

/**
 *     When the boot mode is poweroff charger or meta mode, 
 *     we usb USB1 OTG and don't use usb switch function.
 */
int xxxxx_usb_switch_poweoff_charging_mode(struct device *dev)
{
	unsigned int boot_mode = check_boot_mode(dev);

	if (((boot_mode == NORMAL_BOOT) || (boot_mode == SW_REBOOT)
				|| (boot_mode == ALARM_BOOT) || (boot_mode == DONGLE_BOOT))) {
		return 0;
	}

	return 1;
}

static void vbus_en(struct usb_switch_data *data, int on)
{
	if (gpio_is_valid(data->usb_host_en_gpio))
		gpio_set_value(data->usb_host_en_gpio, 0);
}

/**
 * @return
 *   usb_switch_status bit0 :pogo dev status bit1:usb switch mode
 *   0：pogo dev not insert, usb device mode
 *   1：pogo dev insert, usb device mode
 *   2：pogo dev not insert, usb host mode
 *   3：pogo dev insert, usb host mode
 */
int get_usb_switch_status(void)
{
	pr_info("%s: usb_switch_status:%d\n", __func__, usb_switch_status);
	return usb_switch_status;
}

static void usb_host_switch(struct usb_switch_data *data, int on)
{
	pr_info("%s: on:%d\n", __func__, on);

	cancel_delayed_work(&data->usb_switch_work);

	if (on) {
		if (gpio_is_valid(data->gl850_en_gpio))
			gpio_set_value(data->gl850_en_gpio, 1);
		if (gpio_is_valid(data->usb_sw1_sel_gpio))
			gpio_set_value(data->usb_sw1_sel_gpio, 1);
		if (gpio_is_valid(data->usb_sw2_sel_gpio))
			gpio_set_value(data->usb_sw2_sel_gpio, 1);
		SET_BIT(usb_switch_status,1);
	}
	else {
		if (gpio_is_valid(data->gl850_en_gpio))
			gpio_set_value(data->gl850_en_gpio, 0);
		if (gpio_is_valid(data->usb_sw1_sel_gpio))
			gpio_set_value(data->usb_sw1_sel_gpio, 0);
		if (gpio_is_valid(data->usb_sw2_sel_gpio))
			gpio_set_value(data->usb_sw2_sel_gpio, 0);
		CLR_BIT(usb_switch_status,1);
	}
}

static int tcpc_notifier_call(struct notifier_block *nb,  unsigned long action, void *noti_data)
{
	struct tcp_notify *noti = noti_data;
	struct usb_switch_data *data = (struct usb_switch_data *)container_of(nb, struct usb_switch_data, tcpc_nb);

	pr_info("%s: action:%d\n", __func__, action);

	switch(action) {
		case TCP_NOTIFY_SOURCE_VBUS:
			vbus_en(data, !!noti->vbus_state.mv);
			break;

		case TCP_NOTIFY_TYPEC_STATE:
			data->typec_state = noti->typec_state.new_state;

			if ((noti->typec_state.old_state == TYPEC_UNATTACHED) &&
					(noti->typec_state.new_state == TYPEC_ATTACHED_SRC)) {
				usb_host_switch(data, 1);
			}
			else if ((noti->typec_state.old_state == TYPEC_UNATTACHED) &&
					(noti->typec_state.new_state == TYPEC_ATTACHED_SNK)) {
				if (data->pogo_dev_state != POGO_DEV_STATE_ONLINE) {
					usb_host_switch(data, 0);
				}
			}
			else if ((noti->typec_state.old_state != TYPEC_UNATTACHED) &&
					(noti->typec_state.new_state == TYPEC_UNATTACHED)) {
				if (noti->typec_state.old_state == TYPEC_ATTACHED_SRC) {
					if (data->lcd_state == FB_BLANK_POWERDOWN) {
						schedule_delayed_work(&data->usb_switch_work, msecs_to_jiffies(10000));
					}
					else {
						usb_host_switch(data, !!data->default_mode);
					}
				}
				else {
					usb_host_switch(data, !!data->default_mode);
				}
			}
			break;

		case TCP_NOTIFY_DR_SWAP:
			if (noti->swap_state.new_role == PD_ROLE_UFP) {
				if (!((data->default_mode) && (data->usb_type == POWER_SUPPLY_USB_TYPE_DCP))) {
					usb_host_switch(data, 0);
				}
			}
			else if (noti->swap_state.new_role == PD_ROLE_DFP) {
				usb_host_switch(data, 1);
			}
			break;

		case TCP_NOTIFY_PR_SWAP:
			if (noti->swap_state.new_role == PD_ROLE_SINK) {
				vbus_en(data, 0);
			}
			else if (noti->swap_state.new_role == PD_ROLE_SOURCE) {
				vbus_en(data, 1);
			}
			break;
	}

	return NOTIFY_OK;
}

static void xxxxx_tcpc_dev_init_work(struct work_struct *work)
{
	int ret = 0;
	struct usb_switch_data *data = (struct usb_switch_data *)container_of(work, struct usb_switch_data, tcpc_dwork.work);

	if (!data->tcpc_dev) {
		data->tcpc_dev = tcpc_dev_get_by_name("type_c_port0");
		if (!data->tcpc_dev) {
			schedule_delayed_work(&data->tcpc_dwork, msecs_to_jiffies(200));
			return;
		}
	}

	data->tcpc_nb.notifier_call = tcpc_notifier_call;
	ret = register_tcp_dev_notifier(data->tcpc_dev, &data->tcpc_nb,
			TCP_NOTIFY_TYPE_VBUS | TCP_NOTIFY_TYPE_USB | TCP_NOTIFY_TYPE_MISC);
	if (ret < 0) {
		schedule_delayed_work(&data->tcpc_dwork, msecs_to_jiffies(200));
		return;
	}
}

static void xxxxx_tcpc_dev_init(struct usb_switch_data *data)
{
	data->lcd_state = FB_BLANK_UNBLANK;
	usb_host_switch(data, !!data->default_mode);

	INIT_DELAYED_WORK(&data->tcpc_dwork, xxxxx_tcpc_dev_init_work);
	schedule_delayed_work(&data->tcpc_dwork, 0);
}

static int chg_dev_notifier_call(struct notifier_block *nb,  unsigned long event, void *noti_data)
{
	static struct power_supply *chg_psy = NULL;
	union power_supply_propval prop;
	int ret;
	struct usb_switch_data *data = (struct usb_switch_data *)container_of(nb, struct usb_switch_data, chg_dev_nb);

	if (event != CHARGER_DEV_NOTIFY_BC)
		return NOTIFY_OK;

	chg_psy = power_supply_get_by_name("mtk_charger_type");
	if (IS_ERR_OR_NULL(chg_psy))
		return NOTIFY_OK;

	ret = power_supply_get_property(chg_psy, POWER_SUPPLY_PROP_USB_TYPE, &prop);
	if (ret < 0) {
		return NOTIFY_OK;
	}

	data->usb_type = prop.intval;

	if (data->usb_type == POWER_SUPPLY_USB_TYPE_DCP) {
		if (data->default_mode) {
			if (data->lcd_state == FB_BLANK_UNBLANK) {
				usb_host_switch(data, 1);
			}
		}
	}

	return NOTIFY_OK;
}

static void xxxxx_chg_dev_init_work(struct work_struct *work)
{
	struct usb_switch_data *data = (struct usb_switch_data *)container_of(work, struct usb_switch_data, chg_dev_dwork.work);

	data->chg_dev = get_charger_by_name("primary_chg");
	if (!IS_ERR_OR_NULL(data->chg_dev)) {
		data->chg_dev_nb.notifier_call = chg_dev_notifier_call;
		chg_dev_notifier_call(&data->chg_dev_nb, CHARGER_DEV_NOTIFY_BC, NULL);
		register_charger_device_notifier(data->chg_dev, &data->chg_dev_nb);
	}
	else {
		schedule_delayed_work(&data->chg_dev_dwork, msecs_to_jiffies(500));
	}
}

static void xxxxx_chg_dev_init(struct usb_switch_data *data)
{
	data->usb_type = POWER_SUPPLY_TYPE_UNKNOWN;

	INIT_DELAYED_WORK(&data->chg_dev_dwork, xxxxx_chg_dev_init_work);
	schedule_delayed_work(&data->chg_dev_dwork, 0);
}

static void pogo_detect_notify_dwork(struct work_struct *work)
{
	struct usb_switch_data *data = (struct usb_switch_data *)container_of(work, struct usb_switch_data, pogo_detect_dwork.work);

	pogo_detect_irq_handle(gpio_to_irq(data->pogo_detect_gpio), data);
}

static int r15_status_notifier_call(struct notifier_block *self, unsigned long event, void *data)
{
	struct usb_switch_data *usb_sdata = (struct usb_switch_data *)container_of(
			self, struct usb_switch_data, r15_nb);
	int mode;

	if (event == SET_POWER_STATUS) {
		usb_sdata->r15_state = *(int *)data;
		pr_info("r15_state: %d\n", usb_sdata->r15_state);
	}
	else if (event == SET_USB_MODE_SWTICH) {
		mode = *(int *)data;
		pr_info("set usb mode: %d\n", mode);
		if (usb_sdata->lcd_state == FB_BLANK_UNBLANK) {
			if (USB_MODE_HOST == mode) {
				usb_host_switch(usb_sdata, 1);
			}
			else {
				usb_host_switch(usb_sdata, 0);
			}
		}
	}
	else if (event == SET_POGO_DEV_STATE) {
		usb_sdata->pogo_dev_state = *(int *)data;
		pr_info("pogo_dev_state: %d\n", usb_sdata->pogo_dev_state);

		if (usb_sdata->pogo_dev_state == POGO_DEV_STATE_ONLINE) {
			if (usb_sdata->lcd_state == FB_BLANK_UNBLANK) {
				usb_host_switch(usb_sdata, 1);
			}
		}
	}

	return NOTIFY_OK;
}

static void pogo_detect_init(struct usb_switch_data *data)
{
	int irq;
	int ret;

	data->r15_state = R15_STATUS_OFFLINE;
	data->pogo_dev_state = POGO_DEV_STATE_OFFLINE;

	if (gpio_is_valid(data->pogo_detect_gpio)) {
		ret = gpio_direction_input(data->pogo_detect_gpio);
		irq = gpio_to_irq(data->pogo_detect_gpio);
		ret = request_irq(irq, pogo_detect_irq_handle, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING|IRQF_NO_SUSPEND|IRQF_NO_THREAD, 
				"pogo_detect_gpio", data);
		if (ret < 0) {
			pr_err("Error: failed to request pogo_detect_gpio%d (ret = %d)\n",
					data->pogo_detect_gpio, ret);
		}

		data->r15_nb.priority = 99;
		data->r15_nb.notifier_call = r15_status_notifier_call;
		r15_status_notify_register_client(&data->r15_nb);

		INIT_DELAYED_WORK(&data->pogo_detect_dwork, pogo_detect_notify_dwork);
		schedule_delayed_work(&data->pogo_detect_dwork, msecs_to_jiffies(500));
	}
}

static int xxxxx_usb_fb_notifier_call(struct notifier_block *self, unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int blank;
	struct usb_switch_data *usb_sdata = (struct usb_switch_data *)container_of(
			self, struct usb_switch_data, xxxxx_usb_fb_notifier);

	if (event != FB_EVENT_BLANK)
		return 0;

	blank = *(int *)evdata->data;

	switch (blank) {
		case FB_BLANK_UNBLANK:
			if ((usb_sdata->typec_state == TYPEC_UNATTACHED) || (usb_sdata->usb_type == POWER_SUPPLY_USB_TYPE_DCP))
				usb_host_switch(usb_sdata, 1);

			usb_sdata->lcd_state = blank;
			break;
		case FB_BLANK_POWERDOWN:
			if (usb_sdata->typec_state == TYPEC_UNATTACHED) {
				schedule_delayed_work(&usb_sdata->usb_switch_work, msecs_to_jiffies(10000));
			}

			usb_sdata->lcd_state = blank;
			break;
		default:
			break;
	}

	return 0;
}

extern const char *cmdline_get_value(const char *key);
static int xxxxx_usb_swtich_probe(struct platform_device *pdev)
{
	int ret = 0,i = 0;
	struct usb_switch_data *data;
	struct device_node *np = pdev->dev.of_node;
	const char *terminal_name;

    //[FEATURE]-MOD-BEGIN by xxx@xxxxx.com 2021-07-01, for M5x product no usb switch
    terminal_name = cmdline_get_value("androidboot.terminal_name");
    if(strcmp(terminal_name, "M5x") == 0 || xxxxx_usb_switch_poweoff_charging_mode(&pdev->dev))
    {
		pr_err( "product M5x and power off  && meta mode no usb switch module.\n");
		goto err0;
    }
    //[FEATURE]-MOD-END by xxx@xxxxx.com 2021-07-01, for M5x product no usb switch
	
	pr_err("%s\n", __func__);

	data = devm_kzalloc(&pdev->dev, sizeof(struct usb_switch_data), GFP_KERNEL);
	if (!data) {
		pr_err("%s, alloc fail\n", __func__);
		return -ENOMEM;
	}

	/* resource */
	xxxxx_charger_gpio_init(data,np);
	if (ret) {
		goto req_res_fail;
	}

	INIT_DELAYED_WORK(&data->usb_switch_work, do_usb_switch_work);

	xxxxx_tcpc_dev_init(data);
	xxxxx_chg_dev_init(data);
	pogo_detect_init(data);

	if (data->default_mode) {
		data->xxxxx_usb_fb_notifier.priority = 99;
		data->xxxxx_usb_fb_notifier.notifier_call = xxxxx_usb_fb_notifier_call;
		ret = fb_register_client(&data->xxxxx_usb_fb_notifier);
	}

	if (g_class_xxxxx != NULL) {
		data->xxxxx_class = g_class_xxxxx;
	} else {
		data->xxxxx_class = class_create(THIS_MODULE, "xxxxx");
	}
	
	//creat sysfs for debug /sys/class/xxxxx/usb_switch/cur_switch
	data->dev = device_create(data->xxxxx_class, &pdev->dev, 0, NULL, "usb_switch");
	platform_set_drvdata(pdev, data);
	
	for (i = 0; ; i++) {
		if (dev_attr[i].attr.name == NULL)
			break;
		device_create_file(data->dev, &dev_attr[i]);
	}

	pr_info("%s: success.\n", __func__);
	return 0;

req_res_fail:
	devm_kfree(&pdev->dev, data);
	return ret;
err0:
    pr_err("usb_switch probe occured error \n");
    return ret;
}

static int xxxxx_usb_swtich_remove(struct platform_device *pdev)
{
	struct usb_switch_data *data = platform_get_drvdata(pdev);
	int i;
	
	if (data) {
		cancel_delayed_work_sync(&data->usb_switch_work);
		gpio_free(data->usb_host_en_gpio);
		
		for (i = 0; ; i++) {
			if (dev_attr[i].attr.name == NULL)
				break;
			device_remove_file(data->dev, &dev_attr[i]);
		}
		mutex_destroy(&data->lock);

		devm_kfree(&pdev->dev, data);
	}

	return 0;
}


static const struct of_device_id xxxxx_usb_switch_of_match[] = {
	{ .compatible = "xxxxx,usb_switch" },
	{},
};

MODULE_DEVICE_TABLE(of, xxxxx_usb_switch_of_match);


static int xxxxx_usb_switch_suspend(struct device *dev)
{
	pr_err("==%s\n", __func__);
	return 0;
}

static int xxxxx_usb_switch_resume(struct device *dev)
{
	pr_err("==%s\n", __func__);
	return 0;
}

static void xxxxx_usb_switch_shutdown(struct platform_device *pdev)
{
	pr_err("==%s\n", __func__);
}

static const struct dev_pm_ops xxxxx_usb_swtich_pm_ops = {
	.suspend = xxxxx_usb_switch_suspend,
	.resume = xxxxx_usb_switch_resume,
};

static struct platform_driver xxxxx_usb_switch_driver = {
	.probe = xxxxx_usb_swtich_probe,
	.remove = xxxxx_usb_swtich_remove,
	.driver = {
		.name = "xxxxx_usb_switch",
		.of_match_table = xxxxx_usb_switch_of_match,
		.pm = &xxxxx_usb_swtich_pm_ops,
	},
	.shutdown = xxxxx_usb_switch_shutdown,
};


static int __init xxxxx_usb_switch_init(void)
{
	return platform_driver_register(&xxxxx_usb_switch_driver);
}

static void __exit xxxxx_usb_switch_exit(void)
{
	platform_driver_unregister(&xxxxx_usb_switch_driver);
}

/**
 * if listen typec event, must be late_init wait for pd core init;
 * in no_bat mode, must early then typec/pd, in case tyepc/pd cutoff vbus.
 */
#ifdef REGISTER_TYPEC_NOTIFY
late_initcall(xxxxx_usb_switch_init);
#else
module_init(xxxxx_usb_switch_init);
//subsys_initcall(xxxxx_usb_switch_init);
#endif

module_exit(xxxxx_usb_switch_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("xxx");
MODULE_DESCRIPTION("xxxxx usb switch");
