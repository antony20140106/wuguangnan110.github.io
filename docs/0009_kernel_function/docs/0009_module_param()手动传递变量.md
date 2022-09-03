# 概述

内核模块module_param传参实例，并在sysfs目录下实操

# 参考

* [怎样使用module_param()来手动传递变量](https://blog.csdn.net/u011006622/article/details/7916
* [module_param()函数](https://kunaly.blog.csdn.net/article/details/96001018?spm=1001.2101.3001.6661.1&utm_medium=distribute.pc_relevant_t0.none-task-blog-2%7Edefault%7ECTRLIST%7Edefault-1-96001018-blog-117125919.pc_relevant_multi_platform_whitelistv1&depth_1-utm_source=distribute.pc_relevant_t0.none-task-blog-2%7Edefault%7ECTRLIST%7Edefault-1-96001018-blog-117125919.pc_relevant_multi_platform_whitelistv1&utm_relevant_index=1)

## module_param()的定义

module_param()宏是Linux 2.6内核中新增的，该宏被定义在include/linux/moduleparam.h文件中，定义如下：

```C++

/**
  * module_param - typesafe helper for a module/cmdline parameter
  * @value: the variable to alter, and exposed parameter name.
  * @type: the type of the parameter
  * @perm: visibility in sysfs.
  *
  * @value becomes the module parameter, or (prefixed by KBUILD_MODNAME and a
  * ".") the kernel commandline parameter.  Note that - is changed to _, so
  * the user can use "foo-bar=1" even for variable "foo_bar".
  *
  * @perm is 0 if the the variable is not to appear in sysfs, or 0444
  * for world-readable, 0644 for root-writable, etc.  Note that if it
  * is writable, you may need to use kparam_block_sysfs_write() around
  * accesses (esp. charp, which can be kfreed when it changes).
  *
  * The @type is simply pasted to refer to a param_ops_##type and a
  * param_check_##type: for convenience many standard types are provided but
  * you can create your own by defining those variables.
  *
  * Standard types are:
  *      byte, short, ushort, int, uint, long, ulong
  *      charp: a character pointer
  *      bool: a bool, values 0/1, y/n, Y/N.
  *      invbool: the above, only sense-reversed (N = true).
  */
 
#define module_param(name, type, perm)
          module_param_named(name, name, type, perm)
 
 
 /**
  * module_param_named - typesafe helper for a renamed module/cmdline parameter
  * @name: a valid C identifier which is the parameter name.
  * @value: the actual lvalue to alter.
  * @type: the type of the parameter
  * @perm: visibility in sysfs.
  *
  * Usually it's a good idea to have variable names and user-exposed names the
  * same, but that's harder if the variable must be non-static or is inside a
  * structure.  This allows exposure under a different name.
```

原型：module_param(name, type, perm);

参数：

* name:既是用户看到的参数名，又是模块内接受参数的变量；;
* type:表示参数的类型;
* perm:指定了在sysfs中相应文件的访问权限;

这个宏定义应当放在任何函数之外, 典型地是出现在源文件的前面.定义如下：

```C++
int kernel_init_done;
int musb_force_on;
int musb_host_dynamic_fifo = 1;
int musb_host_dynamic_fifo_usage_msk;
bool musb_host_db_enable;
bool musb_host_db_workaround1;
bool musb_host_db_workaround2;
long musb_host_db_delay_ns;
long musb_host_db_workaround_cnt;
int mtk_host_audio_free_ep_udelay = 1000;

module_param(musb_fake_CDP, int, 0644);
module_param(kernel_init_done, int, 0644);
module_param(musb_host_dynamic_fifo, int, 0644);
module_param(musb_host_dynamic_fifo_usage_msk, int, 0644);
module_param(musb_host_db_enable, bool, 0644);
module_param(musb_host_db_workaround1, bool, 0644);
module_param(musb_host_db_workaround2, bool, 0644);
module_param(musb_host_db_delay_ns, long, 0644);
module_param(musb_host_db_workaround_cnt, long, 0644);
module_param(mtk_host_audio_free_ep_udelay, int, 0644);
```

### module_param()支持的类型：

```C++
bool，invbool /*一个布尔型( true 或者 false)值(相关的变量应当是 int 类型). invbool 类型颠倒了值, 所以真值变成 false, 反之亦然.*/
charp/*一个字符指针值. 内存为用户提供的字串分配, 指针因此设置.*/
int，long，short
uint，ulong，ushort /*基本的变长整型值. 以 u 开头的是无符号值.*/
```

最后的 module_param 字段是一个权限值,表示此参数在sysfs文件系统中所对应的文件节点的属性。你应当使用 中定义的值. 这个值控制谁可以存取这些模块参数在 sysfs 中的表示.当perm为0时，表示此参数不存在 sysfs文件系统下对应的文件节点。 否则, 模块被加载后，在/sys/module/ 目录下将出现以此模块名命名的目录, 带有给定的权限。例如：

```log
PAYPHONEM50:/sys/module/musb_hdrc/parameters # cat musb_fake_CDP
0
```

### 其它衍生的方法

* 原型：module_param_array(name, type, nump, perm);传递数组

参数：

* name:既是用户看到的参数名，又是模块内接受参数的变量；
* type:表示参数的类型;
* nump:指针，指向一个整数，其值表示有多少个参数存放在数组name中。值得注意是name数组必须静态分配
* perm:指定了在sysfs中相应文件的访问权限;

例：

```
static int finsh[MAX_FISH];
static int nr_fish;
module_param_array（fish, int, &nr_fish, 0444）; //最终传递数组元素个数存在nr_fish中
```

* 原型：module_param_string(name, string, len, perm); 传递字符串

参数：

* name:既是用户看到的参数名，又是模块内接受参数的变量；;
* string:是内部的变量名
* nump:以string命名的buffer大小（可以小于buffer的大小，但是没有意义）
* perm:指定了在sysfs中相应文件的访问权限;

* 原型：module_param_cb(name, ops, arg, perm)； 这个支持你传进去一个参数处理的回调函数。

例：

```
static char species[BUF_LEN]；
module_param_string(specifies, species, BUF_LEN, 0);
```

* 原型：module_param_cb(name, ops, arg, perm)； 这个支持你传进去一个参数处理的回调函数。
  * @name: a valid C identifier which is the parameter name.
  * @ops: the set & get operations for this parameter.
  * @perm: visibility in sysfs.

1、name是你要定义的变量的名字
2、ops中指定你要用于set和get这个变量value的方法
3、arg是你在你的code中默认初始化的这个变量名
4、perm是系统读写等权限的设置 看一个实例你就全明白了


### 实例

```C++
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>


#define MAX_ARRAY 6

static int int_var = 0;
static const char *str_var = "default";
static int int_array[6];
int narr;

module_param(int_var, int, 0644);
MODULE_PARM_DESC(int_var, "A integer variable");


module_param(str_var, charp, 0644);
MODULE_PARM_DESC(str_var, "A string variable");

module_param_array(int_array, int, &narr, 0644);
MODULE_PARM_DESC(int_array, "A integer array");
 

static int __init hello_init(void)
{
       int i;
       printk(KERN_ALERT "Hello, my LKM.\n");
       printk(KERN_ALERT "int_var %d.\n", int_var);
       printk(KERN_ALERT "str_var %s.\n", str_var);

       for(i = 0; i < narr; i ++){
               printk("int_array[%d] = %d\n", i, int_array[i]);
       }
       return 0;
}

static void __exit hello_exit(void)
{
       printk(KERN_ALERT "Bye, my LKM.\n");
}
module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ydzhang");
MODULE_DEION("This module is a example.");
```

操作：

```
测试：insmod parm_hello.ko int_var=100 str_var=hello int_array=100,200
```

* module_param_cb实例如下：
```C++
static int set_musb_force_on(const char *val, const struct kernel_param *kp)
{
	int option;
	int rv;

	rv = kstrtoint(val, 10, &option);
	if (rv != 0)
		return rv;

	DBG(0, "musb_force_on:%d, option:%d\n", musb_force_on, option);
	if (option == 0 || option == 1) {
		DBG(0, "update to %d\n", option);
		musb_force_on = option;
	}

	switch (option) {
	case 2:
		DBG(0, "trigger reconnect\n");
		mt_usb_connect();
		break;
	case 3:
		DBG(0, "disable USB IRQ\n");
		disable_irq(mtk_musb->nIrq);
		break;
	case 4:
		DBG(0, "enable USB IRQ\n");
		enable_irq(mtk_musb->nIrq);
		break;
	default:
		break;
	}
	return 0;
}
static struct kernel_param_ops musb_force_on_param_ops = {
	.set = set_musb_force_on,
	.get = param_get_int,
};
module_param_cb(musb_force_on, &musb_force_on_param_ops, &musb_force_on, 0644);
```
这样一来，这段code会在开发板的sys/module/musb_hdrc/parameters/目录下创建一个名字叫musb_force_on的文件，你可以通过这个文件set和get变量enable的数据

操作
```log
PAYPHONEM50:/sys/module/musb_hdrc/parameters # echo 2 > musb_force_on
PAYPHONEM50:/sys/module/musb_hdrc/parameters # cat musb_force_on
1
打印：
[Fri Jun 24 07:49:29 2022] (6)[3259:sh][MUSB]set_musb_force_on 2965: musb_force_on:1, option:2
[Fri Jun 24 07:49:29 2022] (6)[3259:sh][MUSB]set_musb_force_on 2973: trigger reconnect
[Fri Jun 24 07:49:29 2022] (6)[3259:sh][MUSB]mt_usb_connect 715: [MUSB] USB connect
[Fri Jun 24 07:49:29 2022] (6)[3259:sh][MUSB]issue_connection_work 709: issue work, ops<2>
[Fri Jun 24 07:49:29 2022] (5)[6365:kworker/u16:2][MUSB]do_connection_work 614: is_host<0>, power<1>, ops<2>
[Fri Jun 24 07:49:29 2022] (5)[6365:kworker/u16:2]mt6370_pmu_charger mt6370_pmu_charger: mt6370_charger_get_online: online = 1
[Fri Jun 24 07:49:29 2022] (5)[6365:kworker/u16:2][MUSB]usb_cable_connected 582: online=1, type=1
[Fri Jun 24 07:49:29 2022] (5)[6365:kworker/u16:2][MUSB]cmode_effect_on 601: cable_mode=1, effect=0
[Fri Jun 24 07:49:29 2022] -(5)[6365:kworker/u16:2][MUSB]do_connection_work 675: do nothing, usb_on:1, power:1
```