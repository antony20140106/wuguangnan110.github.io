# README

Android 传统HAL层介绍。

本文介绍了 Android 中最为常见的两种硬件接口，传统 HAL 和 HIDL。其中 HAL 在 Android 8 中弃用，取而代之的是基于 IPC 的 HIDL 方案，后者同时支持 passthrough 模式兼容传统的 HAL，这也是很多厂商移植前的临时过渡方案。虽然使用 IPC 会在一定程度影响性能，但 HIDL 方案提供了许多优化的措施，比如通过共享内存快速消息队列(FMQ)进行数据交互。此外，我们还基于 HIDL 编写了一个简单的 demo 驱动以及配套的 service 和 client 示例，便于理解硬件创建和调用的流程，这对于固件驱动逆向而言也是必要的知识。

# refers

* [Android HAL 与 HIDL 开发笔记](https://blog.csdn.net/u010206565/article/details/109424991)

HAL 是 Hardware Abstraction Layer 的缩写，即硬件抽象层。从碎片化角度来说，作为系统的设计者，肯定是希望底层硬件按照类型整齐划一，而不是 Boardcom 实现一套、TI、ESP 又自己实现一套自己的 WIFi 接口；从商业角度说，硬件厂商自己硬件的软件(驱动)也是视为传家宝一样不希望被别人分析，所以要求操作系统可以无视自己的底层实现，只需要协商出统一的交互协议。

不论如何，多方交织的结果就是中间多了一层抽象。对于 Android 系统来说，这层抽象就是 HAL，虽然这并不是 Android 独有的概念。简而言之，Android HAL 就是定义了一个 .h 接口，并由硬件厂商拓展实现为动态链接库 .so，并使用约定的方式去加载和调用。

现在的时间已经来到了 Android 11，其实早在 Android 8 之后就已经弃用了曾经的 HAL 方式，不过由于碎片化原因，现在还有许多 IoT 设备等还是使用传统的 HAL 模式。另外出于对历史进展的研究，了解传统 HAL 也是有必要的。

# 分析

传统 HAL (Legacy HALs) 的接口文件为 hardware/libhardware/include/hardware/hardware.h ，主要定义了三个结构，分别是:

```C
struct hw_module_t;
struct hw_module_methods_t;
struct hw_device_t;
```

# hw_module_t

硬件模块 (hardware module) 表示 HAL 中打包的实现，即输出的.so动态链接库文件。hw_module_t 结构中主要包括 tab、version、name、author 等信息字段以及一个 struct hw_module_methods_t *methods 字段。methods 中包括打开设备的函数指针，如下:

```c
typedef struct hw_module_methods_t {
    /** Open a specific device */
    int (*open)(const struct hw_module_t* module, const char* id,
            struct hw_device_t** device);

} hw_module_methods_t;
```

每个硬件模块动态库中都需要定义一个符号 HAL_MODULE_INFO_SYM，并且该符号的第一个字段是 hw_module_t 类型。也就是说，厂商可以拓展 hw_module_t 类型，增加自己的额外字段。比如某个摄像头硬件所定义的结构如下:

```c
typedef struct camera_module {
    hw_module_t common;
    int (*get_number_of_cameras)(void);
    int (*get_camera_info)(int camera_id, struct camera_info *info);
} camera_module_t;
```

这也是使用 C 语言实现继承的一种典型方式。

# hw_device_t

device 用于抽象产品的某个具体硬件，比如对于某些摄像头模组，其硬件模块中就可能包括 2D 摄像头、3D 深度摄像头、红外摄像头等具体的 device。设备的结构基本元素如下:

```c
/**
 * Every device data structure must begin with hw_device_t
 * followed by module specific public methods and attributes.
 */
typedef struct hw_device_t {
    /** tag must be initialized to HARDWARE_DEVICE_TAG */
    uint32_t tag;

    /**
     * Version of the module-specific device API. This value is used by
     * the derived-module user to manage different device implementations.
     *
     * The module user is responsible for checking the module_api_version
     * and device version fields to ensure that the user is capable of
     * communicating with the specific module implementation.
     *
     * One module can support multiple devices with different versions. This
     * can be useful when a device interface changes in an incompatible way
     * but it is still necessary to support older implementations at the same
     * time. One such example is the Camera 2.0 API.
     *
     * This field is interpreted by the module user and is ignored by the
     * HAL interface itself.
     */
    uint32_t version;

    /** reference to the module this device belongs to */
    struct hw_module_t* module;

    /** padding reserved for future use */
#ifdef __LP64__
    uint64_t reserved[12];
#else
    uint32_t reserved[12];
#endif

    /** Close this device */
    int (*close)(struct hw_device_t* device);

} hw_device_t;
```

和模块一样，厂商也是通过继承拓展 device 结构来实现具体的设备。除了上面这些简单的标准属性，其实对于不同种类的硬件，也有特定的数据结构类型，见 Android HAL Reference。例如，对于摄像头类型的硬件，在 hardware/camera.h 中定义了其标准拓展接口和数据类型，比如打开/关闭摄像头、设置参数、数据回调等等。

# 历程

bms hal层如下：

```C++
├── 1.0
│   └── default
│       ├── android.hardware.xxx_bms@1.0-service-lazy.rc
│       ├── android.hardware.xxx_bms@1.0-service.rc
│       ├── Android.mk
│       ├── PaxBMS.cpp  //hw_device_t 具体硬件
│       ├── PaxBMS.h
│       ├── service.cpp
│       └── serviceLazy.cpp
├── Android.mk
├── xxx_bms.c // hw_module_t
└── xxx_bms_test.c

PaxBMS.cpp构造函数中直接获取模块的接口：
1.module 封装起来，供System以上的服务进行调用。
xxx_bms_dev_t* getPaxBMSDevice() {
    xxx_bms_dev_t* xxx_bmsDevice = NULL;
    const hw_module_t* hwModule = NULL;

    int ret = hw_get_module (xxx_BMS_HARDWARE_MODULE_ID, &hwModule);
    if (ret == 0) {
        ret = hwModule->methods->open(hwModule, xxx_BMS_HARDWARE_MODULE_ID,
            reinterpret_cast<hw_device_t**>(&xxx_bmsDevice));
        if (ret != 0) {
            ALOGE("xxx_bms_open %s failed: %d", xxx_BMS_HARDWARE_MODULE_ID, ret);
        }
}

PaxBMS::PaxBMS()
  : mPaxBMS(NULL) {
          mPaxBMS = getPaxBMSDevice();
}

封装接口：
// Methods from ::android::hardware::xxx_bms::V1_0::IPaxBMS follow.
Return<int32_t> PaxBMS::EnableCharge() {
        int ret = 0;
        int en = 1;

        if (mPaxBMS) {
                ret = mPaxBMS->bms_ctl(mPaxBMS, SET_CHG_EN, &en);
        }

        return ret;
}


xxx_bms.c：
/** Open a new instance of a device using name */
static int open_xxx_bms(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{
    pthread_once(&g_init, init_globals);

        if (strcmp(xxx_BMS_HARDWARE_MODULE_ID, name) != 0)
                return -1;

    struct xxx_bms_dev_t *dev = malloc(sizeof(struct xxx_bms_dev_t));
    if (!dev)
        return -ENOMEM;

    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t*)module;
    dev->common.close = (int (*)(struct hw_device_t*))close_xxx_bms;
    dev->bms_ctl = xxx_bms_ctl;

    *device = (struct hw_device_t*)dev;
    return 0;
}


static struct hw_module_methods_t xxx_bms_module_methods = {
    .open =  open_xxx_bms,
};

/*
 * The xxx bms Module
 */
struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    //.version_major = 1,
    //.version_minor = 0,
    .id = xxx_BMS_HARDWARE_MODULE_ID,
    .name = "xxx_bms Module",
    .author = "xxx",
    .methods = &xxx_bms_module_methods,
};
```

`1.0/default/service.cpp`通过 defaultPassthroughServiceImplementation 的调用来注册服务即绑定+直通式binder:

```c
using android::hardware::xxx_bms::V1_0::IPaxBMS;
using android::hardware::defaultPassthroughServiceImplementation;

int main() {
    return defaultPassthroughServiceImplementation<IPaxBMS>();
}
```

可以改为绑定式，参考：

* [Android-Gnss/GPS HAL层实现绑定+直通式修改为纯绑定式示例](https://blog.csdn.net/halazi100/article/details/107081622/)