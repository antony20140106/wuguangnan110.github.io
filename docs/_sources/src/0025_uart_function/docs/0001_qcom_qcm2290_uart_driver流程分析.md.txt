# 概述

分析一下qcm2290 uart驱动架构

# 参考


# kernel config

```
CONFIG_SERIAL_CORE=y
CONFIG_SERIAL_MSM_GENI=y 
CONFIG_SERIAL_MSM_GENI_HALF_SAMPLING=y
CONFIG_SERIAL_MSM_GENI_CONSOLE=y

CONFIG_TTY=y
```

# Makefile

* `serial/Makefile`:
```
obj-$(CONFIG_SERIAL_CORE) += serial_core.o
obj-$(CONFIG_SERIAL_MSM_GENI) += msm_geni_serial.o
```

* `tty/serial/Makefile`:
```
obj-$(CONFIG_TTY)       += tty_io.o n_tty.o tty_ioctl.o tty_ldisc.o \
                   tty_buffer.o tty_port.o tty_mutex.o \
                   tty_ldsem.o tty_baudrate.o tty_jobctrl.o \
                   n_null.o
```