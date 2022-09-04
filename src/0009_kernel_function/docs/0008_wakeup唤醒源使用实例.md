# 概述

notify实例

# 参考

* [电源管理](https://www.jianshu.com/p/92591a82486a?utm_campaign=maleskine&utm_content=note&utm_medium=seo_notes&utm_source=recommendation)

# pm_stay_awake()

当设备有wakeup event正在处理时，需要调用该接口通知PM core，该接口的实现如下`kernel/drivers/base/power/wakeup.c`：

```C++
void pm_stay_awake(struct device *dev)
{
    unsigned long flags;

    if (!dev)
        return;

    spin_lock_irqsave(&dev->power.lock, flags);
    __pm_stay_awake(dev->power.wakeup);
    spin_unlock_irqrestore(&dev->power.lock, flags);
}

void __pm_stay_awake(struct wakeup_source *ws)
{
    unsigned long flags;

    if (!ws)
        return;

    spin_lock_irqsave(&ws->lock, flags);

    wakeup_source_report_event(ws);
    del_timer(&ws->timer);
    ws->timer_expires = 0;

    spin_unlock_irqrestore(&ws->lock, flags);
}

static void wakeup_source_report_event(struct wakeup_source *ws)
{
    ws->event_count++;
    /* This is racy, but the counter is approximate anyway. */
    if (events_check_enabled)
        ws->wakeup_count++;

    if (!ws->active)
        wakeup_source_activate(ws);
}
```

wakeup_source_report_event中增加wakeup source的event_count次数，即表示该source又产生了一个event。然后根据events_check_enabled变量的状态，增加wakeup_count。如果wakeup source没有active，则调用wakeup_source_activate进行activate操作。

```C++
/*
 * The functions below use the observation that each wakeup event starts a
 * period in which the system should not be suspended.  The moment this period
 * will end depends on how the wakeup event is going to be processed after being
 * detected and all of the possible cases can be divided into two distinct
 * groups.
 *
 * First, a wakeup event may be detected by the same functional unit that will
 * carry out the entire processing of it and possibly will pass it to user space
 * for further processing.  In that case the functional unit that has detected
 * the event may later "close" the "no suspend" period associated with it
 * directly as soon as it has been dealt with.  The pair of pm_stay_awake() and
 * pm_relax(), balanced with each other, is supposed to be used in such
 * situations.
 *
 * Second, a wakeup event may be detected by one functional unit and processed
 * by another one.  In that case the unit that has detected it cannot really
 * "close" the "no suspend" period associated with it, unless it knows in
 * advance what's going to happen to the event during processing.  This
 * knowledge, however, may not be available to it, so it can simply specify time
 * to wait before the system can be suspended and pass it as the second
 * argument of pm_wakeup_event().
 *
 * It is valid to call pm_relax() after pm_wakeup_event(), in which case the
 * "no suspend" period will be ended either by the pm_relax(), or by the timer
 * function executed when the timer expires, whichever comes first.
 */

/**
 * wakup_source_activate - Mark given wakeup source as active.
 * @ws: Wakeup source to handle.
 *
 * Update the @ws' statistics and, if @ws has just been activated, notify the PM
 * core of the event by incrementing the counter of of wakeup events being
 * processed.
 */
static void wakeup_source_activate(struct wakeup_source *ws)
{
	unsigned int cec;

	if (WARN_ONCE(wakeup_source_not_registered(ws),
			"unregistered wakeup source\n"))
		return;

	ws->active = true;
	ws->active_count++;
	ws->last_time = ktime_get();
	if (ws->autosleep_enabled)
		ws->start_prevent_time = ws->last_time;

	/* Increment the counter of events in progress. */
	cec = atomic_inc_return(&combined_event_count);

	trace_wakeup_source_activate(ws->name, cec);
}
```

wakeup_source_activate中首先调用freeze_wake，将系统从suspend to freeze状态下唤醒，然后设置active标志，增加active_count，更新last_time。如果使能了autosleep，更新start_prevent_time，此刻该wakeup source会开始阻止系统auto sleep。增加“wakeup events in progress”计数，增加该计数意味着系统正在处理的wakeup event数目不为零，即系统不能suspend。

# pm_relax()
pm_relax和pm_stay_awake成对出现，用于在wakeup event处理结束后通知PM core，其实现如下

* `kernel/drivers/base/power/wakeup.c`:

```C++
void pm_relax(struct device *dev)
{
    unsigned long flags;

    if (!dev)
        return;

    spin_lock_irqsave(&dev->power.lock, flags);
    __pm_relax(dev->power.wakeup);
    spin_unlock_irqrestore(&dev->power.lock, flags);
}

void __pm_relax(struct wakeup_source *ws)
{
    unsigned long flags;

    if (!ws)
        return;

    spin_lock_irqsave(&ws->lock, flags);
    if (ws->active)
        wakeup_source_deactivate(ws);
    spin_unlock_irqrestore(&ws->lock, flags);
}
```
pm_relax中直接调用pm_relax，pm_relax判断wakeup source如果处于active状态，则调用wakeup_source_deactivate接口，deactivate该wakeup source

# __pm_wakeup_event

* [Linux电源管理总体框架及实现原理](https://www.elecfans.com/emb/202009141297122.html)

关于wakeup event framework核心功能

1. __pm_stay_awake ： wakeup source 切换为acTIve状态的接口

2. __pm_relax： wakeup source 切换为disacTIve状态的接口

3. __pm_wakeup_event： 上边两个接口的结合体，引入了时间控制

```c++
/**
 * __pm_wakeup_event - Notify the PM core of a wakeup event.
 * @ws: Wakeup source object associated with the event source.
 * @msec: Anticipated event processing time (in milliseconds).预期事件处理时间
 *
 * Notify the PM core of a wakeup event whose source is @ws that will take 内核处理大约需要@msec 毫秒
 * approximately @msec milliseconds to be processed by the kernel.  If @ws is
 * not active, activate it.  If @msec is nonzero, set up the @ws' timer to  
 * execute pm_wakeup_timer_fn() in future. 如果@ws 未激活，请激活它。 如果@msec 不为零，则设置@ws 的计时器以在将来执行 pm_wakeup_timer_fn()。
 *
 * It is safe to call this function from interrupt context.
 */
void __pm_wakeup_event(struct wakeup_source *ws, unsigned int msec)
{
        unsigned long flags;
        unsigned long expires;

        if (!ws)
                return;

        spin_lock_irqsave(&ws->lock, flags);

        wakeup_source_report_event(ws);

        if (!msec) {
                wakeup_source_deactivate(ws);
                goto unlock;
        }

        expires = jiffies + msecs_to_jiffies(msec);
        if (!expires)
                expires = 1;

        if (!ws->timer_expires || time_after(expires, ws->timer_expires)) {
                mod_timer(&ws->timer, expires);
                ws->timer_expires = expires;
        }

 unlock:
        spin_unlock_irqrestore(&ws->lock, flags);
}
EXPORT_SYMBOL_GPL(__pm_wakeup_event);
```

# 历程

以下是基于高通qcm2290平台kernel-4.19实现的。

```C++
#include <linux/pm_wakeup.h>

static int mp2721_read_byte(struct mp2721 *mp, u8 *data, u8 reg)
{
	int ret;

	pm_stay_awake(mp->dev);
	mutex_lock(&mp2721_i2c_lock);
	ret = i2c_smbus_read_byte_data(mp->client, reg);
	if (ret < 0) {
		chr_err("failed to read 0x%.2x\n", reg);
		mutex_unlock(&mp2721_i2c_lock);
		return ret;
	}

	*data = (u8)ret;
	mutex_unlock(&mp2721_i2c_lock);
	pm_relax(mp->dev);

	return 0;
}
```