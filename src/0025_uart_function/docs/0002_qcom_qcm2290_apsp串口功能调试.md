# qcom qcm2290 apsp串口功能调试

记录一下

# APSP串口不通

目前AP端的串口程序都是A6650项目可用的，但是在ABL阶段使用流控功能出现握手失败。

* 修改成115200波特率：
```diff
--- a/UM.9.15/bootable/bootloader/edk2/MdePkg/Library/UartQupv3Lib/UartXBL.c
+++ b/UM.9.15/bootable/bootloader/edk2/MdePkg/Library/UartQupv3Lib/UartXBL.c
@@ -979,7 +979,7 @@ DESCRIPTION:
 RETURN_STATUS EFIAPI
 uart_initialize(void)
 {
-/*
+
    uart_open_config c;

    c.baud_rate = 115200;
@@ -994,7 +994,7 @@ uart_initialize(void)


    uart_open(&uart_debug_handle, UART_DEBUG_PORT, &c);
-*/
+
    return RETURN_SUCCESS;
 }

@@ -1010,7 +1010,7 @@ uart5_initialize(void)
 {
    uart_open_config uart5_c;

-   uart5_c.baud_rate = 3000000;
+   uart5_c.baud_rate = 115200;
    uart5_c.parity_mode = UART_NO_PARITY;
    uart5_c.num_stop_bits = UART_1_0_STOP_BITS;
    uart5_c.bits_per_char = UART_8_BITS_PER_CHAR;

--- a/A6650_Unpacking_Tool/BOOT.XF.4.1/boot_images/QcomPkg/Library/UartQupv3Lib/UartXBL.c
+++ b/A6650_Unpacking_Tool/BOOT.XF.4.1/boot_images/QcomPkg/Library/UartQupv3Lib/UartXBL.c
@@ -841,7 +841,8 @@ uart_initialize(void)

    uart_open_config uart5_c;

-   uart5_c.baud_rate = 3000000/*115200*/;
+   uart5_c.baud_rate = 115200;
+   //uart5_c.baud_rate = 3000000/*115200*/;
```

原因是SP端RTS复位功能未打开，当AP发送时RTS未拉低，SP端发送，AP不接收。