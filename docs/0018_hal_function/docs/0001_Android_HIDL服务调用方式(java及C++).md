# README

Android HIDL服务可以通过java层service或者C++ jni方式进行binder通信，介绍一下。

# refers

* [android HIDL HAL控制LED demo（二）之JAVA作客户端](https://blog.csdn.net/weixin_39655765/article/details/106324762)
* [android HIDL HAL控制LED demo（一）之C++作客户端](https://blog.csdn.net/weixin_39655765/article/details/100871332)

# 项目实例

## java方式

目前M8/M50项目中paxservice用的时java直接调用binder和HIDL通信，架构如下：

![0002_架构图.png](/docs/0004_framework_function/docs/images/0002_架构图.png)

参考文档：

* [0002_PaxApiService架构(双击唤醒+R15+R20控制功能).md](/docs/0004_framework_function/docs/0002_PaxApiService架构(双击唤醒+R15+R20控制功能).md)

## C++方式

目前M8/M50项目中paxbms采用的是C++ jni方式和HIDL进行binder通信，架构如下：

![0009_0014.png](/docs/0001_charger/docs/images/0009_0014.png)

参考文档：

* [0009_charger_BMS驱动及整体架构简介.md](/docs/0001_charger/docs/0009_charger_BMS驱动及整体架构简介.md)