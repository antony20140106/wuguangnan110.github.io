# README

linux input子系统介绍。

# refers

* [linux input子系统](https://blog.csdn.net/WANGYONGZIXUE/article/details/116462351)

# 前言
按键、鼠标、键盘、触摸屏等都属于输入(input)设备，Linux 内核为此专门做了一个叫做 input子系统的框架来处理输入事件。输入设备本质上还是字符设备，只是在此基础上套上了 input 框架。

input 框架
和 pinctrl 和 gpio 子系统一样，input 子系统管理输入的子系统。

中间部分属于Linux 内核空间，分为驱动层、核心层和事件层

驱动层：输入设备的具体驱动程序，比如按键驱动程序，向内核层报告输入内容。
核心层：承上启下，为驱动层提供输入设备注册和操作接口。通知事件层对输入事件进行处理。
事件层：主要和用户空间进行交互。

input 核心层会向 Linux 内核注册一个字符设备,drivers/input/input.c 这个文件，就是 input 输入子系统的核心层

```C++
struct class input_class = {
        .name           = "input",
        .devnode        = input_devnode,
};

static int __init input_init(void)
{
        int err;

        err = class_register(&input_class);
        if (err) {
                pr_err("unable to register input_dev class\n");
                return err;
        }

        err = input_proc_init();
        if (err)
                goto fail1;

        err = register_chrdev_region(MKDEV(INPUT_MAJOR, 0),
                                     INPUT_MAX_CHAR_DEVICES, "input");
        if (err) {
                pr_err("unable to register char major %d", INPUT_MAJOR);
                goto fail2;
        }

        return 0;

 fail2: input_proc_exit();
 fail1: class_unregister(&input_class);
        return err;
}

static void __exit input_exit(void)
{
        input_proc_exit();
        unregister_chrdev_region(MKDEV(INPUT_MAJOR, 0),
                                 INPUT_MAX_CHAR_DEVICES);
        class_unregister(&input_class);
}
```

注册一个字符设备，主设备号为 INPUT_MAJOR，INPUT_MAJOR 定义在 include/uapi/linux/major.h 文件中，定义如下：
```
#define INPUT_MAJOR 13
```

因此，input 子系统的所有设备主设备号都为 13，我们在使用 input 子系统处理输入设备
的时候就不需要去注册字符设备了，我们只需要向系统注册一个 input_device 即可。

## 1、注册 input_dev
在使用 input 子系统的时候我们只需要注册一个 input 设备即可，input_dev 结构体表示 input设备，此结构体定义在 include/linux/input.h 文件中:
```C++
struct input_dev {
        const char *name;
        const char *phys;
        const char *uniq;
        struct input_id id;

        unsigned long propbit[BITS_TO_LONGS(INPUT_PROP_CNT)];

        unsigned long evbit[BITS_TO_LONGS(EV_CNT)];
        unsigned long keybit[BITS_TO_LONGS(KEY_CNT)];
        unsigned long relbit[BITS_TO_LONGS(REL_CNT)];
        unsigned long absbit[BITS_TO_LONGS(ABS_CNT)];
        unsigned long mscbit[BITS_TO_LONGS(MSC_CNT)];
        unsigned long ledbit[BITS_TO_LONGS(LED_CNT)];
        unsigned long sndbit[BITS_TO_LONGS(SND_CNT)];
        unsigned long ffbit[BITS_TO_LONGS(FF_CNT)];
        unsigned long swbit[BITS_TO_LONGS(SW_CNT)];
```

evbit、keybit、 relbit表示输入事件类型，可选的事件类型定义在 include/uapi/linux/input.h 文件中，事件类型如下：
```C++
/*
 * Event types 事件类型
 */
#define EV_SYN                  0x00
#define EV_KEY                  0x01
#define EV_REL                  0x02
#define EV_ABS                  0x03
#define EV_MSC                  0x04
#define EV_SW                   0x05
#define EV_LED                  0x11
#define EV_SND                  0x12
#define EV_REP                  0x14
#define EV_FF                   0x15
#define EV_PWR                  0x16
#define EV_FF_STATUS            0x17
#define EV_MAX                  0x1f
#define EV_CNT                  (EV_MAX+1)
```

按键事件，因此要用到 keybit，keybit 就是按键事件使用的位图，Linux 内核定义了很多按键值，这些按键值定义在 include/uapi/linux/input.h 文件中，按键值如下：
```C++
#define KEY_RESERVED            0
#define KEY_ESC                 1
#define KEY_1                   2
#define KEY_2                   3
#define KEY_3                   4
#define KEY_4                   5
#define KEY_5                   6
#define KEY_6                   7
#define KEY_7                   8
#define KEY_8                   9
#define KEY_9                   10
#define KEY_0                   11
#define KEY_MINUS               12
#define KEY_EQUAL               13
#define KEY_BACKSPACE           14
#define KEY_TAB                 15
#define KEY_Q                   16
#define KEY_W                   17
#define KEY_E                   18
#define KEY_R                   19
#define KEY_T                   20
#define KEY_Y                   21
#define KEY_U                   22
#define KEY_I                   23
#define KEY_O                   24
#define KEY_P                   25
#define KEY_LEFTBRACE           26
```

编写 input 设备驱动的时候我们需要先申请一个 input_dev 结构体变量,使用input_allocate_device 函数来申请一个 input_dev
```C++
struct input_dev *input_allocate_device(void)
```

申请好一个 input_dev 以后就需要初始化这个 input_dev
```C++
int input_register_device(struct input_dev *dev)
```

注销 input 驱动
```C++
void input_unregister_device(struct input_dev *dev)
```

注销 input 设备
```C++
void input_free_device(struct input_dev *dev)
```

综上所述，input_dev 注册过程如下:

1. 使用 input_allocate_device 函数申请一个 input_dev。
2. 初始化 input_dev 的事件类型以及事件值。
3. 使用 input_register_device 函数向 Linux 系统注册前面初始化好的 input_dev。
4. 卸载input驱动的时候需要先使用input_unregister_device函数注销掉注册的input_dev，然后使用 input_free_device 函数释放掉前面申请的 input_dev。

## 2、上报输入事件,事件上报 API 函数

input_event 函数，此函数用于上报指定的事件以及对应的值，函数原型如下：
```C++
void input_event(struct input_dev *dev, 
						 unsigned int type, 
						 unsigned int code, 
						 int value)
```

参数和返回值含义如下：
* dev：需要上报的 input_dev。
* type: 上报的事件类型，比如 EV_KEY。
* code：事件码，也就是我们注册的按键值，比如 KEY_0、KEY_1 等等。
* value：事件值，比如 1 表示按键按下，0 表示按键松开。

上报按键所使用的input_report_key 函数,就是使用的input_event
```C++
static inline void input_report_key(struct input_dev *dev,
unsigned int code, int value)
{
	 input_event(dev, EV_KEY, code, !!value);
}
```

内核中还有很多上报函数:
```C++
void input_report_rel(struct input_dev *dev, unsigned int code, int value)
void input_report_abs(struct input_dev *dev, unsigned int code, int value）
void input_report_ff_status(struct input_dev *dev, unsigned int code, int value)
void input_report_switch(struct input_dev *dev, unsigned int code, int value)
void input_mt_sync(struct input_dev *dev)
```

上报事件以后还需要使用 input_sync 函数来告诉 Linux 内核 input 子系统上报结束，input_sync 函数本质是上报一个同步事件.
```
void input_sync(struct input_dev *dev)
```

上报事件的示例如下：
```C++
void timer_function(unsigned long arg)
{
	unsigned char value;
	unsigned char num;
	struct irq_keydesc *keydesc;
	struct keyinput_dev *dev = (struct keyinput_dev *)arg;

	num = dev->curkeynum;
	keydesc = &dev->irqkeydesc[num];
	value = gpio_get_value(keydesc->gpio); 	/* 读取IO值 */
	if(value == 0){ 						/* 按下按键 */
		/* 上报按键值 */
		input_report_key(dev->inputdev, keydesc->value, 1);/* 最后一个参数表示按下还是松开，1为按下，0为松开 */
		input_sync(dev->inputdev);
	} else { 									/* 按键松开 */
		input_report_key(dev->inputdev, keydesc->value, 0);
		input_sync(dev->inputdev);
	}	
}
```

## 3、input_event 结构体
Linux 内核使用 input_event 这个结构体来表示所有的输入事件，input_envent 结构体定义在
include/uapi/linux/input.h 文件中
```c
struct input_event {
	struct timeval time;
	__u16 type;
	__u16 code;
	__s32 value;
 };
```

type：事件类型，比如 EV_KEY，表示此次事件为按键事件，此成员变量为 16 位。
code：事件码，比如在 EV_KEY 事件中 code 就表示具体的按键码，如：KEY_0、KEY_1
等等这些按键。此成员变量为 16 位。
value：值，比如 EV_KEY 事件中 value 就是按键值，表示按键有没有被按下，如果为 1 的
话说明按键按下，如果为 0 的话说明按键没有被按下或者按键松开了。

# 驱动实战

## 1、按键 input 驱动程序

```C++
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/input.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define KEYINPUT_CNT		1			/* 设备号个数 	*/
#define KEYINPUT_NAME		"keyinput"	/* 名字 		*/
#define KEY0VALUE			0X01		/* KEY0按键值 	*/
#define INVAKEY				0XFF		/* 无效的按键值 */
#define KEY_NUM				1			/* 按键数量 	*/

/* 中断IO描述结构体 */
struct irq_keydesc {
	int gpio;								/* gpio */
	int irqnum;								/* 中断号     */
	unsigned char value;					/* 按键对应的键值 */
	char name[10];							/* 名字 */
	irqreturn_t (*handler)(int, void *);	/* 中断服务函数 */
};

/* keyinput设备结构体 */
struct keyinput_dev{
	dev_t devid;			/* 设备号 	 */
	struct cdev cdev;		/* cdev 	*/
	struct class *class;	/* 类 		*/
	struct device *device;	/* 设备 	 */
	struct device_node	*nd; /* 设备节点 */
	struct timer_list timer;/* 定义一个定时器*/
	struct irq_keydesc irqkeydesc[KEY_NUM];	/* 按键描述数组 */
	unsigned char curkeynum;				/* 当前的按键号 */
	struct input_dev *inputdev;		/* input结构体 */
};

struct keyinput_dev keyinputdev;	/* key input设备 */

//中断服务函数
static irqreturn_t key0_handler(int irq, void *dev_id)
{
	struct keyinput_dev *dev = (struct keyinput_dev *)dev_id;

	dev->curkeynum = 0;
	dev->timer.data = (volatile long)dev_id;
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));	/* 10ms定时 用于去抖 */
	return IRQ_RETVAL(IRQ_HANDLED);
}

//定时器服务函数，用于按键消抖，定时器到了以后再次读取按键值，如果按键还是处于按下状态就表示按键有效。
void timer_function(unsigned long arg)
{
	unsigned char value;
	unsigned char num;
	struct irq_keydesc *keydesc;
	struct keyinput_dev *dev = (struct keyinput_dev *)arg;

	num = dev->curkeynum;
	keydesc = &dev->irqkeydesc[num];
	value = gpio_get_value(keydesc->gpio); 	/* 读取IO值 */
	if(value == 0){ 						/* 按下按键 */
		/* 上报按键值 */
		//input_event(dev->inputdev, EV_KEY, keydesc->value, 1);
		input_report_key(dev->inputdev, keydesc->value, 1);/* 最后一个参数表示按下还是松开，1为按下，0为松开 */
		input_sync(dev->inputdev);
	} else { 									/* 按键松开 */
		//input_event(dev->inputdev, EV_KEY, keydesc->value, 0);
		input_report_key(dev->inputdev, keydesc->value, 0);
		input_sync(dev->inputdev);
	}	
}


static int keyio_init(void)
{
	unsigned char i = 0;
	char name[10];
	int ret = 0;
	
	keyinputdev.nd = of_find_node_by_path("/key");
	if (keyinputdev.nd== NULL){
		printk("key node not find!\r\n");
		return -EINVAL;
	} 

	/* 提取GPIO */
	for (i = 0; i < KEY_NUM; i++) {
		keyinputdev.irqkeydesc[i].gpio = of_get_named_gpio(keyinputdev.nd ,"key-gpio", i);
		if (keyinputdev.irqkeydesc[i].gpio < 0) {
			printk("can't get key%d\r\n", i);
		}
	}
	
	/* 初始化key所使用的IO，并且设置成中断模式 */
	for (i = 0; i < KEY_NUM; i++) {
		memset(keyinputdev.irqkeydesc[i].name, 0, sizeof(name));	/* 缓冲区清零 */
		sprintf(keyinputdev.irqkeydesc[i].name, "KEY%d", i);		/* 组合名字 */
		gpio_request(keyinputdev.irqkeydesc[i].gpio, name);
		gpio_direction_input(keyinputdev.irqkeydesc[i].gpio);	
		keyinputdev.irqkeydesc[i].irqnum = irq_of_parse_and_map(keyinputdev.nd, i);
	}
	/* 申请中断 */
	keyinputdev.irqkeydesc[0].handler = key0_handler;
	keyinputdev.irqkeydesc[0].value = KEY_0;
	
	for (i = 0; i < KEY_NUM; i++) {
		ret = request_irq(keyinputdev.irqkeydesc[i].irqnum, keyinputdev.irqkeydesc[i].handler, 
		                 IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, keyinputdev.irqkeydesc[i].name, &keyinputdev);
		if(ret < 0){
			printk("irq %d request failed!\r\n", keyinputdev.irqkeydesc[i].irqnum);
			return -EFAULT;
		}
	}

	/* 创建定时器 */
	init_timer(&keyinputdev.timer);
	keyinputdev.timer.function = timer_function;

	/* 申请input_dev */
	keyinputdev.inputdev = input_allocate_device();
	keyinputdev.inputdev->name = KEYINPUT_NAME;
#if 0
	/* 初始化input_dev，设置产生哪些事件 */
	__set_bit(EV_KEY, keyinputdev.inputdev->evbit);	/* 设置产生按键事件          */
	__set_bit(EV_REP, keyinputdev.inputdev->evbit);	/* 重复事件，比如按下去不放开，就会一直输出信息 		 */

	/* 初始化input_dev，设置产生哪些按键 */
	__set_bit(KEY_0, keyinputdev.inputdev->keybit);	
#endif

#if 0
	keyinputdev.inputdev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);
	keyinputdev.inputdev->keybit[BIT_WORD(KEY_0)] |= BIT_MASK(KEY_0);
#endif

	keyinputdev.inputdev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);
	input_set_capability(keyinputdev.inputdev, EV_KEY, KEY_0);

	/* 注册输入设备 */
	ret = input_register_device(keyinputdev.inputdev);
	if (ret) {
		printk("register input device failed!\r\n");
		return ret;
	}
	return 0;
}

static int __init keyinput_init(void)
{
	keyio_init();
	return 0;
}

static void __exit keyinput_exit(void)
{
	unsigned int i = 0;
	/* 删除定时器 */
	del_timer_sync(&keyinputdev.timer);	/* 删除定时器 */
		
	/* 释放中断 */
	for (i = 0; i < KEY_NUM; i++) {
		free_irq(keyinputdev.irqkeydesc[i].irqnum, &keyinputdev);
	}
	/* 释放input_dev */
	input_unregister_device(keyinputdev.inputdev);
	input_free_device(keyinputdev.inputdev);
}

module_init(keyinput_init);
module_exit(keyinput_exit);
MODULE_LICENSE("GPL");
```

* 1) 使用 KEY_0为按键值
* 2) 使用中断，
* 3) 使用定时器，延时10ms去抖

## 2、应用程序APP

```C++
**#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <linux/input.h>
/* 定义一个input_event变量，存放输入事件信息 */
static struct input_event inputevent;

int main(int argc, char *argv[])
{
	int fd;
	int err = 0;
	char *filename;

	filename = argv[1];

	if(argc != 2) {
		printf("Error Usage!\r\n");
		return -1;
	}

	fd = open(filename, O_RDWR);
	if (fd < 0) {
		printf("Can't open file %s\r\n", filename);
		return -1;
	}

	while (1) {
		err = read(fd, &inputevent, sizeof(inputevent));
		if (err > 0) { /* 读取数据成功 */
			switch (inputevent.type) {
				case EV_KEY:
					if (inputevent.code < BTN_MISC) { /* 键盘键值 */
						printf("key %d %s\r\n", inputevent.code, inputevent.value ? "press" : "release");
					} else {
						printf("button %d %s\r\n", inputevent.code, inputevent.value ? "press" : "release");
					}
					break;

				/* 其他类型的事件，自行处理 */
				case EV_REL:
					break;
				case EV_ABS:
					break;
				case EV_MSC:
					break;
				case EV_SW:
					break;
			}
		} else {
			printf("读取数据失败\r\n");
		}
	}
	return 0;
}
```