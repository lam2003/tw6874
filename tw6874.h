/***************************************** (C) COPYRIGHT 2014 Intersil *****************************************
* File Name         : tw6874.h
* Description       : 
* Change log        : 
*                   
****************************************************************************************************************
* THE PRESENT SOFTWARE IS DISTRIBUTED IN THE HOPE THAT IT WILL PROVIDE CUSTOMERS 
* WITH CODING INFORMATION TO SPEED UP THEIR PRODUCT DEVELOPMENT, BUT WITHOUT
* ANY WARRANTY; WITHOUT EVEN THE IMPLIED WARRANTY OF MERCHANTABILITY OR FITNESS
* FOR A PARTICULAR PURPOSE. AS A RESULT, Intersil SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
* CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
****************************************************************************************************************/

#ifndef __TW6874_H__
#define __TW6874_H__

#include <linux/module.h>
#include <linux/delay.h>
#include "hst.h"
#include "typedefs.h"
#include "gpio_i2c.h"

//==================================================================================
#define OUTPUT_AUTO_OFF

void TW6874_Init(void);

u8 get_TW6874_SLAVE_ID0(void);
void TW6874_Ch(U8, U8);
U8 ReadTW6874(WORD);

void SetSignalTW6874(WORD, BYTE, BYTE);
U8 ReadTW6874(WORD);
void assert_reset_TW6874(void);
void clk_phase(U8, U8);
U8 SDI_locked(U8);
U8 CRC_error(U8);
void reset_WC_SDI1(void);
u8 check_tw6874_input_source(u8 ch);
void set_dirac_func(U8 ch, U8 res, U8 mode);
void AocSet(WORD chs);
void CrcCount(WORD channels);

U8 getVideoFormat(u8 channel);

void CheckChLock(U8 ch);
//void SetCableLen(U8 ch, U8 len);
void crc_check25(U8 ch, U8 aoc, U8 agc, U8 step);
void crc_auto(U8 ch, U8 mode, U8 aocI, U8 agcI, U8 stepI, U16 period);

//void afc_test(U8 ch);
//void lock_test(U8 ch);
void lock_auto(U8 ch);
//void lock_auto_sd(U8 ch, U8 long_offset, U8 short_offset);
U8 lock_check(U8 ch, U8 aoc, U8 agc);

void SetAudioCh(U8 InCh, U8 InMode, U8 OutCh);
void ReadANC(U8 ch, U8 pos, U8 did, U16 line);

void ReadANC_On(U8 ch, U8 did, U8 did2);
void ReadANC_Off(U8 ch);
U8 ReadANC_Resolution(U8 ch);
U8 ReadANC_Data(U8 ch, U8 did, U8 did2);

extern U8 poll_CRC;

extern unsigned char bAutoEQlock;
extern U8 DiracAutoFlag;
extern U8 bAutoCh[4];
//extern unsigned char bAutoVOctrl;
extern U8 DiracInput[4];
//extern U8 CLen;
//extern short calibratedAOC[5];
extern U16 minErr[5];

extern U8 Force_Lock;
extern U8 ForceDisplay;

extern U8 right_audio_chan;

#define Printf printk
#define Delay(x) msleep(x / 5)

//#define DEBUG_PRINT

#endif //... __TW6874_H__
