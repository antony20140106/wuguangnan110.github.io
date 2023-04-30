# README

不少同事发现AVC每次都打印不出来，分析一下。

# 参考

* [register_pernet_subsys相关学习](https://blog.csdn.net/lickylin/article/details/18013879)
* [Android10.0 日志系统分析(二)-logd、logcat架构分析及日志系统初始化-[Android取经之路]](https://blog.csdn.net/yiranfeng/article/details/104244900)

# 先说结论

* 发现如下打印，表示audit丢失了：
```log
04-22 02:12:33.100 W/audit   (    0): audit_lost=228 audit_rate_limit=5 audit_backlog_limit=1024
04-22 02:12:33.100 E/audit   (    0): rate limit exceeded
```

* 解决方案，`auditctl -r 100`指令可以设置每秒消息发送率：
```
1|A6650:/ # auditctl
Nothing to do
Usage: auditctl [-r rate]

auditctl -r 100
```

# auditctl应用分析

* `QSSI.12/system/logging/logd/auditctl.cpp`:
```c
static void do_update_rate(uint32_t rate) {
    int fd = audit_open();
    if (fd == -1) {
        error(EXIT_FAILURE, errno, "Unable to open audit socket");
    }
    int result = audit_rate_limit(fd, rate);
    close(fd);
    if (result < 0) {
        fprintf(stderr, "Can't update audit rate limit: %d\n", result);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {
    uint32_t rate = 0;
    bool update_rate = false;
    int opt;

    while ((opt = getopt(argc, argv, "r:")) != -1) {
        switch (opt) {
            case 'r':
                if (!android::base::ParseUint<uint32_t>(optarg, &rate)) {
                    error(EXIT_FAILURE, errno, "Invalid Rate");
                }
                update_rate = true;
                break;
            default: /* '?' */
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // In the future, we may add other options to auditctl
    // so this if statement will expand.
    // if (!update_rate && !update_backlog && !update_whatever) ...
    if (!update_rate) {
        fprintf(stderr, "Nothing to do\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (update_rate) {
        do_update_rate(rate);
    }

    return 0;
}
```

* `logd/libaudit.cpp`上面发送函数`audit_rate_limit`，创建一个netlink的socket，协议号为NETLINK_AUDIT，并通过audit_rate_limit发送selinux频率:
```c
int audit_rate_limit(int fd, uint32_t limit) {
    struct audit_status status = {
            .mask = AUDIT_STATUS_RATE_LIMIT, .rate_limit = limit, /* audit entries per second */
    };
    return audit_send(fd, AUDIT_SET, &status, sizeof(status));
}

/**
 *
 * @param fd
 *  The netlink socket fd
 * @param type
 *  The type of netlink message
 * @param data
 *  The data to send
 * @param size
 *  The length of the data in bytes
 * @return
 *  This function returns a positive sequence number on success, else -errno.
 */
static int audit_send(int fd, int type, const void* data, size_t size) {
    struct sockaddr_nl addr = {.nl_family = AF_NETLINK};
  /* 省略 */
    /* Set up the netlink headers */
    struct audit_message req = {};
    req.nlh.nlmsg_type = static_cast<uint16_t>(type);
    req.nlh.nlmsg_len = NLMSG_SPACE(size);
    req.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    ssize_t rc = TEMP_FAILURE_RETRY(
            sendto(fd, &req, req.nlh.nlmsg_len, 0, (struct sockaddr*)&addr, sizeof(addr)));

    /* Not all the bytes were sent */
    if (rc < 0) {
        return -errno;
    } else if ((uint32_t)rc != req.nlh.nlmsg_len) {
        return -EPROTO;
    }

    /* We sent all the bytes, get the ack */
    rc = get_ack(fd);

    /* If the ack failed, return the error, else return the sequence number */
    rc = (rc == 0) ? (int)sequence : rc;

    return rc;
}

int audit_open() {
    return socket(PF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_AUDIT);
}
```

# 启动 logd-auditctl

logd.rc中启动logd-auditctl
```shell

# Limit SELinux denial generation to 5/second
service logd-auditctl /system/bin/auditctl -r 5
    oneshot
    disabled
    user logd
    group logd
    capabilities AUDIT_CONTROL
```
logd-auditctl 的主体是 /system/bin/auditctl，在logd的android.bp中，通过编译 auditctl.cpp得来，并加载了liblogd的库。
logd-auditctl是Android 10.0中引入的新功能，目的是让selinux denia的日志打印限制为5次/秒。


# kernel audit消息接收

* `kernel/audit.c`:
```c
* postcore_initcall(audit_init); //audit驱动的注册
  * register_pernet_subsys(&audit_net_ops); //register a network namespace subsystem
  * kauditd_task = kthread_run(kauditd_thread, NULL, "kauditd"); //感觉应该是轮询socket消息的
```

`register_pernet_subsys`该函数的主要作用是将一个网络协议子系统添加到网络命令空间对应的全局链表pernet_list中，并针对每一个注册在net_namespace_list链表中的网络命令空间，均执行其ops->init(audit_net_init)程序进行初始化，一般其ops->init会在其对应的proc目录下，生成一个网络协议模块对应的proc文件或proc目录，并执行一些协议初始化相关的函数。
```c
static struct pernet_operations audit_net_ops __net_initdata = {
    .init = audit_net_init,
    .exit = audit_net_exit,
    .id = &audit_net_id,
    .size = sizeof(struct audit_net),
};

static int __net_init audit_net_init(struct net *net)
{
    struct netlink_kernel_cfg cfg = {
        .input  = audit_receive,
        .bind   = audit_bind,
        .flags  = NL_CFG_F_NONROOT_RECV,
        .groups = AUDIT_NLGRP_MAX,
    };

    struct audit_net *aunet = net_generic(net, audit_net_id);

    aunet->sk = netlink_kernel_create(net, NETLINK_AUDIT, &cfg);
    if (aunet->sk == NULL) {
        audit_panic("cannot initialize netlink socket in namespace");
        return -ENOMEM;
    }
    aunet->sk->sk_sndtimeo = MAX_SCHEDULE_TIMEOUT;

    return 0;
}
```

* socket input及bind函数如下，通过`audit_receive_msg`轮询并处理消息：
```c
/**
 * audit_receive - receive messages from a netlink control socket
 * @skb: the message buffer
 *
 * Parse the provided skb and deal with any messages that may be present,
 * malformed skbs are discarded.
 */
static void audit_receive(struct sk_buff  *skb)
{
    struct nlmsghdr *nlh;
    /*
     * len MUST be signed for nlmsg_next to be able to dec it below 0
     * if the nlmsg_len was not aligned
     */
    int len;
    int err;

    nlh = nlmsg_hdr(skb);
    len = skb->len;

    audit_ctl_lock();
    while (nlmsg_ok(nlh, len)) {
        err = audit_receive_msg(skb, nlh);
        /* if err or if this message says it wants a response */
        if (err || (nlh->nlmsg_flags & NLM_F_ACK))
            netlink_ack(skb, nlh, err, NULL);

        nlh = nlmsg_next(nlh, &len);
    }
    audit_ctl_unlock();
}

/* Run custom bind function on netlink socket group connect or bind requests. */
static int audit_bind(struct net *net, int group)
{
    if (!capable(CAP_AUDIT_READ))
        return -EPERM;

    return 0;
}

static int audit_receive_msg(struct sk_buff *skb, struct nlmsghdr *nlh)
{
    u32         seq;
    void            *data;
    int         data_len;
    int         err;
    struct audit_buffer *ab;
    u16         msg_type = nlh->nlmsg_type;
    struct audit_sig_info   *sig_data;
    char            *ctx = NULL;
    u32         len;

    err = audit_netlink_ok(skb, msg_type);
    if (err)
        return err;

    seq  = nlh->nlmsg_seq;
    data = nlmsg_data(nlh);
    data_len = nlmsg_len(nlh);

    switch (msg_type) {
        case AUDIT_SET: {

            if (s.mask & AUDIT_STATUS_RATE_LIMIT) {
                err = audit_set_rate_limit(s.rate_limit); //设置速率
                if (err < 0)
                    return err;
            }
        }
    }
}
```

* 设置打印速率，先不分析了：
```c
static int audit_set_rate_limit(u32 limit)
{
    return audit_do_config_change("audit_rate_limit", &audit_rate_limit, limit);
}

```