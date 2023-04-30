# README

记录一些零散的Android方面的学习知识。

# docs

NO.|文件名称|摘要
:--:|:--|:--
0008| [Mark](docs/0008_Mark.md) | 记录一些零碎的知识
0007| [Android常用开发指令](docs/0007_Android常用开发指令.md) | 记录一下开发Android时常用的开发指令
0006| [Android12之toybox](docs/0006_Android12之toybox.md) | 在Android版本中，许多Linux命令以及Android自带的命令，有一部分命令已经是二进制实体直接放在/system/bin/目录下了，比较新的android版本，还有一部分命令是集成在toybox这个二进制文件中了，然后是通过软连接到toybox来执行对应命令的。
0005| [Android_qcm2290新增裁剪编译规则](docs/0005_Android_qcm2290新增裁剪编译规则.md) | Android系统源码全编译时，都会通过PRODUCT_PACKAGES来控制源码模块是否需要编译， 对于系统默认的一些模块， 如果想要进行裁剪， 比如pos上面，可能不需要电话，短信等APP， 此时就需要修改系统基础的配置文件， 将PRODUCT_PACKAGES中包含的电话短信APP模块删除， 这样改动太大， 耦合性也太大， 我们可以在Android系统源码的编译规则中增加一个`PRODUCT_REMOVE_PACKAGES`变量来对模块进行删除，实现模块裁剪的功效。当然裁剪不仅仅局限于系统APP， 源码中所有的模块都可以通过`PRODUCT_REMOVE_PACKAGES`来裁剪。
0004| [qcom_qcm2290_32go版本编译](docs/0004_qcom_qcm2290_32go版本编译.md) | 高通平台Android 12 Go版本编译问题记录。
0003| [Android_Kernel如何确定使用哪个defconfig文件](docs/0003_Android_Kernel如何确定使用哪个defconfig文件.md) | 以高通平台qcm2290为例，分析一下Kernel如何确定使用哪个defconfig文件。
0002| [Android_R平台上qssi的介绍](docs/0002_Android_R平台上qssi的介绍.md) | 以高通平台qcm2290为例，分析一下QSSI是什么。
0001| [Android_lunch之如何检索系统所需要MK编译文件](docs/0001_Android_lunch之如何检索系统所需要MK编译文件.md) | 以高通平台qcm2290为例，分析一下lunch product到底干了什么，又是如何检索所有的mk文件的，`AndroidProducts.mk`这个是Android lunch的起始文件分析一下，有助于理解device下的架构。
