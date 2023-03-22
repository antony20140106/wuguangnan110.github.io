# 概述

qcom qcm2290 TypeC 死机调试过程。

# 参考


# 分析过程

当换成typec通知xxx_charger进行vbus和plug状态时，发现机器拔插还是容易死机。如下是插入host时直接挂了：
```log
console:/ $ [   46.908242] xxx-pd-manager soc:xxx_pd_manager: pd_tcp_notifier_call event = 12

console:/ $ [   47.130891] xxx-pd-manager soc:xxx_pd_manager: pd_tcp_notifier_call event = 9
[   47.138396] xxx-pd-manager soc:xxx_pd_manager: pd_tcp_notifier_call source vbus 0mV
[   47.146293] xxx-pd-manager soc:xxx_pd_manager: pd_tcp_notifier_call - source vbus 0v output
[   47.154859] set vbus status: 0

[   47.269901] xxx-pd-manager soc:xxx_pd_manager: pd_tcp_notifier_call event = 10
[   47.277482] xxx-pd-manager soc:xxx_pd_manager: pd_tcp_notifier_call sink vbus 5000mV 100mA type(0x01)
```

发现typec端是非租塞的srcu通知，而charger注册的是blocking阻塞式的，如下：
```C++
1.typec通知：
#ifdef CONFIG_TCPC_NOTIFICATION_NON_BLOCKING
struct tcp_notify_work {
	struct work_struct work;
	struct tcpc_device *tcpc;
	struct tcp_notify tcp_noti;
	uint8_t type;
	uint8_t state;
};

static void tcp_notify_func(struct work_struct *work)
{
	struct tcp_notify_work *tn_work =
		container_of(work, struct tcp_notify_work, work);
	struct tcpc_device *tcpc = tn_work->tcpc;
	struct tcp_notify *tcp_noti = &tn_work->tcp_noti;
	uint8_t type = tn_work->type;
	uint8_t state = tn_work->state;
#ifdef CONFIG_PD_BEGUG_ON
	struct timeval begin, end;
	int timeval = 0;

	do_gettimeofday(&begin);
	srcu_notifier_call_chain(&tcpc->evt_nh[type], state, tcp_noti);
	do_gettimeofday(&end);
	timeval = (timeval_to_ns(end) - timeval_to_ns(begin))/1000/1000;
	PD_BUG_ON(timeval > TCPC_NOTIFY_OVERTIME);
#else
	srcu_notifier_call_chain(&tcpc->evt_nh[type], state, tcp_noti);
#endif

	kfree(tn_work);
}

static int tcpc_check_notify_time(struct tcpc_device *tcpc,
	struct tcp_notify *tcp_noti, uint8_t type, uint8_t state)
{
	struct tcp_notify_work *tn_work;

	tn_work = kzalloc(sizeof(*tn_work), GFP_KERNEL);
	if (!tn_work)
		return -ENOMEM;

	INIT_WORK(&tn_work->work, tcp_notify_func);
	tn_work->tcpc = tcpc;
	tn_work->tcp_noti = *tcp_noti;
	tn_work->type = type;
	tn_work->state = state;

	return queue_work(tcpc->evt_wq, &tn_work->work) ? 0 : -EAGAIN;
}

xxx通知：
static BLOCKING_NOTIFIER_HEAD(mp2721_notify_list);

/**
 *  mp2721_notify_register_client - register a client notifier
 *  @nb: notifier block to callback on events
 */
int mp2721_notify_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&mp2721_notify_list, nb);
}
EXPORT_SYMBOL(mp2721_notify_register_client);

/**
 *  mp2721_notify_unregister_client - unregister a client notifier
 *  @nb: notifier block to callback on events
 */
int mp2721_notify_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&mp2721_notify_list, nb);
}
EXPORT_SYMBOL(mp2721_notify_unregister_client);

/**
 * mp2721_notify_call_chain - notify clients of mp2721_notify_events
 *
 */
int mp2721_notify_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&mp2721_notify_list, val, v);
}
EXPORT_SYMBOL_GPL(mp2721_notify_call_chain);
```

## 1.尝试换成非租塞的notify

* 修改如下：
```C++
struct srcu_notifier_head mp2721_notify_list;

/**
 *  mp2721_notify_register_client - register a client notifier
 *  @nb: notifier block to callback on events
 */
int mp2721_notify_register_client(struct notifier_block *nb)
{
	return srcu_notifier_chain_register(&mp2721_notify_list, nb);
}
EXPORT_SYMBOL(mp2721_notify_register_client);

/**
 *  mp2721_notify_unregister_client - unregister a client notifier
 *  @nb: notifier block to callback on events
 */
int mp2721_notify_unregister_client(struct notifier_block *nb)
{
	return srcu_notifier_chain_unregister(&mp2721_notify_list, nb);
}
EXPORT_SYMBOL(mp2721_notify_unregister_client);

/**
 * mp2721_notify_call_chain - notify clients of mp2721_notify_events
 *
 */
int mp2721_notify_call_chain(unsigned long val, void *v)
{
	return srcu_notifier_call_chain(&mp2721_notify_list, val, v);
}
EXPORT_SYMBOL_GPL(mp2721_notify_call_chain);
```

开机导致typec通知直接挂了，原因是因为不是阻塞的，charger那边还没注册的，typec直接发送，造成空指针。
```log
[    7.826346] __tcpc_class_complete_work = type_c_port0
[    7.837427] ///PD dbg info 41d
[    7.840505] <    7.837>TCPC-TYPEC:typec_init: TrySRC
[    7.841025] tcpc_device_irq_enable : tcpc irq enable OK!
[    7.843973] husb311_set_low_power_mode - write HUSB311_REG_BMC_CTRL=0x7
[    7.844142] xxx-pd-manager soc:xxx_pd_manager: pd_tcp_notifier_call event = 12
[    7.865011] ALSA device list:
[    7.867992]   No soundcards found.
[    7.868093] ///PD dbg info 250d
[    7.874559] <    7.838>TCPC-TYPEC:PowerOffCharge
[    7.874559] <    7.838>TCPC-TYPEC:** Unattached.SNK
[    7.874559] <    7.841>TCPC-TCPC:Alert:0x0001, Mask:0x21067f
[    7.874559] <
[    7.874561]    7.842>TCPC-TCPC:tcpci_alert_cc_changed ++
[    7.874561] <    7.842>TCPC-TYPEC:[CC_Alert] 5/0
[    7.874561] <    7.844>TCPC-TYPEC:** AttachWait.SNK
[    7.908197] Freeing unused kernel memory: 4992K
[    7.912796] Run /init as init process
[    7.920971] init: init first stage started!
[    7.925453] init: Unable to open /lib/modules, skipping module loading.
[    7.932831] init: [libfs_mgr]dt_fstab: Skip disabled entry for partition vendor
[    7.940238] init: [libfs_mgr]ReadFstabFromDt(): failed to read fstab from dt
[    7.947561] init: [libfs_mgr]dt_fstab: Skip disabled entry for partition vendor
[    7.956325] init: Using Android DT directory /proc/device-tree/firmware/android/
[    7.964197] ///PD dbg info 71d
[    7.967259] <    7.964>TCPC-TYPEC:[CC_Change] 5/0
[    7.967259] <    7.964>TCPC-TYPEC:** Try.SRC
[    8.000102] ///PD dbg info 266d
[    8.003262] <    7.964>TCPC-TYPEC:Attached-> NULL(repeat)
[    8.003262] <    7.966>TCPC-TCPC:Alert:0x0001, Mask:0x23067f
[    8.003262] <    7.966>TCPC-TCPC:tcpci_alert_
[    8.003264] cc_changed ++
[    8.003264] <    7.967>TCPC-TYPEC:[CC_Alert] 0/0
[    8.003264] <    7.967>TCPC-TYPEC:[Warning] CC No Res
[    8.003264] <    7.967>TCPC-TYPEC:[Try] Ignore
[    8.019163] cc_detach
[    8.066502] ///PD dbg info 75d
[    8.069785] xxx-pd-manager soc:xxx_pd_manager: pd_tcp_notifier_call event = 9
[    8.076980] xxx-pd-manager soc:xxx_pd_manager: pd_tcp_notifier_call source vbus 0mV
[    8.084780] <    8.066>TCPC-TYPEC:[CC_Change] 0/0
[    8.084780] <    8.066>TCPC-TYPEC:** TryWait.SNK
[    8.084849] xxx-pd-manager soc:xxx_pd_manager: pd_tcp_notifier_call - source vbus 0v output
[    8.102495] set_vbus_status status 0
[    8.106105] Unable to handle kernel read from unreadable memory at virtual address 0000003a20942000
[    8.114678] init: [libfs_mgr]superblock s_max_mnt_count:65535,/dev/block/by-name/metadata
[    8.115193] Mem abort info:
[    8.116069] ///PD dbg info 224d
[    8.116071] <    8.069>TCPC-TCPC:source_vbus: 0 mV, 0 mA
[    8.116071] <    8.069>TCPC-TYPEC:Attached-> NULL(repeat)
[    8.116071] <    8.094>TCPC-TCPC:Alert:0x0001, Ma
[    8.116073] sk:0x23067f
[    8.116073] <    8.095>TCPC-TCPC:tcpci_alert_cc_changed ++
[    8.116073] <    8.096>TCPC-TYPEC:[CC_Alert] 5/0
[    8.123487] init: [libfs_mgr]Filesystem on /dev/block/by-name/metadata was not cleanly shutdown; state flags: 0x1, incompat feature flags: 0x46
[    8.126211]   ESR = 0x96000005
[    8.129941] init: [libfs_mgr]check_fs(): mount(/dev/block/by-name/metadata,/metadata,ext4)=-1: No such file or directory
[    8.144794]   Exception class = DABT (current EL), IL = 32 bits
[    8.157445] init: [libfs_mgr]Running /system/bin/e2fsck on /dev/block/mmcblk0p72
[    8.170234]   SET = 0, FnV = 0
[    8.175665] e2fsck: e2fsck 1.45.4 (23-Sep-2019)
[    8.184218]   EA = 0, S1PTW = 0
[    8.184220] Data abort info:
[    8.184221]   ISV = 0, ISS = 0x00000005
[    8.184223]   CM = 0, WnR = 0
[    8.184225] [0000003a20942000] user address but active_mm is swapper
[    8.184229] Internal error: Oops: 96000005 [#1] PREEMPT SMP
[    8.184233] Modules linked in:
[    8.184243] Process kworker/u8:8 (pid: 240, stack limit = 0xffffff8009d18000)
[    8.194053] e2fsck: /dev/block/by-name/metadata: recovering journal
[    8.197600] CPU: 3 PID: 240 Comm: kworker/u8:8 Tainted: G        W         4.19.157 #1
[    8.197602] Hardware name: Qualcomm Technologies, Inc. Scuba IOT IDP (DT)
[    8.197612] Workqueue: type_c_port0 tcp_notify_func
[    8.197616] pstate: 80400005 (Nzcv daif +PAN -UAO)
[    8.197624] pc : __srcu_read_lock+0x34/0x78
[    8.197627] lr : __srcu_read_lock+0x20/0x78
[    8.197628] sp : ffffff8009d1bc00
[    8.197629] x29: ffffff8009d1bc00 x28: ffffffe75361b030
[    8.197632] x27: 00000000ffffffff x26: 0000000000000000
[    8.197635] x25: ffffffe81cf10e80 x24: ffffffae1e940e22
[    8.197638] x23: 0000000000000000 x22: 0000000040046201
[    8.197641] x21: ffffff8009d1bca0 x20: ffffffae1fc47028
[    8.197646] x19: 0000000000000000 x18: 0000000000000387
[    8.216874] ///PD dbg info 76d
[    8.218139] x17: 000000000115b8a8 x16: 0000000000004e20
[    8.218142] x15: 0000000000000387 x14: 0000000000000001
[    8.218145] x13: 0000000000000000 x12: 0000000000000001
[    8.218147] x11: 0000000100000000 x10: 0000000000000000
[    8.218150] x9 : 0000003a20942000 x8 : 0000003a20942000
[    8.218153] x7 : 6174735f73756276 x6 : ffffffae1f403618
[    8.218156] x5 : 0000000000000000 x4 : ffffffe81e01be00
[    8.218159] x3 : 0000000000000000 x2 : ffffff8009d1bca0
[    8.218162] x1 : 0000000040046201 x0 : 0000000000000001
[    8.218165] Call trace:
[    8.218173]  __srcu_read_lock+0x34/0x78
[    8.218180]  srcu_notifier_call_chain+0x34/0xd8
[    8.218184]  mp2721_notify_call_chain+0x1c/0x28
[    8.218192]  pd_tcp_notifier_call+0x6c8/0x900
[    8.224640] <    8.216>TCPC-TYPEC:[CC_Change] 5/0
[    8.224640] <    8.216>TCPC-TYPEC:** Attached.SNK
[    8.230139]  tcp_notifier_func_stub+0x14/0x20
[    8.230150]  srcu_notifier_call_chain+0x90/0xd8
[    8.230152]  tcp_notify_func+0x34/0x48
[    8.230156]  process_one_work+0x278/0x468
[    8.230159]  worker_thread+0x238/0x4c8
[    8.230161]  kthread+0x148/0x158
[    8.230165]  ret_from_fork+0x10/0x1c
[    8.230171] Code: f940a288 d538d089 8b130d08 8b080128 (c85f7d0a)
[    8.230174] ---[ end trace 17af4f9bea3460d1 ]---
[    8.230258] note: kworker/u8:8[240] exited with preempt_count 1
[    8.230496] kworker/u8:8 (240) used greatest stack depth: 12128 bytes left
[    8.234209] e2fsck: Pass 1: Checking inodes, blocks, and sizes
[    8.256084] ///PD dbg info 158d
[    8.261870] e2fsck: Pass 2: Checking directory structure
[    8.266624] <    8.233>TCPC-TCPC:wake_lock=1
[    8.266624] <    8.233>TCPC-TCPC:sink_vbus: 5000 mV, 100 mA
[    8.266624] <    8.233>TCPC-TYPEC:Attached-> SINK
[    8.266624] <    8.23
[    8.266625] 3>TCPC-TCPC:usb_port_attached
```