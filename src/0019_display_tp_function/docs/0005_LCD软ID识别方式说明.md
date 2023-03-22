# LCD软ID识别方式说明

简单说明下百富定义的物料软ID，定义此ID的目的是为了方便百富对物料信息进行追踪

## 该文档适用人员

* 驱动开发工程师，需要通过软ID识别物料，与对应驱动进行匹配

* 物料采购或硬件相关同事，需要在百富info系统申请关键字，将关键字转换成对应的软ID，并将ID告知供应商
  
* 供应商伙伴，需要将物料对应的软ID烧录到IC中，以供软件驱动读取识别

## 软ID定义规则

* 从百富info系统申请关键字，以下图为例：

![34_百富lcd关键字](images/34_xxx_lcd_keyword.png)

上图的关键字中，包含了物料料号，lcd的分辨率，尺寸大小，对应项目，LCD玻璃信息，LCD-IC型号，TP-IC型号，供应商名称等等

* 将关键字转换成软ID并烧录到IC内

关键字申请成功后，需要将关键字转换成对应的软ID，以上图的254关键字为例，一般百富info系统上TP和LCD关键字是同一个，则表示TP、LCD的关键字都为254，如不同则按具体关键字烧录，首先需要将十进制的关键字转换成16进制，如`254`转换为16进制为`0xFE`(驱动中读取一般是按16进制读取)，则需要将LCD和TP的软ID`0xFE`写入lcd的IC中，以供lcd驱动读取，在上图的M8项目中，使用的显示芯片的型号为JD9365，通过JD9365的datasheet，可以知道读取ID的三个寄存器地址分别为`0xDA，0xDB，0xDC`，下图是datasheet上对这三个寄存器的说明：

![34_js9365_寄存器1](images/34_xxx_lcd_register_address1.png)

![34_jd9365_寄存器2](images/34_xxx_lcd_register_address2.png)

![34_js9365_寄存器3](images/34_xxx_lcd_register_address3.png)

每个寄存器占8bit，则三个寄存器占24bit，百富定义为以0xDA的bit0开始，前10bit用于烧录LCD关键字，第11-20bit用于烧录TP关键字，最后21-24bit预留以备不时之需，供应商需要将转换之后的软ID`0xFE`烧录到`0xDA,0xDB,0xDC`这三个寄存器中，默认从0xDA开始烧录，未使用到的则默认烧录0，最终烧录情况如下：

![34_js9365_寄存器4](images/34_xxx_lcd_register_address4.png)

## 软件驱动识别说明

以mtk 6765平台为例，说明下如何通过代码识别软ID，其他平台仅供参考

```C
#define LCM_ID_JD9365 (0xFE)

static unsigned int lcm_compare_id(void)
{
	int id = 0;
	unsigned char buffer[3] = {0};
	unsigned int array[16];
    int data[4] = {0,0,0,0};
    int res = 0;
    int rawdata = 0;
    int lcm_vol = 0;

	array[0] = 0x00023700;	/* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xDA, buffer, 1);
	read_reg_v2(0xDB, buffer + 1, 1);
	read_reg_v2(0xDC, buffer + 2, 1);
	id = buffer[2]|(buffer[1]<<8)|(buffer[0]<<16); 	/* we only need ID */

	LCM_LOGI("%s, jd9365_compare_id=0x%04x \n", __func__, id);
	if (id == LCM_ID_JD9365 ) {
		LCM_LOGI("%s,jd9365 compare id OK!\n", __func__);
		return 1;
	}
	else
		return 0;
}
```

## 注意事项

**驱动开发同事拿到屏后，一般调试时读取ID都是按照供应商提供的datasheet中提供的三个寄存器的值去读取识别，调试完成后提交代码，后续如果物料或者硬件同事通知供应商重新烧录软ID，一定要同步告知驱动开发同事去修改驱动代码中的id，否则未修改还是按原来的id去匹配驱动，会导致匹配失败，屏无法点亮的现象**

   