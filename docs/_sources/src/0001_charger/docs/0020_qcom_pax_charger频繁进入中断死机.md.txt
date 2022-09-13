# 概述

目前charger由于插电脑未限制充电电流，开机过程中插usb会频繁进入中断，而且容易导致死机。

# 参考

* [Linux Kernel编程 --- 设备驱动中的并发控制](https://blog.csdn.net/u010961173/article/details/90664584)
* [48253621 下半部和下半部执行的工作--工作队列](https://blog.csdn.net/hongbochen1223/article/details/48253621%20下半部和下半部执行的工作--工作队列)
* [中断与抢占 #10](https://github.com/leon0625/blog/issues/10)

# 中断原理

中断会打断内核的正常进程调度，所以尽量短小精悍。不过实际系统，中断中要的事又比较多。 为了解决这一矛盾，Linux把中断分为两部分：

* 顶半部，top half，紧急且量小任务，一般为读取寄存器中的中断状态，并清除中断标记。总之，完成必要的硬件操作。处于中断上下文，不可被打断。
* 底半部，bottom half，完成主要任务，非中断上下文，可以被打断。

![0020_0001.png](images/0020_0001.png)

注:不一定所有的中断都分两部分，如果要干的事很少，完全可以不要底半部。

linux查看中断统计信息的方法：cat /proc/interrupts文件

# 中断代码

代码是参考
```C++
static int mp2721_charger_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	ret = gpio_request(mp2721_info->irq_pin, "mp2721_info irq pin");
	if (ret) {
		chr_mp2721_err("%s: %d gpio request failed\n", __func__, mp2721_info->irq_pin);
		goto probe_fail_0;
	}
	gpio_direction_input(mp2721_info->irq_pin);

	irqn = gpio_to_irq(mp2721_info->irq_pin);
	if (irqn < 0) {
		chr_mp2721_err("%s:%d gpio_to_irq failed\n", __func__, irqn);
		ret = irqn;
		goto err_1;
	}
	client->irq = irqn;

	INIT_WORK(&mp2721_info->irq_work, mp2721_charger_irq_workfunc);
	mp2721_info->chg_workqueue = create_singlethread_workqueue("mp2721_thread");

	ret = sysfs_create_group(&mp2721_info->psy->dev.kobj, &mp2721_attr_group);
	if (ret) {
		chr_mp2721_err("failed to register sysfs. err: %d\n", ret);
		goto err_sysfs_create;
	}

	ret = request_irq(client->irq, mp2721_charger_interrupt, IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "mp2721_charger_irq", mp2721_info);
	if (ret) {
		chr_mp2721_err("%s:Request IRQ %d failed: %d\n", __func__, client->irq, ret);
		goto err_irq;
	} else {
		chr_mp2721_info("%s:irq = %d\n", __func__, client->irq);
	}

	queue_work(mp2721_info->chg_workqueue, &mp2721_info->irq_work);/*in case of adapter has been in when power off*/

}
```
# 分析过程

![0020_0000.png](images/0020_0000.png)

当快速频繁中断发生后，会有多个CPU存在工作队列线程在工作，对于共享资源就没有同步机制，那解决问题的办法就是保证对共享资源的互斥访问，访问共享资源的代码区称为临界区，需要对临界区进行互斥保护。所谓互斥访问是指一个执行单元访问共享资源的时候，其他执行单元被禁止；

# 解决方案：中断屏蔽

 中断屏蔽，就是关闭当前core的中断，实际是屏蔽CPSR的I位。也就保证了临界区不会受中断的影响。调度是依赖中断的，中断屏蔽以后，整个core的任务调度都不进行了，自然就不会有进程抢占了。 所以，被包含的临界区应该尽量短，以下是中断操作函数：
```C++
1. 单个使能/屏蔽
extern void disable_irq_nosync(unsigned int irq);    // 立即返回
extern void disable_irq(unsigned int irq);        // 等待中断处理完成后返回
extern void enable_irq(unsigned int irq);   


2.全局使能屏蔽
#define local_irq_enable()         // 全局使能
#define local_irq_disable()         // 全局关闭
#define local_irq_save(flags)        // 全局关闭，同时保存目前中断状态到flag，flag时unsigned long，不是指针
#define local_irq_restore(flags)    // 全局使能，恢复中断状态
```

# 修改方案

* `mp2721_charger_interrupt`是中断上半段，`mp2721_charger_irq_workfunc`是下半段。
```C++
/* mp2721 irq list start */
void mp2721_irq_enable(void)
{
    unsigned long flags = 0;

    if (g_mp2721_info->mp2721_irq_is_disable) {
		spin_lock_irqsave(&g_mp2721_info->mp2721_irq_lock, flags);
		chr_mp2721_err("%s:\n", __func__);
        enable_irq(g_mp2721_info->client->irq);
        g_mp2721_info->mp2721_irq_is_disable = 0;
		spin_unlock_irqrestore(&g_mp2721_info->mp2721_irq_lock, flags);
    }
}


void mp2721_irq_disable(void)
{
    unsigned long flags;

    if (!g_mp2721_info->mp2721_irq_is_disable) {
		spin_lock_irqsave(&g_mp2721_info->mp2721_irq_lock, flags);
		chr_mp2721_err("%s:\n", __func__);
        g_mp2721_info->mp2721_irq_is_disable = 1;
        disable_irq_nosync(g_mp2721_info->client->irq);
		spin_unlock_irqrestore(&g_mp2721_info->mp2721_irq_lock, flags);
    }
}

static void mp2721_charger_irq_workfunc(struct work_struct *work)
{
	int last_charge_status = 0;

	mp2721_irq_disable();
	chr_mp2721_err("%s:mp2721 irq\n", __func__);

	g_mp2721_info->charge_fault = _get_fault_status();
	if (g_mp2721_info->charge_fault) {
		pr_err("charger fault = 0x%x\n", g_mp2721_info->charge_fault);
	}
	last_charge_status = _get_charge_status();
	if (g_mp2721_info->charge_status != last_charge_status) {
		g_mp2721_info->charge_status = last_charge_status;
		pax_charger_update();
	}
    mp2721_irq_enable();
}

static irqreturn_t mp2721_charger_interrupt(int irq, void *data)
{
	queue_work(g_mp2721_info->chg_workqueue, &g_mp2721_info->irq_work);
	return IRQ_HANDLED;
}
/* mp2721 irq list end */
```