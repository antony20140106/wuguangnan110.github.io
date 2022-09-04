# 概述

Android kernel经常需要用到一些内核API，总结以下用过的。

# 参考

* [linux 内核API总结](https://www.cnblogs.com/bobfly1984/p/14596763.html)
* [Linux内核API|极客笔记 (deepinout.com)](https://deepinout.com/linux-kernel-api)
* [主线剧情07.3-Linux驱动编程-内核态API总结备查](https://blog.csdn.net/Staokgo/article/details/125126579)

# 内容

```C++
common API
sprintf
snprintf
sscanf
strcmp
strncmp
strcpy
strlcpy
kstrtol
strtol
__raw_writel
__raw_readl
memset
memcpy
memcmp
container_of
udelay
msleep
printk
poll_wait	实现IO多路复用
__get_user
__put_user
iminor

find_first_zero_bit
set_bit
BUG_ON
ARRAY_SIZE
proc_create
print_hex_dump_bytes	缓冲区16进制dump 输出
kstrtoul
IS_ERR
set_bit
test_bit
clear_bit
devm 系列函数 devres.txt
kthread_create

bitmap 操作
DECLARE_BITMAP
for_each_set_bit

debugfs API
debugfs_create_dir

list链表 API
list_for_each_entry
INIT_LIST_HEAD
LIST_HEAD
list_add
list_del
list_empty
list_entry
list_first_entry

spi API
spi_register_driver
struct spi_driver
spi_set_drvdata
struct spi_transfer
struct spi_message
spi_message_init
spi_message_add_tail
spi_async
spi_sync
spi_setup

uart API
struct uart_driver
uart_register_driver
struct uart_ops
struct uart_port
uart_add_one_port
struct ktermios

thread API
struct task_struct

char device API
struct file_operations
register_chrdev
unregister_chrdev
MKDEV
MAJOR
find_first_zero_bit
cdev_alloc
cdev_add
cdev_del
cdev_init
alloc_chrdev_region
unregister_chrdev_region

设备模型API
platform_driver_register
platform_device_register_simple
platform_driver_unregister
to_platform_device
platform_set_drvdata
class_create
class_destroy
device_create	// 上报uevent事件
kobject_uevent_env / kobject_uevent	// 上报uevent事件
device_create_file

memory API
devm_kzalloc
ioremap
kzalloc
kfree

regulator API
devm_regulator_get
regulator_enable
regulator_disable

device tree OF API
struct device_node
of_get_child_count
for_each_child_of_node
of_find_property
of_get_named_gpio
of_get_gpio_flags 解析gpios属性：gpios = <&tlmm_pinmux 49 0>;由gpio controller driver翻译
of_get_property
of_property_read_u32
of_property_read_bool
of_alias_get_id 根据传入到device node在 aliaes 节点找到唯一的编号
of_platform_populate 为root节点下的所有子节点创建platform device
platform_get_resource_byname
platform_get_irq_byname
clk_get
clk_set_rate
clk_prepare_enable

pinctrl API
devm_pinctrl_get
pinctrl_lookup_state
pinctrl_select_state

gpio API
devm_gpio_request_one
devm_gpio_free
gpio_request
gpio_free
gpio_set_value_cansleep
gpio_get_value_cansleep
gpio_export
gpio_direction_output
gpio_direction_input
gpio_set_value
gpio_get_value
gpio_is_valid

IRQ API
gpio_to_irq
devm_request_any_context_irq
local_irq_disable
local_irq_enable
platform_get_irq_byname
devm_request_irq
request_irq
request_threaded_irq
free_irq
enable_irq
disable_irq
enable_irq_wake
disable_irq_wake
irq_read_line

power_supply API
power_supply_get_by_name
power_supply_set_supply_type
power_supply_set_present

sysfs API
sysfs_create_file 创建属性文件
sysfs_remove_file
sysfs_create_group	创建属性组
sysfs_remove_group

wakelock API
wake_lock_init
wake_lock_timeout
wake_unlock
wake_lock_active
wake_lock
msecs_to_jiffies

workqueue API
create_singlethread_workqueue
INIT_WORK
INIT_DELAYED_WORK
queue_work
queue_delayed_work
cancel_delayed_work_sync
struct work_struct
alloc_workqueue

waitqueue API
DECLARE_WAIT_QUEUE_HEAD
wait_queue_head_t
init_waitqueue_head
wake_up_interruptible
wake_up
wait_event_interruptible
wait_event_timeout

互斥API
DEFINE_MUTEX
mutex_lock
mutex_unlock
mutex_init

spinlock_t
spin_lock_init
spin_lock_irqsave
spin_lock_irq
spin_unlock_irq

atomic_t
atomic_read
atomic_set
atomic_dec
atomic_dec_and_test

timer API
struct timer_list
mod_timer
msecs_to_jiffies
del_timer
setup_timer
add_timer

内核线程 API
struct kthread_worker
struct kthread_work
init_kthread_worker
kthread_run(kthread_worker_fn)
init_kthread_work
queue_kthread_work
flush_kthread_worker
kthread_should_stop

hrtimer API
struct hrtimer
hrtimer_start

通知链机制 API
// 参考/kernel/kernel/power/main.c PM core实现的通知链
struct notifier_block
blocking_notifier_chain_register
BLOCKING_NOTIFIER_HEAD

内核空间 文件操作 API
struct file
filp_open
get_fs
set_fs
调用文件指针的操作接口： f->f_op->write/read/unlocked_ioctl
filp_close

complete 接口API
struct completion
complete_all
wait_for_completion_interruptible_timeout
wait_for_completion
init_completion
complete

time API
struct timespec
struct rtc_time
getnstimeofday
rtc_time_to_tm
```