# 概述

内核模块引用CONFIG实例

# 参考

* `Makefile`:

```C++
usb_switch:
ifeq ($(CONFIG_xxx_GPIOS_CONTROL), y)
    subdir-ccflags-y += -I$(srctree)/drivers/misc/xxx/gpio/
endif

xxx_gpio:
# xxx gpio set control
obj-$(CONFIG_xxx_GPIOS_CONTROL) += xxx_gpio_control.o r15_status_notify.o
```

* xxx_usb_switch:

```C++
#ifdef CONFIG_xxx_GPIOS_CONTROL
#include "r15_status_notify.h"
#endif
```