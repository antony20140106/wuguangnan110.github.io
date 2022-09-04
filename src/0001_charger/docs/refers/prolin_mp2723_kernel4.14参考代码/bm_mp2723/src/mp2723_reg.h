
#ifndef __MP2723_HEADER__
#define __MP2723_HEADER__

/* Register 00h */
#define MP2723_REG_00      			0x00
#define MP2723_ENHIZ_MASK		    0x80
#define MP2723_ENHIZ_SHIFT		    7
#define MP2723_HIZ_ENABLE          	1
#define MP2723_HIZ_DISABLE         	0
#define MP2723_ENILIM_MASK		    0x40
#define MP2723_ENILIM_SHIFT			6
#define MP2723_ENILIM_ENABLE       	1
#define MP2723_ENILIM_DISABLE      	0

#define MP2723_IINLIM_MASK		    0x3F
#define MP2723_IINLIM_SHIFT			0
#define MP2723_IINLIM_BASE         	100
#define MP2723_IINLIM_LSB          	50

/* Register 01h */
#define MP2723_REG_01		    	0x01
#define MP2723_RESET_MASK			0x80
#define MP2723_RESET_SHIFT		    7
#define MP2723_RESET				1
// input vlotage limit
#define MP2723_VINMIN_MASK       	0x0F
#define MP2723_VINMIN_SHIFT      	0
#define MP2723_VINMIN_BASE       	3700
#define MP2723_VINMIN_LSB        	100

/* Register 0x02 */
#define MP2723_REG_02               0x02
#define MP2723_SLEEP_DELAY_MASK		0x80
#define MP2723_SLEEP_DELAY_SHIFT	7
#define MP2723_SLEEP_NO_DELAY		0
#define MP2723_SLEEP_DELAY_10S		1	// 10s delay (default)

#define MP2723_NTC_TYPE_MASK		0x40
#define MP2723_NTC_TYPE_SHIFT		6
#define MP2723_NTC_TYPE_STANDARD	0
#define MP2723_NTC_TYPE_JEITA		1	// (default)

#define MP2723_EN_OTG_NTC_MASK      0x20
#define MP2723_EN_OTG_NTC_SHIFT     5
#define MP2723_EN_OTG_NTC_ENABLE    1
#define MP2723_EN_OTG_NTC_DISABLE   0

#define MP2723_EN_CHG_NTC_MASK      0x10
#define MP2723_EN_CHG_NTC_SHIFT     4
#define MP2723_EN_CHG_NTC_ENABLE    1
#define MP2723_EN_CHG_NTC_DISABLE   0

#define MP2723_THERMALREG_MASK      0x0C
#define MP2723_THERMALREG_SHIFT     2
#define MP2723_THERMALREG_60C       0
#define MP2723_THERMALREG_80C       1
#define MP2723_THERMALREG_100C      2
#define MP2723_THERMALREG_120C      3

#define MP2723_NTC_OPT_MASK         0x02
#define MP2723_NTC_OPT_SHIFT        1
#define MP2723_NTC_OPT_PCB          1
#define MP2723_NTC_OPT_BATTERY      0

#define MP2723_AICO_EN_MASK   		0x01
#define MP2723_AICO_EN_SHIFT  		0
#define MP2723_AICO_EN_ENABLE    	1
#define MP2723_AICO_EN_DISABLE   	0

/* Register 0x03 */
#define MP2723_REG_03              0x03
#define MP2723_ADC_START_MASK      0x80
#define MP2723_ADC_START_SHIFT     7
#define MP2723_ADC_START_ENABLE    1
#define MP2723_ADC_START_DISABLE   0
#define MP2723_ADC_RATE_MASK       0x40
#define MP2723_ADC_RATE_SHIFT      6
#define MP2723_ADC_CONTINUE_ENABLE  1
#define MP2723_ADC_CONTINUE_DISABLE 0
// OTG voltage setting
#define MP2723_VINDSCHG_MASK		0x38
#define MP2723_VINDSCHG_SHIFT		3
#define MP2723_VINDSCHG_BASE		4800	// 4.8V
#define MP2723_VINDSCHG_LSB			100
// OTG current limit
#define MP2723_IINDSCHG_MASK		0x3
#define MP2723_IINDSCHG_SHIFT		0
#define MP2723_IINDSCHG_0_5A		0x0		// 0.5A	(default)
#define MP2723_IINDSCHG_0_8A		0x1		// 0.8A
#define MP2723_IINDSCHG_1_1A		0x2		// 1.1A
#define MP2723_IINDSCHG_1_5A		0x3		// 1.5A

/* Register 0x04*/
#define MP2723_REG_04              	0x04
#define MP2723_BAT_LOADEN_MASK     	0x80
#define MP2723_BAT_LOADEN_SHIFT    	7
#define MP2723_BAT_LOADEN_ENABLE   	1
#define MP2723_BAT_LOADEN_DISABLE  	0

#define MP2723_STAT_EN_MASK     	0x40
#define MP2723_STAT_EN_SHIFT    	6
#define MP2723_STAT_EN_ENABLE   	1
#define MP2723_STAT_EN_DISABLE  	0

#define MP2723_CHG_CONFIG_MASK     	0x30
#define MP2723_CHG_CONFIG_SHIFT    	4
#define MP2723_CHG_DISABLE			0
#define MP2723_CHG_ENABLE			1
#define MP2723_CHG_CONFIG_OTG		3

#define MP2723_VSYSMIN_MASK        	0x0E
#define MP2723_VSYSMIN_SHIFT        1
#define MP2723_VSYSMIN_3V         	0
#define MP2723_VSYSMIN_3_15V        1
#define MP2723_VSYSMIN_3_3V			2
#define MP2723_VSYSMIN_3_45V		3
#define MP2723_VSYSMIN_3_525V		4
#define MP2723_VSYSMIN_3_6V			5
#define MP2723_VSYSMIN_3_675V		6
#define MP2723_VSYSMIN_3_75V		7

#define MP2723_VTRACH_MASK        	0x01
#define MP2723_VTRACH_SHIFT        	0
#define MP2723_VTRACH_100MV        	0
#define MP2723_VTRACH_150MV        	1

/* Register 0x05*/
#define MP2723_REG_05              	0x05
// Pre-Charge to Fast Charge Threshold
#define MP2723_VBATTPRE_MASK       	0x80
#define MP2723_VBATTPRE_SHIFT       7
#define MP2723_VBATTPRE_2_8V		0	// 2.8V
#define MP2723_VBATTPRE_3V			1	// 3.0V (default)
// fast charge current
#define MP2723_ICC_MASK          	0x3F
#define MP2723_ICC_SHIFT         	0
#define MP2723_ICC_BASE          	320 // mA
#define MP2723_ICC_LSB          	42 // mA

/* Register 0x06*/
#define MP2723_REG_06              	0x06
#define MP2723_IPRECHG_MASK           	0xF0
#define MP2723_IPRECHG_SHIFT          	4
#define MP2723_IPRECHG_BASE           	120	// mA
#define MP2723_IPRECHG_LSB            	40

#define MP2723_ITERM_MASK           0x0F
#define MP2723_ITERM_SHIFT          0
#define MP2723_ITERM_BASE           100	// mA
#define MP2723_ITERM_LSB            40

/* Register 0x07*/
#define MP2723_REG_07              	0x07
#define MP2723_VBATTREG_MASK		0xFE
#define MP2723_VBATTREG_SHIFT		1
#define MP2723_VBATTREG_BASE		3400	// mV
#define MP2723_VBATTREG_LSB			10
#define MP2723_VBATTREG_MAX 		4670    // mV

#define MP2723_VRECHG_MASK			0x01
#define MP2723_VRECHG_SHIFT			0
#define MP2723_VRECHG_100MV			0	// (default)
#define MP2723_VRECHG_200MV			1

/* Register 0x08*/
#define MP2723_REG_08              	0x08
#define MP2723_EN_TERM_MASK        	0x80
#define MP2723_EN_TERM_SHIFT       	7
#define MP2723_TERM_ENABLE         	1	// (default)
#define MP2723_TERM_DISABLE        	0

#define MP2723_WDT_MASK            	0x30
#define MP2723_WDT_SHIFT           	4
#define MP2723_WDT_DISABLE         	0
#define MP2723_WDT_40S             	1	// (default)
#define MP2723_WDT_80S             	2
#define MP2723_WDT_160S            	3
#define MP2723_WDT_BASE            	0
#define MP2723_WDT_LSB             	40

#define MP2723_WDT_RST_MASK        	0x08
#define MP2723_WDT_RST_SHIFT       	3
#define MP2723_WDT_RESET			1

#define MP2723_CHG_TIMER_MASK      	0x06
#define MP2723_CHG_TIMER_SHIFT     	1
#define MP2723_CHG_TIMER_5HOURS    	0
#define MP2723_CHG_TIMER_8HOURS    	1
#define MP2723_CHG_TIMER_12HOURS   	2
#define MP2723_CHG_TIMER_20HOURS   	3

#define MP2723_EN_TIMER_MASK       	0x1
#define MP2723_EN_TIMER_SHIFT      	0
#define MP2723_EN_TIMER_ENABLE     	1
#define MP2723_EN_TIMER_DISABLE    	0

/* Register 0x09*/
// IR Compensation resistor
#define MP2723_REG_09              	0x09
#define MP2723_RBATCMP_MASK      	0xF0
#define MP2723_RBATCMP_SHIFT     	4
#define MP2723_RBATCMP_BASE			0
#define MP2723_RBATCMP_LSB			10
// Battery Compensation Voltage Clamp (Above VBATT_REG) 
#define MP2723_VCLAMP_MASK      	0x07
#define MP2723_VCLAMP_SHIFT     	0
#define MP2723_VCLAMP_BASE			30
#define MP2723_VCLAMP_LSB			60

/* Register 0x0A*/
#define MP2723_REG_0A              	0x0A
#define MP2723_SW_FREQ_MASK         0x80
#define MP2723_SW_FREQ_SHIFT        7
#define MP2723_FET_OFF_TIME_MASK	0x08
#define MP2723_FET_OFF_TIME_SHIFT	3
#define MP2723_FET_OFF_TIME_4S     	0
#define MP2723_SW_FREQ_1_35MHZ      0
#define MP2723_SW_FREQ_1MHZ         1

#define MP2723_TMR2X_EN_MASK        0x40
#define MP2723_TMR2X_EN_SHIFT       6
#define MP2723_TMR2X_EN_DISABLE     0
#define MP2723_TMR2X_EN_ENABLE      1

#define MP2723_BATFET_DIS_MASK      0x20
#define MP2723_BATFET_DIS_SHIFT     5
#define MP2723_BATFET_DIS_ON    	0
#define MP2723_BATFET_DIS_OFF     	1

#define MP2723_SYSRST_SEL_MASK      0x10
#define MP2723_SYSRST_SEL_SHIFT     4
#define MP2723_SYSRST_SEL_HW    	0
#define MP2723_SYSRST_SEL_SW     	1

#define MP2723_TDISCH_MASK      	0x0C
#define MP2723_TDISCH_SHIFT     	2
#define MP2723_TDISCH_0_5S    		0
#define MP2723_TDISCH_2S     		1
#define MP2723_TDISCH_4S    		2
#define MP2723_TDISCH_8S     		3

#define MP2723_TDISCL_MASK      	0x03
#define MP2723_TDISCL_SHIFT     	0
#define MP2723_TDISCL_8S    		0
#define MP2723_TDISCL_10S     		1
#define MP2723_TDISCL_12S    		2
#define MP2723_TDISCL_16S     		3

/* Register 0x0B*/
#define MP2723_REG_0B              0x0B
#define MP2723_INT_MASK_MASK		0xC0
#define MP2723_INT_MASK_SHIFT		6
#define MP2723_USB_DET_EN_MASK		0x20
#define MP2723_USB_DET_EN_SHIFT		5
#define MP2723_USB_DET_ENABLE		1
#define MP2723_USB_DET_DISABLE		0

/* Register 0x0C*/
#define MP2723_REG_0C              	0x0c
#define MP2723_VIN_STAT_MASK		0xE0
#define MP2723_VIN_STAT_SHIFT		5
#define MP2723_VIN_STAT_NOINPUT		0
#define MP2723_VIN_STAT_NOSTD		1
#define MP2723_VIN_STAT_SDP			2
#define MP2723_VIN_STAT_CDP			3
#define MP2723_VIN_STAT_DCP			4
#define MP2723_VIN_STAT_OTG			7

#define MP2723_CHG_STAT_MASK		0x18
#define MP2723_CHG_STAT_SHIFT		3
#define MP2723_CHG_STAT_NOCHG		0
#define MP2723_CHG_STAT_TRICHG		1
#define MP2723_CHG_STAT_CCCHG		2	// constant current charge
#define MP2723_CHG_STAT_CHGDONE		3

#define MP2723_RNTCFLOAT_STAT_MASK	0x4
#define MP2723_RNTCFLOAT_STAT_SHIFT	2

#define MP2723_THERM_STAT_MASK		0x2
#define MP2723_THERM_STAT_SHIFT		1
#define MP2723_THERM_STAT_NORMAL	0
#define MP2723_THERM_STAT_THERMREG	1

#define MP2723_VSYS_STAT_MASK		0x1
#define MP2723_VSYS_STAT_SHIFT		0
#define MP2723_VSYS_STAT_IN			0
#define MP2723_VSYS_STAT_NOTIN		1

/* Register 0x0D*/
#define MP2723_REG_0D              	0x0D
#define MP2723_FAULT_WDT_MASK      	0x80
#define MP2723_FAULT_WDT_SHIFT     	7
#define MP2723_FAULT_OTG_MASK    	0x40
#define MP2723_FAULT_OTG_SHIFT   	6
#define MP2723_FAULT_INPUT_MASK    	0x20
#define MP2723_FAULT_INPUT_SHIFT   	5

#define MP2723_FAULT_THERM_SHUTDOWN_MASK     	0x10
#define MP2723_FAULT_THERM_SHUTDOWN_SHIFT    	4

#define MP2723_FAULT_BAT_MASK      0x08
#define MP2723_FAULT_BAT_SHIFT     3

#define MP2723_FAULT_NTC_MASK      0x07
#define MP2723_FAULT_NTC_SHIFT     0
#define MP2723_FAULT_NTC_NORMAL    0
#define MP2723_FAULT_NTC_WARM      2
#define MP2723_FAULT_NTC_COOL      3
#define MP2723_FAULT_NTC_COLD      5
#define MP2723_FAULT_NTC_HOT       6

/* Register 0x0E*/
#define MP2723_REG_0E              	0x0E
#define MP2723_BATV_MASK           	0xFF
#define MP2723_BATV_SHIFT          	0
#define MP2723_BATV_BASE          	0
#define MP2723_BATV_LSB            	20

/* Register 0x0F*/
#define MP2723_REG_0F              	0x0F
#define MP2723_SYSV_MASK           	0xFF
#define MP2723_SYSV_SHIFT          	0
#define MP2723_SYSV_BASE           	0
#define MP2723_SYSV_LSB            	20

/* Register 0x10*/
#define MP2723_REG_10              	0x10
#define MP2723_NTC_MASK          	0xFF
#define MP2723_NTC_SHIFT         	0
#define MP2723_NTC_BASE          	0
#define MP2723_NTC_LSB           	39 //should be 0.39,kernel does not support float

/* Register 0x11*/
#define MP2723_REG_11              	0x11
#define MP2723_VIN_MASK          	0x7F
#define MP2723_VIN_SHIFT         	0
#define MP2723_VIN_BASE          	0
#define MP2723_VIN_LSB           	60	// mV

/* Register 0x12*/
#define MP2723_REG_12              0x12
#define MP2723_ICHGR_MASK          0xFF
#define MP2723_ICHGR_SHIFT         0
#define MP2723_ICHGR_BASE          0
#define MP2723_ICHGR_LSB           17500	// 17.5mA

/* Register 0x13*/
#define MP2723_REG_13              	0x13
#define MP2723_IIN_MASK      		0xFF
#define MP2723_IIN_SHIFT     		0
#define MP2723_IIN_BASE          	0
#define MP2723_IIN_LSB           	13300	// 13.3mA

/* Register 0x14*/
#define MP2723_REG_14               0x14
#define MP2723_VDPM_STAT_MASK     0x80
#define MP2723_VDPM_STAT_SHIFT    7
#define MP2723_IDPM_STAT_MASK     0x40
#define MP2723_IDPM_STAT_SHIFT    6
#define MP2723_IDPM_LIM_MASK      0x3F
#define MP2723_IDPM_LIM_SHIFT     0
#define MP2723_IDPM_LIM_BASE      100
#define MP2723_IDPM_LIM_LSB       50

/* Register 0x15*/
#define MP2723_REG_15              	0x15
#define MP2723_AICO_STAT_MASK      	0x80
#define MP2723_AICO_STAT_SHIFT     	7
#define MP2723_VPPM_INT_MASK_MASK   0x40
#define MP2723_VPPM_INT_MASK_SHIFT  6
#define MP2723_IPPM_INT_MASK_MASK   0x20
#define MP2723_IPPM_INT_MASK_SHIFT  5

/* Register 0x16*/
#define MP2723_REG_16              	0x16
#define MP2723_JEITA_VSET_MASK      0x80
#define MP2723_JEITA_VSET_SHIFT     7
#define MP2723_JEITA_ISET_MASK   	0x40
#define MP2723_JEITA_ISET_SHIFT  	6
#define MP2723_VHOT_MASK   			0x20
#define MP2723_VHOT_SHIFT  			5
#define MP2723_VWARM_MASK   		0x18
#define MP2723_VWARM_SHIFT  		3
#define MP2723_VCOOL_MASK   		0x06
#define MP2723_VCOOL_SHIFT  		1
#define MP2723_VCOLD_MASK   		0x01
#define MP2723_VCOLD_SHIFT  		0

/* Register 0x17*/
#define MP2723_REG_17              0x17
#define MP2723_SAFTY_TIMER_MASK    0x80
#define MP2723_SAFTY_TIMER_SHIFT   7
#define MP2723_SAFTY_TIMER         1
#define MP2723_PN_MASK             0x38
#define MP2723_PN_SHIFT            3

/* Register 0x18*/
#define MP2723_REG_18              			0x18
#define MP2723_IINLIM_VINMIN_RSTEN_MASK    	0x80
#define MP2723_IINLIM_VINMIN_RSTEN_SHIFT   	7
#define MP2723_ADDRESS_MASK             	0x04
#define MP2723_ADDRESS_SHIFT            	2
#define MP2723_PFM_EN_MASK             		0x02
#define MP2723_PFM_EN_SHIFT            		1

#endif


