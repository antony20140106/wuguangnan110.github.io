# 概述

内核模块引用CONFIG实例

# 参考

* `Makefile`:

```C++
usb_switch:
ifeq ($(CONFIG_PAX_GPIOS_CONTROL), y)
    subdir-ccflags-y += -I$(srctree)/drivers/misc/pax/gpio/
endif

pax_gpio:
# pax gpio set control
obj-$(CONFIG_PAX_GPIOS_CONTROL) += pax_gpio_control.o r15_status_notify.o
```

* pax_usb_switch:

```C++
#ifdef CONFIG_PAX_GPIOS_CONTROL
#include "r15_status_notify.h"
#endif
```