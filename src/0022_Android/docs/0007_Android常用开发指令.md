# Android常用开发指令

记录一下开发Android时常用的开发指令


# 查找某个app安装位置

```
1|M92xx:/cache $ pm -l|grep market
package:com.xxx.market.android.app
M92xx:/cache $ pm -p com.xxx.market.android.app
package:/data/app/~~dE4wALCxRS61DxReGFCuGA==/com.xxx.market.android.app-iMGWfPBNEhmQVKssxH1F0A==/base.apk
```