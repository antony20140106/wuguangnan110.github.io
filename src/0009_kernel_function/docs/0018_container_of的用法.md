# 概述

Android kernel经常碰到调整驱动加载顺序，详细看一下。

# 参考

* [container_of的用法](https://blog.csdn.net/wzc18743083828/article/details/118730678)

# container_of的用途说明

container_of的主要作用是：通过已知的一个数据结构成员指针ptr，数据结构类型type，以及这个成员指针在这个数据结构中的成员名member，来获取指向这个数据结构的指针type *。

# 历程

将chg->plug_nb参数传入注册notify传参，pax_usb_switch_notifier通过container_of来获取指向这个数据结构chg的指针。
```C++
static int pax_usb_switch_notifier(struct notifier_block *self, unsigned long event, void *value)
{
	int usb_role = 0;
	struct smb_charger *chg = container_of(self, struct smb_charger, plug_nb);

	switch (event) {
		   case SET_USB_SWITCH:
				   usb_role = *(int *)value;
				   pr_err("SET_USB_SWITCH: %d\n", usb_role);
				   smblite_lib_notify_usb_role(chg, usb_role);
				   break;
		   default:
				   break;
	};

	return NOTIFY_DONE;
}

init()
{
	chg->plug_nb.notifier_call = pax_usb_switch_notifier;
	rc = husb311_notify_register_client(&chg->plug_nb);
	if (rc < 0) {
		return 0;
	}
}
```