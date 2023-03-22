# 概述

在Android版本中，许多Linux命令以及Android自带的命令，有一部分命令已经是二进制实体直接放在/system/bin/目录下了，比较新的android版本，还有一部分命令是集成在toybox这个二进制文件中了，然后是通过软连接到toybox来执行对应命令的。

```shell
 root:/ # cd system/bin/
 root:/system/bin # ls -l
 -rwxr-xr-x 1 root shell  489352 2009-01-01 08:00 toybox
 -rwxr-xr-x 1 root shell   11688 2009-01-01 08:00 reboot
 -rwxr-xr-x 1 root shell  164928 2009-01-01 08:00 remount
 lrwxr-xr-x 1 root shell       6 2009-01-01 08:00 sync -> toybox
 lrwxr-xr-x 1 root shell       6 2009-01-01 08:00 sysctl -> toybox
 lrwxr-xr-x 1 root shell       6 2009-01-01 08:00 tac -> toybox
 lrwxr-xr-x 1 root shell       6 2009-01-01 08:00 tail -> toybox
 lrwxr-xr-x 1 root shell       6 2009-01-01 08:00 tar -> toybox
 lrwxr-xr-x 1 root shell       6 2009-01-01 08:00 vmstat -> toybox
 lrwxr-xr-x 1 root shell       6 2009-01-01 08:00 watch -> toybox
 lrwxr-xr-x 1 root shell       6 2009-01-01 08:00 wc -> toybox
 lrwxr-xr-x 1 root shell       6 2009-01-01 08:00 which -> toybox
 lrwxr-xr-x 1 root shell       6 2009-01-01 08:00 whoami -> toybox
 ......
 root:/system/bin # toybox
 acpi base64 basename blkid blockdev cal cat chattr chcon chgrp 
```

# 参考

* [Android12 系统的裁剪编译规则](https://blog.csdn.net/ldswfun/article/details/119783255)

# toybox工具制作原理

1.通过toybox的main()函数，可以调用到子命令函数。

即主main函数根据传进来的参数，调用子main函数。

2.将每个子命令软连接到系统，Android系统在/system/bin目录下，Linux在/usr/bin目录下。

即创建软连接。

# toybox工作命令原理

如果执行"toybox ls", 拆成两个字符串，如果第一个为空，就会接着判断第二个字符串“ls”,主main调用ls_main()函数即可。

# stty工具Android源码修改

* stty工具显示不支持750k波特率，打印如下：
```shell
1|M92xx:/ # stty -F /dev/ttyHS1 speed 750000 cs8 -parenb -cstopb
9600
stty: unknown speed: 750000
```

* 修改如下：
```diff
--- a/QSSI.12/external/toybox/toys/pending/stty.c
+++ b/QSSI.12/external/toybox/toys/pending/stty.c
@@ -49,7 +49,7 @@ GLOBALS(

 static const int bauds[] = {
   0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800, 9600,
-  19200, 38400, 57600, 115200, 230400, 460800, 500000, 576000, 921600,
+  19200, 38400, 57600, 115200, 230400, 460800, 500000, 576000, 750000, 921600,
   1000000, 1152000, 1500000, 2000000, 2500000, 3000000, 3500000, 4000000
 };
```

* 修改后能够设置：
```shell
M92xx:/ # stty -F /dev/ttyHS1 speed 750000 cs8 -parenb -cstopb
750000
M92xx:/ # stty -F /dev/ttyHS1 -a
speed 750000 baud; rows 0; columns 0; line = 0;
intr = ^C; quit = ^\; erase = ^?; kill = ^U; eof = ^D; eol = <undef>; eol2 = <undef>; swtch = <undef>; start = ^Q;
stop = ^S; susp = ^Z; rprnt = ^R; werase = ^W; lnext = ^V; discard = ^O; min = 1; time = 0;
-parenb -parodd -cmspar cs8 hupcl -cstopb cread clocal -crtscts
-ignbrk -brkint -ignpar -parmrk -inpck -istrip -inlcr -igncr icrnl ixon -ixoff -iuclc -ixany -imaxbel -iutf8
opost -olcuc -ocrnl onlcr -onocr -onlret -ofill -ofdel nl0 cr0 tab0 bs0 vt0 ff0
isig icanon iexten echo echoe echok -echonl -noflsh -xcase -tostop -echoprt echoctl echoke -flusho -extproc
```