/*=============================================================================   
    @file  i2c_devcfg.c
    @brief I2C Device Config data
   
    Copyright (c) 2019 - 2020  Qualcomm Technologies, Incorporated.
                        All rights reserved.
    Qualcomm Technologies, Confidential and Proprietary.
===============================================================================*/

/*=============================================================================
                              EDIT HISTORY
 when       who     what, where, why
 --------   ---     -----------------------------------------------------------	
 24/01/20   AG      ported for Agatti
 07/30/19   bng     ported for Kamorta
 =============================================================================*/

#include <i2c_config.h>
#include "gpi.h"

// NOTE: QUPs are numbered starting from 0 as per the IO sheet
#define ENABLE_QUP_01 //EEPROM 
#define ENABLE_QUP_00 //battery

#define QUPV3_0_CORE_BASE_ADDRESS        0x04A00000 
#define QUPV3_0_CORE_COMMON_BASE_ADDRESS 0x04AC0000 

#define TLMM_GPIO_CFG(gpio, func, dir, pull, drive) \
                          (((gpio) & 0x3FF) << 4  | \
                           ((func) & 0xF  ) << 0  | \
                           ((dir)  & 0x1  ) << 14 | \
                           ((pull) & 0x3  ) << 15 | \
                           ((drive)& 0xF  ) << 17)

#define TLMM_GPIO_INPUT     0x0
#define TLMM_GPIO_OUTPUT    0x1
#define TLMM_GPIO_PULL_UP   0x3
#define TLMM_GPIO_2MA       0x0

#define QUP_0   0
#define QUP_1   1


//
// CONFIGURATION START ============================================
//
#define TOP_QUP_00_SDA TLMM_GPIO_CFG(0, 1, TLMM_GPIO_OUTPUT, TLMM_GPIO_PULL_UP, TLMM_GPIO_2MA)
#define TOP_QUP_00_SCL TLMM_GPIO_CFG(1, 1, TLMM_GPIO_OUTPUT, TLMM_GPIO_PULL_UP, TLMM_GPIO_2MA)
                                        
#define TOP_QUP_01_SDA TLMM_GPIO_CFG(4, 1, TLMM_GPIO_OUTPUT, TLMM_GPIO_PULL_UP, TLMM_GPIO_2MA)
#define TOP_QUP_01_SCL TLMM_GPIO_CFG(5, 1, TLMM_GPIO_OUTPUT, TLMM_GPIO_PULL_UP, TLMM_GPIO_2MA)
                                        
#define TOP_QUP_02_SDA TLMM_GPIO_CFG(6, 1, TLMM_GPIO_OUTPUT, TLMM_GPIO_PULL_UP, TLMM_GPIO_2MA)
#define TOP_QUP_02_SCL TLMM_GPIO_CFG(7, 1, TLMM_GPIO_OUTPUT, TLMM_GPIO_PULL_UP, TLMM_GPIO_2MA)
                                        
#define TOP_QUP_03_SDA TLMM_GPIO_CFG(8, 1, TLMM_GPIO_OUTPUT, TLMM_GPIO_PULL_UP, TLMM_GPIO_2MA)
#define TOP_QUP_03_SCL TLMM_GPIO_CFG(9, 1, TLMM_GPIO_OUTPUT, TLMM_GPIO_PULL_UP, TLMM_GPIO_2MA)
                                    
#define TOP_QUP_04_SDA TLMM_GPIO_CFG(96, 1, TLMM_GPIO_OUTPUT, TLMM_GPIO_PULL_UP, TLMM_GPIO_2MA)
#define TOP_QUP_04_SCL TLMM_GPIO_CFG(97, 1, TLMM_GPIO_OUTPUT, TLMM_GPIO_PULL_UP, TLMM_GPIO_2MA) 
                                    
#define TOP_QUP_05_SDA TLMM_GPIO_CFG(14, 1, TLMM_GPIO_OUTPUT, TLMM_GPIO_PULL_UP, TLMM_GPIO_2MA)
#define TOP_QUP_05_SCL TLMM_GPIO_CFG(15, 1, TLMM_GPIO_OUTPUT, TLMM_GPIO_PULL_UP, TLMM_GPIO_2MA)


const char *common_clocks_str_0 [] =
{
    "gcc_qupv3_wrap0_core_clk",
    "gcc_qupv3_wrap0_core_2x_clk",
    "gcc_qupv3_wrap_0_m_ahb_clk",
    "gcc_qupv3_wrap_0_s_ahb_clk",
    NULL,
};

const char *se_clocks_str_0 [] =
{
    "gcc_qupv3_wrap0_s0_clk",
    "gcc_qupv3_wrap0_s1_clk",
    "gcc_qupv3_wrap0_s2_clk",
    "gcc_qupv3_wrap0_s3_clk",
    "gcc_qupv3_wrap0_s4_clk",
    "gcc_qupv3_wrap0_s5_clk",
    
};

plat_clock_config clk_cfg[] =
{
    // src-freq, speed, div, cycle, high, low
    {     19200,   100,   7,    26,   10,  11 },
    {     19200,   400,   2,    24,    5,  12 },
    {     19200,  1000,   1,    18,    3,   9 },
    {    100000,  8000,   2,     0,    3,   5 },
    {    120000, 12500,   1,     0,    8,   6 },
    {         0,     0,   0,     0,    0,   0 },
};

#ifdef ENABLE_QUP_00
plat_device_config i2c_device_config_01 = 
{
    .core_base_addr              = (uint8 *) QUPV3_0_CORE_BASE_ADDRESS,
    .common_base_addr            = (uint8 *) QUPV3_0_CORE_COMMON_BASE_ADDRESS,
    .core_offset                 = 0x00080000,
    .qupv3_instance              = QUP_0,
    .core_index                  = 1,
    .core_irq                    = 0,
    .polled_mode                 = TRUE,
    .min_data_length_for_dma     = 0,

    .gpi_index                   = 0,

    .scl_encoding                = TOP_QUP_00_SCL,
    .sda_encoding                = TOP_QUP_00_SDA,

    .tcsr_base_addr              = (uint8 *) 0x00300000,
    .tcsr_reg_offset             = 0x00000000,
    .tcsr_reg_value              = 0x00000000,

    .common_clocks               = (uint8 **) common_clocks_str_0,
    .se_clock                    = (uint8 **) (se_clocks_str_0 + 0),

    .clock_config                = clk_cfg,
};
#endif

#ifdef ENABLE_QUP_01
plat_device_config i2c_device_config_02 = 
{
    .core_base_addr              = (uint8 *) QUPV3_0_CORE_BASE_ADDRESS,
    .common_base_addr            = (uint8 *) QUPV3_0_CORE_COMMON_BASE_ADDRESS,
    .core_offset                 = 0x00084000,
    .qupv3_instance              = QUP_0,
    .core_index                  = 2,
    .core_irq                    = 0,
    .polled_mode                 = TRUE,
    .min_data_length_for_dma     = 0,

    .gpi_index                   = 0,

    .scl_encoding                = TOP_QUP_01_SCL,
    .sda_encoding                = TOP_QUP_01_SDA,

    .tcsr_base_addr              = (uint8 *) 0x00300000,
    .tcsr_reg_offset             = 0x00000000,
    .tcsr_reg_value              = 0x00000000,

    .common_clocks               = (uint8 **) common_clocks_str_0,
    .se_clock                    = (uint8 **) (se_clocks_str_0 + 1),

    .clock_config                = clk_cfg,
};
#endif

#ifdef ENABLE_QUP_02
plat_device_config i2c_device_config_03 = 
{
    .core_base_addr              = (uint8 *) QUPV3_0_CORE_BASE_ADDRESS,
    .common_base_addr            = (uint8 *) QUPV3_0_CORE_COMMON_BASE_ADDRESS,
    .core_offset                 = 0x00088000,
    .qupv3_instance              = QUP_0,
    .core_index                  = 3,
    .core_irq                    = 0,
    .polled_mode                 = TRUE,
    .min_data_length_for_dma     = 0,

    .gpi_index                   = 0,

    .scl_encoding                = TOP_QUP_02_SCL,
    .sda_encoding                = TOP_QUP_02_SDA,

    .tcsr_base_addr              = (uint8 *) 0x00300000,
    .tcsr_reg_offset             = 0x00000000,
    .tcsr_reg_value              = 0x00000000,

    .common_clocks               = (uint8 **) common_clocks_str_0,
    .se_clock                    = (uint8 **) (se_clocks_str_0 + 2),

    .clock_config                = clk_cfg,
};
#endif

#ifdef ENABLE_QUP_03
plat_device_config i2c_device_config_04 = 
{
    .core_base_addr              = (uint8 *) QUPV3_0_CORE_BASE_ADDRESS,
    .common_base_addr            = (uint8 *) QUPV3_0_CORE_COMMON_BASE_ADDRESS,
    .core_offset                 = 0x0008C000,
    .qupv3_instance              = QUP_0,
    .core_index                  = 4,
    .core_irq                    = 0,
    .polled_mode                 = TRUE,
    .min_data_length_for_dma     = 0,

    .gpi_index                   = 0,

    .scl_encoding                = TOP_QUP_03_SCL,
    .sda_encoding                = TOP_QUP_03_SDA,

    .tcsr_base_addr              = (uint8 *) 0x00300000,
    .tcsr_reg_offset             = 0x00000000,
    .tcsr_reg_value              = 0x00000000,

    .common_clocks               = (uint8 **) common_clocks_str_0,
    .se_clock                    = (uint8 **) (se_clocks_str_0 + 3),

    .clock_config                = clk_cfg,
};
#endif

#ifdef ENABLE_QUP_04
plat_device_config i2c_device_config_05 = 
{
    .core_base_addr              = (uint8 *) QUPV3_0_CORE_BASE_ADDRESS,
    .common_base_addr            = (uint8 *) QUPV3_0_CORE_COMMON_BASE_ADDRESS,
    .core_offset                 = 0x00090000,
    .qupv3_instance              = QUP_0,
    .core_index                  = 5,
    .core_irq                    = 0,
    .polled_mode                 = TRUE,
    .min_data_length_for_dma     = 0,

    .gpi_index                   = 0,

    .scl_encoding                = TOP_QUP_04_SCL,
    .sda_encoding                = TOP_QUP_04_SDA,

    .tcsr_base_addr              = (uint8 *) 0x00300000,
    .tcsr_reg_offset             = 0x00000000,
    .tcsr_reg_value              = 0x00000000,

    .common_clocks               = (uint8 **) common_clocks_str_0,
    .se_clock                    = (uint8 **) (se_clocks_str_0 + 4),

    .clock_config                = clk_cfg,
};
#endif

#ifdef ENABLE_QUP_05
plat_device_config i2c_device_config_06 = 
{
    .core_base_addr              = (uint8 *) QUPV3_0_CORE_BASE_ADDRESS,
    .common_base_addr            = (uint8 *) QUPV3_0_CORE_COMMON_BASE_ADDRESS,
    .core_offset                 = 0x00094000,
    .qupv3_instance              = QUP_0,
    .core_index                  = 6,
    .core_irq                    = 0,
    .polled_mode                 = TRUE,
    .min_data_length_for_dma     = 0,

    .gpi_index                   = 0,

    .scl_encoding                = TOP_QUP_06_SCL,
    .sda_encoding                = TOP_QUP_06_SDA,

    .tcsr_base_addr              = (uint8 *) 0x00300000,
    .tcsr_reg_offset             = 0x00000000,
    .tcsr_reg_value              = 0x00000000,

    .common_clocks               = (uint8 **) common_clocks_str_0,
    .se_clock                    = (uint8 **) (se_clocks_str_0 + 5),

    .clock_config                = clk_cfg,
};
#endif




