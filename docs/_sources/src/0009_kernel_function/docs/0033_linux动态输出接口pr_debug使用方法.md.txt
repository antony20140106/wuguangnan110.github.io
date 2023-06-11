# 概述

在系统运行过程中，维护者可以通过控制 pr_debug 的开关来动态的配置某个模块中的调试信息是否输出，相对于 printk 来说，它显然是更加灵活。

# 参考

* [【内核】动态输出接口 pr_debug 使用方法](https://www.cnblogs.com/zackary/p/13951642.html)
* [uid-9672747-id-4535573.html Linux内核动态调试信息的打开pr_debug（基于按键驱动，全过程记录）](hhttp://blog.chinaunix.net/uid-9672747-id-4535573.html)
* [show-3058139.html?action=onClick 如何打开pr_debug调试信息](http://www.taodudu.cc/news/show-3058139.html?action=onClick%20如何打开pr_debug调试信息)

# 使用方法

 如内核`include/linux/printk.h`文件中定义：
 ```c
/* If you are writing a driver, please use dev_dbg instead */
#if defined(CONFIG_DYNAMIC_DEBUG) || \
    (defined(CONFIG_DYNAMIC_DEBUG_CORE) && defined(DYNAMIC_DEBUG_MODULE))
#include <linux/dynamic_debug.h>

/**
 * pr_debug - Print a debug-level message conditionally
 * @fmt: format string
 * @...: arguments for the format string
 *
 * This macro expands to dynamic_pr_debug() if CONFIG_DYNAMIC_DEBUG is
 * set. Otherwise, if DEBUG is defined, it's equivalent to a printk with
 * KERN_DEBUG loglevel. If DEBUG is not defined it does nothing.
 *
 * It uses pr_fmt() to generate the format string (dynamic_pr_debug() uses
 * pr_fmt() internally).
 */
#define pr_debug(fmt, ...)            \
    dynamic_pr_debug(fmt, ##__VA_ARGS__)
#elif defined(DEBUG)
#define pr_debug(fmt, ...) \
    printk(KERN_DEBUG pr_fmt(fmt), ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...) \
    no_printk(KERN_DEBUG pr_fmt(fmt), ##__VA_ARGS__)
#endif
 ```

可以看到通过配置可以分别实现有三种功能： 

1. 只有当`CONFIG_DYNAMIC_DEBUG`等宏已定义时，动态输出功能才会真正的启用，其核心是靠 dynamic_pr_debug 来实现，大概是通过将描述信息插入到 section("__dyndbg") 段内来进一步实现；
2. 在引用到 pr_debug 接口的某个文件或某个模块中，通过自定义一个`DEBUG`来配置它的第二种实现，这时候大家可以发现它就等同于`printk`，很直观；
3. 如果前两种配置都不成立，那么好了，`pr_debug`将不会有任何的输出。 

# 首先修改系统打印等级

默认的console级别是7（在kernel/printk/printk.c中定义了7）
* include/linux/printk.h:
```c
61:#define CONSOLE_LOGLEVEL_DEFAULT CONFIG_CONSOLE_LOGLEVEL_DEFAULT

wugn@jcrj-tf-compile:bengal_32go$ ack CONFIG_CONSOLE_LOGLEVEL_DEFAULT obj/KERNEL_OBJ/.config
CONFIG_CONSOLE_LOGLEVEL_DEFAULT=7
```

只有那些级别“小于7”的调试信息才能打印出来，而printk(KERN_DEBUG...)的级别是7，那就还需要提高console打印级别
如果要查看debug信息，那就直接改代码或者用命令`dmesg -n 8`

# 方案一 Makefile中定义DEBUG

测试源码：

* `pr_dbg.c`:
```c
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/printk.h>

static struct timer_list timer = {0};

void timer_handler(struct timer_list *t)
{
    pr_debug("pr_dbg: This is pr_init func.\n");
    mod_timer(&timer, jiffies+msecs_to_jiffies(5000));
}

static int pr_test_init(void)
{
    timer_setup(&timer, timer_handler, 0);
    timer.expires = jiffies + 5 * HZ;
    add_timer(&timer);

    return 0;
}

static int pr_init(void)
{
    pr_test_init();
    printk("pr_init exec finished.\n");

    return 0;
}

static void pr_exit(void)
{
    del_timer(&timer);
}

module_init(pr_init);
module_exit(pr_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zackary.Liu");
```

* makefile:
```makefile
kernel_path := /root/github/linux

all:
        make -C ${kernel_path} M=`pwd` modules
#KCFLAGS=-DDEBUG

clean:
        make -C ${kernel_path} M=`pwd` clean

obj-m += pr_dbg.o
```

1）先测试第二种配置吧，比较简单一些，如上面说过的只要定义一个 DEBUG 即可以实现，那么我们直接就在 Makefile 中加入好，

将编译指令改为`make -C ${kernel_path} M=`pwd` modules KCFLAGS+=-DDEBUG`即可。

将模块编译好并安装，可以看到有 pr_dbg: This is pr_init func. 信息输出。

# 方案二 CONFIG_DYNAMIC_DEBUG

如果定义了CONFIG_DYNAMIC_DEBUG，就使用动态debug机制dynamic_pr_debug();

kernel动态调试提供一个debugfs接口: `/sys/kernel/debug/dynamic_debug/control`这个文件可以用来获取已完成的调试信息列表:
```log
A6650:/ # cat /sys/kernel/debug/dynamic_debug/control | grep rouleur_dlkm
../../vendor/qcom/opensource/audio-kernel/asoc/codecs/rouleur/rouleur.c:343 [rouleur_dlkm]rouleur_global_mbias_disable =p "%s:mbias already disabled\012"
```

从源码上看cat出来的格式如下`filename:lineno [module]function flags format`：
* `lib/dynamic_debug.c`:
```c
/*
 * Seq_ops show method.  Called several times within a read()
 * call from userspace, with ddebug_lock held.  Formats the
 * current _ddebug as a single human-readable line, with a
 * special case for the header line.
 */
static int ddebug_proc_show(struct seq_file *m, void *p)
{
    struct ddebug_iter *iter = m->private;
    struct _ddebug *dp = p;
    struct flagsbuf flags;

    vpr_info("called m=%p p=%p\n", m, p);

    if (p == SEQ_START_TOKEN) {
        seq_puts(m,
             "# filename:lineno [module]function flags format\n"); //格式
        return 0;
    }

    seq_printf(m, "%s:%u [%s]%s =%s \"",
           trim_prefix(dp->filename), dp->lineno,
           iter->table->mod_name, dp->function,
           ddebug_describe_flags(dp->flags, &flags));
    seq_escape(m, dp->format, "\t\r\n\"");
    seq_puts(m, "\"\n");

    return 0;
}
```
其中`flags=p`表示打开调试信息，`flags=_`表示关闭。


* 打开文件rouleur.c所有调试信息:
```shell
nullarbor:~ # echo -n 'file rouleur.c +p' > /sys/kernel/debug/dynamic_debug/control
```

* 打开文件rouleur.c 746行调试信息，如果你想执行多个命令，你需要为每个加入`echo`分割，像这样：
```shell
nullarbor:~ # echo 'file rouleur.c line 746 +p' > /sys/kernel/debug/dynamic_debug/control \
> echo 'file rouleur.c line 662 +p' > /sys/kernel/debug/dynamic_debug/control
```

* 打开文件`wcd-mbhc*`所有调试信息:
```
echo "file wcd-mbhc* +p" > /sys/kernel/debug/dynamic_debug/control
```

* 打开`module rouleur_dlkm`所有调试信息：
```
echo "module rouleur_dlkm +p" > /sys/kernel/debug/dynamic_debug/control
```

* 打开多个module可以使用bat脚本：
```shell
@echo off
adb wait-for-device
adb shell "echo "module swr_ctrl_dlkm +p" > /sys/kernel/debug/dynamic_debug/control"
adb shell "echo "module machine_dlkm +p" > /sys/kernel/debug/dynamic_debug/control"
adb shell "echo "module rouleur_dlkm +p" > /sys/kernel/debug/dynamic_debug/control"
adb shell "echo "module  aw87xxx_dlkm +p" > /sys/kernel/debug/dynamic_debug/control"
adb shell "echo "module mbhc_dlkm +p" > /sys/kernel/debug/dynamic_debug/control"
pause
```

file可以替换成module,format等匹配方式，具体用法请参考`Documentation/admin-guide/dynamic-debug-howto.rst`:
```shell
Examples
========

::

  // enable the message at line 1603 of file svcsock.c
  nullarbor:~ # echo -n 'file svcsock.c line 1603 +p' >
                <debugfs>/dynamic_debug/control

  // enable all the messages in file svcsock.c
  nullarbor:~ # echo -n 'file svcsock.c +p' >
                <debugfs>/dynamic_debug/control

  // enable all the messages in the NFS server module
  nullarbor:~ # echo -n 'module nfsd +p' >
                <debugfs>/dynamic_debug/control

  // enable all 12 messages in the function svc_process()
  nullarbor:~ # echo -n 'func svc_process +p' >
                <debugfs>/dynamic_debug/control

  // disable all 12 messages in the function svc_process()
  nullarbor:~ # echo -n 'func svc_process -p' >
                <debugfs>/dynamic_debug/control

  // enable messages for NFS calls READ, READLINK, READDIR and READDIR+.
  nullarbor:~ # echo -n 'format "nfsd: READ" +p' >
                <debugfs>/dynamic_debug/control

  // enable messages in files of which the paths include string "usb"
  nullarbor:~ # echo -n '*usb* +p' > <debugfs>/dynamic_debug/control

  // enable all messages
  nullarbor:~ # echo -n '+p' > <debugfs>/dynamic_debug/control

  // add module, function to all enabled messages
  nullarbor:~ # echo -n '+mf' > <debugfs>/dynamic_debug/control

  // boot-args example, with newlines and comments for readability
  Kernel command line: ...
    // see whats going on in dyndbg=value processing
    dynamic_debug.verbose=1
    // enable pr_debugs in 2 builtins, #cmt is stripped
    dyndbg="module params +p #cmt ; module sys +p"
    // enable pr_debugs in 2 functions in a module loaded later
    pc87360.dyndbg="func pc87360_init_device +p; func pc87360_find +p"                     
```