#ifndef __ASW5480_H__
#define __ASW5480_H__


//ASW5480_reg_DeviceType
#define ASW5480_DeviceType  0x09

#define AWS5480_reg_DeviceType 0x00
#define ASW5480_SwitchEnable  0x04
#define ASW5480_SwitchSelect  0x05
#define ASW5480_SwitchStatus  0x06
//#define AWS5480_DP_R_Switch_Status 

extern int pax_set_audio_switch(bool on);
extern int pax_get_audio_switch(void);
#endif
