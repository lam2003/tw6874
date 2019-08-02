/***************************************** (C) COPYRIGHT 2014 Intersil *****************************************
* File Name         : tw6874.c
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

#include "tw6874.h"

//#define		AUTO_DEBUG	1

#define 	CRC_ENABLE
//#define	CRC_SCREEN
//#define		AUTO_DEBUG2	1

#define		RES_OVER_ANC

U16 CheckChCrc(U8 ch, U16 period); //, U8 addr, U8 range, U8 type);
U8 ANCDetection(U8 ch, U8 did);
U8 ReadANC_Resolution(U8 ch);

U8 DiracInput[4];

U8 p_lock_in[4];
U8 lock_in[4];

U8 right_audio_chan;				//0: only left channel is routed out from SDI port; 1: only right channel is routed out from SDI port;

//U8 CLen;

///////////////
U16 minErr[5];
U8 DiracAutoFlag;
/////////////////

U8 Force_Lock;
U8 ForceDisplay;

extern unsigned char TW6874_SLAVE_ID0;

void Ms_Delay(U16 t)
{
	Delay(5*t);
}
//==================================================================================
//u8 chlockcnt[4], chformat[4], chformatcnt[4];
//U8 chMode[4], chAGC[4];
//short calibratedAOC[5];

void SetSignalI2C(U8 slave, U16 index, U8 range, U8 val)
{
	U8 mask = 0;
	U8 length, high, low, readVal, newVal;

	high  = (range >> 4) & 0x0f;
	low  = range & 0x0f;
	if (high<low)
	{
		Printf("\r\nSetSignalI2C() incorrect range");
		return;
	}
	length = high-low+1;
	mask = ((1<<length)-1)<<low;
	val<<=low;

	readVal=gpio_i2c2_read(slave,index,1);
	//ReadI2C( slave, 2, indexArr, 1, &readVal );
	newVal = (readVal & ~mask) | (mask & val);
	gpio_i2c2_write(slave,index,newVal,1);
	//WriteI2C( slave, 2, indexArr, 1, &newVal );

#ifdef DEBUG_PRINT
	Printf("\r\nhigh %d", (WORD)high);
	Printf("\r\nlow %d", (WORD)low);
	Printf("\r\nlength %d", (WORD)length);
	Printf("\r\nmask %4x", (WORD)mask);
	Printf("\r\nval %4x", (WORD)val);
	Printf("\r\nreadVal %4x", (WORD)readVal);
	Printf("\r\nnewval %4x", (WORD)((readVal & ~mask) | (mask & val)));
#endif
}


void SetSignalTW6874(WORD index, BYTE range, BYTE val)	// use SPI or I2C to modify a TW6874 signal (group of bits in a register). To maintain compatibility between I2C and SPI, we only use byte data. SPI uses 16-bit data, so we need to modify the correct byte.
{
	SetSignalI2C(get_TW6874_SLAVE_ID0(), index, range, val);
}

U8 ReadTW6874(WORD index)	// return a byte data from the selected register regardless of SPI or I2C. SPI uses 16-bit data, so we need to return the correct byte.
{
	U8 b_readVal;

	b_readVal=gpio_i2c2_read(get_TW6874_SLAVE_ID0(),(unsigned short)index,1);
	//ReadI2C(get_TW6874_SLAVE_ID0(), 2, indexArr, 1, &b_readVal);

	return b_readVal;
}

void assert_reset_TW6874(void)
{
	// micro's pins for the reset lines are left as open-drain and a pull-up is attached to each line
	Delay(20);
	//RSTn_6874 = 0;
	Delay(40);
	//RSTn_6874 = 1;
	Delay(40);		
}

U8 dis(U8 v1, U8 v2)
{
	if(v1>v2)
		return v1-v2;
	else
		return v2-v1;
}

void TW6874_Init(void)
{
//	U8 i, r[4], h[4], val;
	DiracInput[0] = DiracInput[1] = DiracInput[2] = DiracInput[3] = 0;
	minErr[0]=minErr[1]=minErr[2]=minErr[3]=1;

#ifdef REG_I2C_DUMP
	bAutoEQlock = 0;
	bAutoCh[0] = bAutoCh[1] = bAutoCh[2] = bAutoCh[3] = 0;
#else
	bAutoCh[0] = bAutoCh[1] = bAutoCh[2] = bAutoCh[3] = 1;
	bAutoEQlock = 1;
#endif
	DiracAutoFlag = 1;
//	CLen = 1; // set to long cable by default;
	p_lock_in[0]=p_lock_in[1]=p_lock_in[2]=p_lock_in[3]=0;
	lock_in[0]=lock_in[1]=lock_in[2]=lock_in[3]=0;

	right_audio_chan = 0;

	//set to 1 indicating force to use LOCK for auto calibration even CRC info is available;
	Force_Lock = 0;
	ForceDisplay = 0;
//////////////////////////////////////////////		
	// Digital per Henry
	// Main reset
	SetSignalTW6874( 0x03A, 0x70, 0 );
	
	// Release reset
	SetSignalTW6874( 0x03A, 0x70, 0xFF );
	
	// VP1 output enable
//	SetSignalTW6874( 0x044, 0x70, 0x00 );	//disable display at init time;
	SetSignalTW6874( 0x044, 0x70, 0x80 );
//	SetSignalTW6874( 0x044, 0x70, 0x81 );	//8bit mode

	// VP2 output enable
//	SetSignalTW6874( 0x046, 0x70, 0x00 );	//disable display at init time;
	SetSignalTW6874( 0x046, 0x70, 0x80 );
//	SetSignalTW6874( 0x046, 0x70, 0x81 );	//8bit mode

	// VP3 output enable
//	SetSignalTW6874( 0x048, 0x70, 0x00 );	//disable display at init time;
	SetSignalTW6874( 0x048, 0x70, 0x80 );
//	SetSignalTW6874( 0x048, 0x70, 0x81 );	//8bit mode
	
	// VP4 output enable
//	SetSignalTW6874( 0x04A, 0x70, 0x00);	//disable display at init time;
	SetSignalTW6874( 0x04A, 0x70, 0x80 );
//	SetSignalTW6874( 0x04A, 0x70, 0x81 );	//8bit mode
	
	// disable CDR auto-reset
	SetSignalTW6874( 0x035, 0x70, 0x00 );

/*
	// Kin's SDI1 auto-process detect disable; ch0
	SetSignalTW6874( 0x080, 0x77, 0 );
	SetSignalTW6874( 0x082, 0x43, 1 );
	
	// Kin's SDI2 auto-process detect disable; ch1
	SetSignalTW6874( 0x086, 0x77, 0 );
	SetSignalTW6874( 0x088, 0x43, 1 );
	
	// Kin's SDI1 auto-process detect disable; ch2
	SetSignalTW6874( 0x090, 0x77, 0 );
	SetSignalTW6874( 0x092, 0x43, 1 );
	
	// Kin's SDI2 auto-process detect disable; ch3
	SetSignalTW6874( 0x096, 0x77, 0 );
	SetSignalTW6874( 0x098, 0x43, 1 );
*/
	// Yaron's audio settings
	SetSignalTW6874(0x11d,0x70,0x00);		//[4]:27MHz
	SetSignalTW6874(0x100,0x70,0x44);		//AIGAIN
	SetSignalTW6874(0x101,0x70,0x44);		//AIGAIN
	SetSignalTW6874(0x121,0x70,0x0f);		//AOGAIN
	SetSignalTW6874(0x10B,0x70,0xc1);       //
//	SetSignalTW6874(0x110,0x70,0x20);       //OUTSEL:AIN0 
	SetSignalTW6874(0x110,0x70,0x30);       //mix OUTSEL:AIN0 
	SetSignalTW6874(0x10B,0x70,0xc1);       //Audio Master ctrl
	SetSignalTW6874(0x130,0x70,0x04);		//audio clk control
	SetSignalTW6874(0x131,0x70,0x0);
	SetSignalTW6874(0x134,0x70,0x00);		//16bit mode;
	SetSignalTW6874(0x310,0x70,0x21);		//Mux always 16 channels;
	SetSignalTW6874(0x102,0x70,0x03);		//ADATR set to 16 channels;


	SetSignalTW6874( 0x081, 0x20, 3 ); // Changes per request from Wei, 6/11/2014
	SetSignalTW6874( 0x087, 0x20, 3 ); // Changes per request from Wei, 6/11/2014
	SetSignalTW6874( 0x091, 0x20, 3 ); // Changes per request from Wei, 6/11/2014
	SetSignalTW6874( 0x097, 0x20, 3 ); // Changes per request from Wei, 6/11/2014

	//Eric: for the 1st ASIC, reg56's bit-7 should be masked
	//      because it will block Dirac video output
//	SetSignalTW6874( 0x056, 0x77, 0 );
	//kb: added ; 05/07
	SetSignalTW6874(0x04E, 0x70, 0x11);
	SetSignalTW6874(0x054, 0x70, 0xf0);


	SetSignalTW6874(0x382, 0x74, 0x6);
	SetSignalTW6874(0x392, 0x74, 0x6);
	SetSignalTW6874(0x3a2, 0x74, 0x6);
	SetSignalTW6874(0x3b2, 0x74, 0x6);

	//kb: added 10/06
	SetSignalTW6874(0x080, 0x77, 0x1);
	SetSignalTW6874(0x082, 0x70, 0x11);
	SetSignalTW6874(0x086, 0x77, 0x1);
	SetSignalTW6874(0x088, 0x70, 0x11);
	SetSignalTW6874(0x090, 0x77, 0x1);
	SetSignalTW6874(0x092, 0x70, 0x11);
	SetSignalTW6874(0x096, 0x77, 0x1);
	SetSignalTW6874(0x098, 0x70, 0x11);

	SetSignalTW6874(0x085, 0x70, 0x2);
	SetSignalTW6874(0x08b, 0x70, 0x2);
	SetSignalTW6874(0x095, 0x70, 0x2);
	SetSignalTW6874(0x09b, 0x70, 0x2);

/*
	SetSignalTW6874(0x382, 0x74, 0xF);	// Per request from Paul for checkfield	
	SetSignalTW6874(0x392, 0x74, 0xF);	// Per request from Paul for checkfield
	SetSignalTW6874(0x3A2, 0x74, 0xF);	// Per request from Paul for checkfield	
	SetSignalTW6874(0x3B2, 0x74, 0xF);	// Per request from Paul for checkfield
*/

//	Printf("\r\nPlease Make Sure All Cables Unplugged!!\r\n");
	Delay(1000);
	SetSignalTW6874( 0x383, 0x70, 0x80);
	SetSignalTW6874( 0x385, 0x70, 0);
	SetSignalTW6874( 0x386, 0x70, 0);
	SetSignalTW6874( 0x393, 0x70, 0x80);
	SetSignalTW6874( 0x395, 0x70, 0);
	SetSignalTW6874( 0x396, 0x70, 0);
	SetSignalTW6874( 0x3a3, 0x70, 0x80);
	SetSignalTW6874( 0x3a5, 0x70, 0);
	SetSignalTW6874( 0x3a6, 0x70, 0);
	SetSignalTW6874( 0x3b3, 0x70, 0x80);
	SetSignalTW6874( 0x3b5, 0x70, 0);
	SetSignalTW6874( 0x3b6, 0x70, 0);


// enable ANC here for resolution info from TW6872, DID reserved at 0x80;
	ReadANC_On(0, 0x80, 0x00);
	ReadANC_On(1, 0x80, 0x00);
	ReadANC_On(2, 0x80, 0x00);
	ReadANC_On(3, 0x80, 0x00);

	Printf("\r\nInitializing TW6874 is done !!\r\n");
}

void TW6874_Ch0(U8 mode)
{
	// 1 Set speed
	// not used
	SetSignalTW6874(0x80, 0x30, 0xf);

	// 6 On-chip regulator
//	SetSignalTW6874( 0x081, 0x20, 7 ); // Changes per request from Wei, 6/11/2014
	SetSignalTW6874( 0x081, 0x73, 0 );
		

	// 14 CDR Test
//	SetSignalTW6874( 0x085, 0x70, 0 );

	// 15 CDR mode2
	SetSignalTW6874( 0x084, 0x70, 0x40 );

	// 16 CDR mode1
	SetSignalTW6874( 0x083, 0x70, mode ? 0x06 : 0x0A );

	// 17 CDR reset
	SetSignalTW6874( 0x080, 0x55, 0 );

	if(mode)
		SetSignalTW6874( 0x080, 0x50, 0x30);
	
	SetSignalTW6874( 0x080, 0x40, 0x0);
	
}
	
void TW6874_Ch1(U8 mode)
{
	// 1 Set speed
	// not used
	SetSignalTW6874(0x86, 0x30, 0xf);
	
	// 6 On-chip regulator
//	SetSignalTW6874( 0x087, 0x20, 7 ); // Increase on-chip regulator voltage as requested by Wei, 6/11/2014
	SetSignalTW6874( 0x087, 0x73, 0 );	
			
	// 14 CDR Test
//	SetSignalTW6874( 0x08B, 0x70, 0 );
	
	// 15 CDR mode2
	SetSignalTW6874( 0x08A, 0x70, 0x40 );
	
	// 16 CDR mode1
	SetSignalTW6874( 0x089, 0x70, mode ? 0x06 : 0x0A );
	
	// 17 CDR reset
	SetSignalTW6874( 0x086, 0x55, 0 );

	if(mode)
		SetSignalTW6874( 0x086, 0x50, 0x30);
	
	SetSignalTW6874( 0x086, 0x40, 0x0);

}

#if 1
void TW6874_Ch2(U8 mode)
{
	// 1 Set speed
	// not used
	SetSignalTW6874(0x90, 0x30, 0xf);

	// 6 On-chip regulator
//	SetSignalTW6874( 0x091, 0x20, 7 ); // Increase on-chip regulator voltage as requested by Wei, 6/11/2014
	SetSignalTW6874( 0x091, 0x73, 0 );	
		
	// 14 CDR Test
//	SetSignalTW6874( 0x095, 0x70, 0 );

	// 15 CDR mode2
	SetSignalTW6874( 0x094, 0x70, 0x40 );

	// 16 CDR mode1
	SetSignalTW6874( 0x093, 0x70, mode ? 0x06 : 0x0A );

	// 17 CDR reset
	SetSignalTW6874( 0x090, 0x55, 0 );

	if(mode)
		SetSignalTW6874( 0x090, 0x50, 0x30);
	
	SetSignalTW6874( 0x090, 0x40, 0x0);

}
	
void TW6874_Ch3(U8 mode)
{
	// 1 Set speed
	// not used
	SetSignalTW6874(0x96, 0x30, 0xf);
	
	// 6 On-chip regulator
//	SetSignalTW6874( 0x097, 0x20, 7 ); // Increase on-chip regulator voltage as requested by Wei, 6/11/2014
	SetSignalTW6874( 0x097, 0x73, 0 );			
	
	// 14 CDR Test
//	SetSignalTW6874( 0x09B, 0x70, 0 );
	
	// 15 CDR mode2
	SetSignalTW6874( 0x09A, 0x70, 0x40 );
	
	// 16 CDR mode1
	SetSignalTW6874( 0x099, 0x70, mode ? 0x06 : 0x0A );
	
	// 17 CDR reset
	SetSignalTW6874( 0x096, 0x55, 0 );

	if(mode)
		SetSignalTW6874( 0x096, 0x50, 0x30);
	
	SetSignalTW6874( 0x096, 0x40, 0x0);

}

#endif

void TW6874_Ch(U8 channel, U8 mode)
{
	if (mode > 1)
	{
		Printf("\r\nERROR: invalid mode - TW6874 SDI%d configuration. 0=HD/3G, 1=SD\r\n", (WORD)channel);
		return;
	}
	switch (channel-1)
	{
		case 0:
			TW6874_Ch0(mode);	 
			Printf("\r\n[ch0]Configured TW6874 SDI1 for %s", mode ? "SD" : "HD/3G");
			break;
		case 1:
			TW6874_Ch1(mode);
			Printf("\r\n[ch1]Configured TW6874 SDI2 for %s", mode ? "SD" : "HD/3G");
			break;
		case 2:
			TW6874_Ch2(mode);
			Printf("\r\n[ch2]Configured TW6874 SDI3 for %s", mode ? "SD" : "HD/3G");
			break;
		case 3:
			TW6874_Ch3(mode);
			Printf("\r\n[ch3]Configured TW6874 SDI4 for %s", mode ? "SD" : "HD/3G");
			break;
		default:			
			Printf("\r\nTW6874 SDI Channel Configuration for SD/HD/3G\r\n");			
			Printf("Usage:\r\n >SDI <ch> <mode>\r\n");
			Printf(" ><ch> .... 1, 2, 3, 4\r\n");
			Printf(" ><mode> .... 0=HD/3G, 1=SD\r\n");
			Printf(" >SDI 1 0 .... Set SDI1 for HD/3G\r\n");
			Printf(" >SDI 1 1 .... Set SDI1 for SD\r\n");
			Printf(" >SDI 4 0 .... Set SDI4 for HD/3G\r\n");
			return;
	}
}

u8 get_TW6874_SLAVE_ID0()
{
#ifdef DEBUG_SetSignalTW6874
	return FPGA_SLAVE_ID0;	// FIXME
#else
	return TW6874_SLAVE_ID0;//|I2C_ADDR;
#endif
}



//Eric change
U8 SDI_locked( U8 ch )
{
	u8 val, v_lock;	// b4, b6, b7, v_lock, d_lock;
	volatile U8 i;
	
	v_lock = 0;
//	d_lock = 0;
	for (i = 0; i < 16; i++)
	{
		val = ReadTW6874( ch << 1 );
//		b7  = (val >> 7) & 0x01;
//		b6  = (val >> 6) & 0x01;
//		b4  = (val >> 4) & 0x01;	//for CRC checking, but some SDI cameras don't provide CRC info
		
		if ( (val >> 6) & 0x01 )			v_lock++;	//for normal video lock checking
//		if ( !b7 && b6 )	d_lock++;	//for Dirac unplug/plug detection
	}
	
//	if ( DiracInput[ch] && (d_lock == 16) )	{DiracInput[ch] = 2;  return(2);}
//	else 
	if ( v_lock == 16 )				return(1);
	else							return(0);	
}
//Eric
/*
U8 CRC_error( U8 channel )
{
	return (ReadTW6874( (channel-1)<<1 )&0x50) == 0x50 ? 1 : 0;
}
*/

//Eric change
U8 isInterlaced( U8 channel )
{
	return (ReadTW6874( channel<<1 ) & 0x20) == 0x20 ? 0 : 1;
}
//Eric

format_t format_tb[] = {
  {0xa0, RES_1080P60}, //1080p59.94
  {0xb0, RES_1080P60}, //1080p60
  {0x80, RES_1080P50}, //1080p48
  {0x90, RES_1080P50}, //1080p50
  {0x51, RES_720P25},  //kb: added for 720P25;
  {0x91, RES_720P50},  //720p50
  {0x61, RES_720P30},	 //kb: added for 720P29.97;
  {0x71, RES_720P30},	 //kb: added for 720P30;
  {0xa1, RES_720P60},  //720p59.94
  {0xb1, RES_720P60},  //720p60
  {0x70, RES_1080I60}, //1080i60
  {0x50, RES_1080I50}, //1080i50
  {0x60, RES_1080P30}, //1080p29.97
  {0x70, RES_1080P30}, //1080p30
  {0x20, RES_1080P25}, //1080p24
  {0x30, RES_1080P25}, //1080p24
  {0x50, RES_1080P25}, //1080p25
  {0x68, RES_NTSC},    //NTSC29.97
  {0x59, RES_PAL},     //PAL50
  {0xFF, RES_NONE}
};


//Eric change
U8 getVideoFormat(u8 channel)
{
	u8 i, val, std;
	
	std = RES_NONE;
	val = ReadTW6874( (channel << 1) + 1);
	
	i=0;
	while(format_tb[i].val != 0xFF)
	{
		if(val == format_tb[i].val) 
		{
			std = format_tb[i].std;
			break;
		}
		i++;	
	}
	
	if (val == 0x70)
	{
		if (isInterlaced(channel)) 	std = RES_1080I60;
		else						std = RES_1080P30;
	}
	else if (val == 0x50)
	{
		if (isInterlaced(channel)) 	std = RES_1080I50;
		else						std = RES_1080P25;
	}
	
/*	if(((std==RES_NTSC)||(std==RES_PAL))&&(DiracAutoFlag==1)&&((ReadTW6874(0x54)&(1<<channel))==0)){	// if dirac auto on, then enable dirac if SD is the input;
//	Printf("\n\r: SD plugged %2x %2x %2x %2x", (WORD)channel, (WORD)std, (WORD)(DiracAutoFlag), (WORD)((ReadTW6874(0x54)&(1<<channel))));
		switch(channel){
		case 0:
			set_dirac_func( 1, ReadTW6874(0x55)&0x7, ReadTW6874(0x83) );
			break;
		case 1:
			set_dirac_func( 2, (ReadTW6874(0x55)>>4)&0x7, ReadTW6874(0x89) );
			break;
		case 2:
			set_dirac_func( 3, ReadTW6874(0x56)&0x7, ReadTW6874(0x93) );
			break;
		case 3:
			set_dirac_func( 4, (ReadTW6874(0x56)>>4)&0x7, ReadTW6874(0x99) );
			break;
		}

		if(bAutoCh[channel]==1)
			CheckChLock(channel);
	//		lock_auto(channel);
	} else if((((std!=RES_NTSC)&&(std!=RES_PAL)))&&(DiracAutoFlag==1)&&(((ReadTW6874(0x54)&(1<<channel))!=0))){
//	Printf("\n\r: SD un plugged %2x %2x %2x %2x", (WORD)channel, (WORD)std, (WORD)(DiracAutoFlag), (WORD)((ReadTW6874(0x54)&(1<<channel))));
		switch(channel){
		case 0:
			set_dirac_func( 1, 8, ReadTW6874(0x83) );
			break;
		case 1:
			set_dirac_func( 2, 8, ReadTW6874(0x89) );
			break;
		case 2:
			set_dirac_func( 3, 8, ReadTW6874(0x93) );
			break;
		case 3:
			set_dirac_func( 4, 8, ReadTW6874(0x99) );
			break;
		}		
	}
*/
	return std;
}
//Eric

void reset_WC_SDI1(void)
{
	SetSignalTW6874( 0x03A, 0x00, 0xEE );
	SetSignalTW6874( 0x03A, 0x70, 0xFF );
	SetSignalTW6874( 0x03D, 0x70, 0xEE );
	SetSignalTW6874( 0x080, 0x60, 0x10 );
	SetSignalTW6874( 0x080, 0x60, 0x30 );
	SetSignalTW6874( 0x03D, 0x70, 0xFF );
}

//Eric change
U8 check_tw6874_input_source(u8 ch)
{
	u8 itype=RES_NONE, val, fmt, res ;//check_lock;
	volatile U8 i;

	//check_lock=0;

	lock_in[ch] = ReadTW6874(1+(ch<<1));
	if((bAutoCh[ch]==1)){
/*
		if(((p_lock_in[ch]&0x7f)!=(lock_in[ch]&0x7f))&&((p_lock_in[ch]&0x44)==(lock_in[ch]&0x44)))
			lock_auto(ch);
		else
*/
		if((p_lock_in[ch])!=(lock_in[ch])){
//		if(((p_lock_in[ch])!=(lock_in[ch]))||(((ReadTW6874(ch<<1)&0xc5)==0xc5)&&(DiracInput[ch]==0)&&DiracAutoFlag)){
			minErr[ch] = 1;
//			check_lock=1;
		}
			CheckChLock(ch);
		p_lock_in[ch] = lock_in[ch];

//		lock_auto(ch);
//		CheckChLock(ch);
	}
/*
					while(RS_ready())
					{
						Monitor();
					}
*/
	val = SDI_locked(ch);

	fmt = ReadTW6874(0x055 + (ch >> 1));
	if (ch == 0 || ch == 2)		res = fmt & 0x07;
	else						res = (fmt >> 4) & 0x07;

	if (val == 0){
		if((DiracAutoFlag)&&((DiracInput[ch]!=0)||((ReadTW6874(0x054)>>ch)&1))){	//if dirac was enabled, disable it;
//Printf("\n\r: unplugged, disable dirac ...");
			switch(ch){
			case 0:
				set_dirac_func( 1, 8, ReadTW6874(0x83) );
				break;
			case 1:
				set_dirac_func( 2, 8, ReadTW6874(0x89) );
				break;
			case 2:
				set_dirac_func( 3, 8, ReadTW6874(0x93) );
				break;
			case 3:
				set_dirac_func( 4, 8, ReadTW6874(0x99) );
				break;
			}	
		}

#ifdef OUTPUT_AUTO_OFF
		if ( (ReadTW6874(0x044+(ch<<1))>>7)&1 )		SetSignalTW6874( 0x044 + (ch<<1), 0x77, 0 );		// turn off output if it's on;
#endif
		return RES_NONE;
	}
	else
	{
//		if (DiracInput[ch])
		if (DiracInput[ch]||(((ReadTW6874(ch<<1)&0xc7)==0xc5)&&(DiracInput[ch]==0)&&DiracAutoFlag))	// even though DiracInput[ch] is 0, still check if it is Dirac case if it is SD input and  DiracAutoFlag is set
		{
//			if(DiracAutoFlag)
//				itype = getVideoFormat(ch);

			// keep this temporarily to have a way for res setting  from host.
			if (res == 0)		itype = RES_720P60;
			else if (res == 1)	itype = RES_720P50;
			else if (res == 2)	itype = RES_1080I60;
			else if (res == 3)	itype = RES_1080I50;
			else if (res == 6)	itype = RES_1080P30;
			else if (res == 7)	itype = RES_1080P25;

			switch  ((ReadTW6874((ch<<1)+1))&0x0f){
				case 8:		//NTSC
					if((ReadTW6874(ch<<1)&0x20)==0){	// I-->720P
						itype = RES_720P60;
						res = DIRAC_RES_720P60;
					}
					else{
						itype = RES_1080P30;
						res = DIRAC_RES_1080P30;
					}
					break;
				case 9:
					if((ReadTW6874(ch<<1)&0x20)==0){	// I-->720P
						itype = RES_720P50;
						res = DIRAC_RES_720P50;
					}
					else{
						itype = RES_1080P25;
						res = DIRAC_RES_1080P25;
					}
					break;
				default:
					break;
			}
			
			i = ReadANC_Resolution(ch);
			if(i<=RES_720P30){
				itype=i;
			}

			if(ch<2)
				SetSignalTW6874( 0x055, (((ch<<2)+3)<<4)+(ch<<2), res );	//ch_1 Dirac output resolution
			else
				SetSignalTW6874( 0x056, ((((ch-2)<<2)+3)<<4)+((ch-2)<<2), res );	//ch_1 Dirac output resolution

//			if((DiracAutoFlag)&&(!((ReadTW6874(0x054)>>ch)&1))){	//if dirac was disabled, enable it;
			if(DiracAutoFlag&&((!((ReadTW6874(0x054)>>ch)&1))||((ReadTW6874(ch<<1)&0x80)==0))){	//if dirac was disabled/status changed, enable it;
				switch(ch){
				case 0:
					set_dirac_func( 1, res, ReadTW6874(0x83) );
					break;
				case 1:
					set_dirac_func( 2, res, ReadTW6874(0x89) );
					break;
				case 2:
					set_dirac_func( 3, res, ReadTW6874(0x93) );
					break;
				case 3:
					set_dirac_func( 4, res, ReadTW6874(0x99) );
					break;
				}
			}
		}
		else
		{
			if((DiracAutoFlag)&&((ReadTW6874(0x054)>>ch)&1)){	//if dirac was enabled, disable it;
				switch(ch){
				case 0:
					set_dirac_func( 1, 8, ReadTW6874(0x83) );
					break;
				case 1:
					set_dirac_func( 2, 8, ReadTW6874(0x89) );
					break;
				case 2:
					set_dirac_func( 3, 8, ReadTW6874(0x93) );
					break;
				case 3:
					set_dirac_func( 4, 8, ReadTW6874(0x99) );
					break;
				}
			}

			itype = getVideoFormat(ch);
			for (i = 0; i < 15; i++)
			{
				if (itype != getVideoFormat(ch)){
					if ( (ReadTW6874(0x044+(ch<<1))>>7)&1 )		SetSignalTW6874( 0x044 + (ch<<1), 0x77, 0 );		// turn off output if it's on;
					return RES_NONE;
				}
			}

		}
	}
	
#ifdef OUTPUT_AUTO_OFF
	if ( !((ReadTW6874(0x044+(ch<<1))>>7)&1) )		
		SetSignalTW6874( 0x044 + (ch<<1), 0x77, 1 );			// turn on output if it's off;
#endif
	return (itype);
}
//Eric


void AocSet(WORD channels)
{
	U8 ii, ch, v; // jj;
	WORD c;

	c = channels;
	for (ii=0;ii<4;ii++)
	{
		ch = c&0xf;
		c = c>>4;
		if ((ch>0)&&(ch<5)){
//			for (jj=0; jj<3; jj++){
			SetSignalTW6874( 0x383+((ch-1)<<4), 0x70, 0x80);

			SetSignalTW6874( 0x385+((ch-1)<<4), 0x70, 0x00);
			SetSignalTW6874( 0x386+((ch-1)<<4), 0x70, 0x00);
			SetSignalTW6874( 0x387+((ch-1)<<4), 0x70, 0x02);
			Delay(20);
			v=ReadTW6874( 0x38c+((ch-1)<<4));
//			if (v&3 == 1){
//				break;
//			}
//			}

			SetSignalTW6874( 0x383+((ch-1)<<4), 0x70, 0x00);			
			if ((v & 3) == 1) {
//				Printf("\n\r: channel %2x AOC calibration done\n\r", (WORD)ch);
				v = ReadTW6874(0x38b + ((ch-1)<<4));
				SetSignalTW6874(0x385 + ((ch-1)<<4), 0x70, (v & 0x7F) | 0x80);
			} else {
				Printf("\n\r: channel %2x AOC calibration failed\n\r", (WORD)(ch+1));
				SetSignalTW6874( 0x385+((ch-1)<<4), 0x70, 0xBF);
			}
		}
	}
}

void CrcCount(WORD channels)
{
	// U8 range, addr;
	WORD val;
	U8 ii, ch;
	WORD c;

	c = channels;
	for (ii=0;ii<4;ii++)
	{
		ch = c&0xf;
		c = c>>4;
		if ((ch>0)&&(ch<5)){
				ch = ch - 1;
/////////////////////////////////////////////////////
			if(ch<2)
				val=0x80+ch*6;
			else
				val=0x90+(ch-2)*6;
			SetSignalTW6874(val,0x60,0x30);
			SetSignalTW6874(val,0x60,0x20);
			Delay(2);

			val=ReadTW6874(ch<<1);
//			Printf("%4x/", (WORD)val);
			if((val&0x3)==1){
				if((ANCDetection(ch,0xf4)==1)&&((val&0x80)==0x80))
					val=CheckChCrc(ch, 750);
				else if(DiracAutoFlag==0)
					val=CheckChCrc(ch, 750);
				else
					val=0xff;
			}
			else
				val=CheckChCrc(ch, 750);

////////////////////////////////////////////////////
//				err = CheckChCrc(ch, 750);
				Printf("\n\r: channel %2x crc count = %4x\n\r", (WORD)(ch+1), (WORD)val); 
		}
	}
}



void set_dirac_func( U8 ch, U8 res, U8 mode )
{
	U8 port, ii, jj;
	U16 val;
	
	port = ch - 1;
/*	
	if(res==8){
//			bAutoCh[port]=0; 
				// set to pre-calibrated AFC/AOC if they are not yet;
				if(ReadTW6874(0x383 + (port<<4)) != 0)
					SetSignalTW6874( 0x383+(port<<4), 0x70, 0);
				SetSignalTW6874( 0x385+(port<<4), 0x70, calibratedAOC[port]|0x80);
//				minErr[ch] = 0x7f00;
				minErr[ch] = 1;
	}
	else{
			bAutoCh[port]=1; 
	}
	bAutoEQlock=bAutoCh[0]|bAutoCh[1]|bAutoCh[2]|bAutoCh[3];	
*/


//	if (port == 0)
	if ((port>=0)&&(port<4))
	{
		SetSignalTW6874( 0x054, (port<<4)+port, 0x00 );	//ch_1 Dirac disable
		SetSignalTW6874( 0x03D, ((port+4)<<4)+port+4, 0x00 );	//ch_1 CDR148 reset
		SetSignalTW6874( 0x03A, (port<<4)+port, 0x00 );	//ch_1 audio AFE and SDI receiver reset
		SetSignalTW6874( 0x03A, (port<<4)+port, 0x01 );	//ch_1 audio AFE and SDI receiver normal operation
		
		if (res == 8)
		{

			DiracInput[port] = 0;
//			SetSignalTW6874( 0x054, (port<<4)+port, 0x00 );	//ch_1 Dirac disable
			TW6874_Ch( port+1, mode );				//ch_1 set SD/HD/3G mode (havn't finished)
			
			Printf("\r\nDirac status: ch[1]:%d, ch[2]:%d, ch[3]:%d, ch[4]:%d; ch=%2x, mode=%2x, res=%2x", 
				   (WORD)DiracInput[0], (WORD)DiracInput[1], (WORD)DiracInput[2], (WORD)DiracInput[3], (WORD)port, (WORD)mode, (WORD)res);
		}
		else
		{
			val=0;
			for(ii=0;ii<50;ii++){
				jj=ReadTW6874(port<<1);
				if((jj&0x47) != 0x45){ 
//					Printf("\n port=%2x, status %2x", (WORD)port, (WORD)jj); 
					val++;
				}
				Delay(1);
			}
			if(val<2){	// make sure dirac signal is stable here;
				DiracInput[port] = 1;
				TW6874_Ch( port+1, 1 );
				
				Printf("\r\nDirac status: ch[1]:%d, ch[2]:%d, ch[3]:%d, ch[4]:%d", 
					   (WORD)DiracInput[0], (WORD)DiracInput[1], (WORD)DiracInput[2], (WORD)DiracInput[3]);
			} else{
				Printf("dirac signal is not stable! ch=%2x", (WORD)port);
				DiracInput[port] = 0;
	//			SetSignalTW6874( 0x054, (port<<4)+port, 0x00 );	//ch_1 Dirac disable
				TW6874_Ch( port+1, mode );				//ch_1 set SD/HD/3G mode (havn't finished)
				
			}
		}
		
		SetSignalTW6874( 0x03D, ((port+4)<<4)+port+4, 0x01 );	//ch_1 CDR148 normal operation
		//kb: revB ?? SetSignalTW6874( 0x070, 0x10, 0x00 );	//ch_1 video port in normal mode (non-Dirac)
		if(port<2){
			SetSignalTW6874( 0x080+(port*6), 0x60, 0x30 );	//ch_1 CDR reset(bit-5)
			SetSignalTW6874( 0x080+(port*6), 0x60, 0x20 );	//ch_1 PLL-DIG reset (bit-4: set LO for normal operation)
		}
		else{
			SetSignalTW6874( 0x090+((port-2)*6), 0x60, 0x30 );	//ch_1 CDR reset(bit-5)
			SetSignalTW6874( 0x090+((port-2)*6), 0x60, 0x20 );	//ch_1 PLL-DIG reset (bit-4: set LO for normal operation)
		}

		if (DiracInput[port])
		{
			Ms_Delay(15);
			for(ii=0;ii<300;ii++){		//check if status is good
				val = ReadTW6874((port<<1));
				if((val&0x47)==0x45)
					break;
			}

/*
			val = ReadTW6874(port<<1);
			for (ii=0;ii<32;ii++){
		 		if((val&3)==1)
					break;
		 		val = ReadTW6874(port<<1);
				Delay(2);
			}
*/
			if(port<2)
				SetSignalTW6874( 0x055, (((port<<2)+3)<<4)+(port<<2), res );	//ch_1 Dirac output resolution
			else
				SetSignalTW6874( 0x056, ((((port-2)<<2)+3)<<4)+((port-2)<<2), res );	//ch_1 Dirac output resolution

			SetSignalTW6874( 0x054, (port<<4)+port, 0x01 );	//ch_1 Dirac enable
		}		
//		ReadTW6874( 0x00 );
//		ReadTW6874( 0x00 );
		
	}
/*
	else if (port == 1)
	{
		SetSignalTW6874( 0x054, 0x11, 0x00 );	//ch_2 Dirac disable
		SetSignalTW6874( 0x03D, 0x55, 0x00 );	//ch_2 CDR148 reset
		SetSignalTW6874( 0x03A, 0x11, 0x00 );	//ch_2 audio AFE and SDI receiver reset
		SetSignalTW6874( 0x03A, 0x11, 0x01 );	//ch_2 audio AFE and SDI receiver normal operation
		
		if (res == 8)
		{
			DiracInput[1] = 0;
			SetSignalTW6874( 0x054, 0x11, 0x00 );	//ch_2 Dirac disable
			TW6874_Ch( port+1, mode );				//ch_2 set SD/HD/3G mode (havn't finished)
			
			Printf("\r\nDirac status: ch[1]:%d, ch[2]:%d, ch[3]:%d, ch[4]:%d", 
				   (WORD)DiracInput[0], (WORD)DiracInput[1], (WORD)DiracInput[2], (WORD)DiracInput[3]);
		}
		else
		{
			DiracInput[1] = 1;
			TW6874_Ch( port+1, 1 );
			
			Printf("\r\nDirac status: ch[1]:%d, ch[2]:%d, ch[3]:%d, ch[4]:%d", 
				   (WORD)DiracInput[0], (WORD)DiracInput[1], (WORD)DiracInput[2], (WORD)DiracInput[3]);
		}
		
		SetSignalTW6874( 0x03D, 0x55, 0x01 );	//ch_2 CDR148 normal operation
		//kb: revB ?? SetSignalTW6874( 0x070, 0x32, 0x00 );	//ch_2 video port in normal mode (non-Dirac)
		SetSignalTW6874( 0x086, 0x60, 0x30 );	//ch_2 CDR reset(bit-5)
		SetSignalTW6874( 0x086, 0x60, 0x20 );	//ch_2 PLL-DIG reset (bit-4: set LO for normal operation)
		
		if (DiracInput[1])
		{
			val = ReadTW6874(0x02);
			for (ii=0;ii<32;ii++){
		 		if((val&3)==1)
					break;
		 		val = ReadTW6874(0x02);
				Delay(2);
			}

			SetSignalTW6874( 0x055, 0x64, res );	//ch_2 Dirac output resolution
			SetSignalTW6874( 0x054, 0x11, 0x01 );	//ch_2 Dirac enable
		}
		ReadTW6874( 0x02 );
		ReadTW6874( 0x02 );
	}
	else if (port == 2)
	{
		SetSignalTW6874( 0x054, 0x22, 0x00 );	//ch_3 Dirac disable
		SetSignalTW6874( 0x03D, 0x66, 0x00 );	//ch_3 CDR148 reset
		SetSignalTW6874( 0x03A, 0x22, 0x00 );	//ch_3 audio AFE and SDI receiver reset
		SetSignalTW6874( 0x03A, 0x22, 0x01 );	//ch_3 audio AFE and SDI receiver normal operation
		
		if (res == 8)
		{
			DiracInput[2] = 0;
			SetSignalTW6874( 0x054, 0x22, 0x00 );	//ch_3 Dirac disable
			TW6874_Ch( port+1, mode );				//ch_3 set SD/HD/3G mode (havn't finished)
			
			Printf("\r\nDirac status: ch[1]:%d, ch[2]:%d, ch[3]:%d, ch[4]:%d", 
				   (WORD)DiracInput[0], (WORD)DiracInput[1], (WORD)DiracInput[2], (WORD)DiracInput[3]);
		}
		else
		{
			DiracInput[2] = 1;
			TW6874_Ch( port+1, 1 );
			
			Printf("\r\nDirac status: ch[1]:%d, ch[2]:%d, ch[3]:%d, ch[4]:%d", 
				   (WORD)DiracInput[0], (WORD)DiracInput[1], (WORD)DiracInput[2], (WORD)DiracInput[3]);
		}
		
		//SetSignalTW6874( 0x092, 0x70, 0x00 );	//ch_3 EQ/CDR Control Byte 6 is set to init value
		//SetSignalTW6874( 0x095, 0x70, 0x74 );	//ch_3 EQ/CDR Control Byte 9 is set to init value
		
		SetSignalTW6874( 0x03D, 0x66, 0x01 );	//ch_3 CDR148 normal operation
		//kb: revB ?? SetSignalTW6874( 0x070, 0x10, 0x00 );	//ch_1 video port in normal mode (non-Dirac)
		SetSignalTW6874( 0x090, 0x60, 0x30 );	//ch_1 CDR reset(bit-5)
		SetSignalTW6874( 0x090, 0x60, 0x20 );	//ch_1 PLL-DIG reset (bit-4: set LO for normal operation)
		//SetSignalTW6874( 0x070, 0x54, 0x00 );	//ch_3 video port in normal mode (non-Dirac)
		//SetSignalTW6874( 0x08C, 0x70, 0xB0 );	//ch_3 CDR reset(bit-5)
		//SetSignalTW6874( 0x08C, 0x70, 0xA0 );	//ch_3 PLL-DIG reset (bit-4: set LO for normal operation)
		
		if (DiracInput[2])
		{
			val = ReadTW6874(0x04);
			for (ii=0;ii<32;ii++){
		 		if((val&3)==1)
					break;
		 		val = ReadTW6874(0x04);
				Delay(2);
			}

			SetSignalTW6874( 0x056, 0x20, res );	//ch_3 Dirac output resolution
			SetSignalTW6874( 0x054, 0x22, 0x01 );	//ch_3 Dirac enable
		}
		ReadTW6874( 0x04 );
		ReadTW6874( 0x04 );
	}
	else if (port == 3)
	{
		SetSignalTW6874( 0x054, 0x33, 0x00 );	//ch_4 Dirac disable
		SetSignalTW6874( 0x03D, 0x77, 0x00 );	//ch_4 CDR148 reset
		SetSignalTW6874( 0x03A, 0x33, 0x00 );	//ch_4 audio AFE and SDI receiver reset
		SetSignalTW6874( 0x03A, 0x33, 0x01 );	//ch_4 audio AFE and SDI receiver normal operation
		
		if (res == 8)
		{
			DiracInput[3] = 0;
			SetSignalTW6874( 0x054, 0x33, 0x00 );	//ch_4 Dirac disable
			TW6874_Ch( port+1, mode );				//ch_4 set SD/HD/3G mode (havn't finished)
			
			Printf("\r\nDirac status: ch[1]:%d, ch[2]:%d, ch[3]:%d, ch[4]:%d", 
				   (WORD)DiracInput[0], (WORD)DiracInput[1], (WORD)DiracInput[2], (WORD)DiracInput[3]);
		}
		else
		{
			DiracInput[3] = 1;
			TW6874_Ch( port+1, 1 );
			
			Printf("\r\nDirac status: ch[1]:%d, ch[2]:%d, ch[3]:%d, ch[4]:%d", 
				   (WORD)DiracInput[0], (WORD)DiracInput[1], (WORD)DiracInput[2], (WORD)DiracInput[3]);
		}
		
		//SetSignalTW6874( 0x09C, 0x70, 0x00 );	//ch_4 Control Byte 6 is set to init value
		//SetSignalTW6874( 0x09F, 0x70, 0x74 );	//ch_4 Control Byte 9 is set to init value

		SetSignalTW6874( 0x03D, 0x77, 0x01 );	//ch_4 CDR148 normal operation
		//kb: revB ?? SetSignalTW6874( 0x070, 0x32, 0x00 );	//ch_2 video port in normal mode (non-Dirac)
		SetSignalTW6874( 0x096, 0x60, 0x30 );	//ch_2 CDR reset(bit-5)
		SetSignalTW6874( 0x096, 0x60, 0x20 );	//ch_2 PLL-DIG reset (bit-4: set LO for normal operation)
		//SetSignalTW6874( 0x070, 0x76, 0x00 );	//ch_4 video port in normal mode (non-Dirac)
		//SetSignalTW6874( 0x096, 0x70, 0xB0 );	//ch_4 CDR reset(bit-5)
		//SetSignalTW6874( 0x096, 0x70, 0xA0 );	//ch_4 PLL-DIG reset (bit-4: set LO for normal operation)
		
		if (DiracInput[3])
		{

			val = ReadTW6874(0x06);
			for (ii=0;ii<32;ii++){
		 		if((val&3)==1)
					break;
		 		val = ReadTW6874(0x06);
				Delay(2);
			}

			SetSignalTW6874( 0x056, 0x64, res );	//ch_4 Dirac output resolution
			SetSignalTW6874( 0x054, 0x33, 0x01 );	//ch_4 Dirac enable
		}
		ReadTW6874( 0x06 );
		ReadTW6874( 0x06 );
	}
*/
	else
	{
		Printf("\r\n Not correct input about Direc setting\r\n");
	}
}

////////////////////////////////////////////////////////////////
// static U16 err_thre = 0;
//static U16 goodAGC;

//static U16 unlock[4] = {0,0,0,0};

U16 CheckChCrc(U8 ch, U16 period) //, U8 addr, U8 range, U8 type)
{
	U16 kk, val, err, errCnt;
	U8 range, type;

	range = ((4+ch)<<4)+(4+ch);
	val = ReadTW6874(ch<<1);
	if((val & 0x44) != 0x44) // Not locked. Return big value.
		return 0x7F00;
	type = val & 0x03;


	// Clear CRC Counter
	if ((type) == 1) { // SD
		SetSignalTW6874(0x038, range, 1);
		SetSignalTW6874(0x038, range, 0);	
	} else { 		  // HD		
		SetSignalTW6874(0x039, 0x70, 0xFF);
		SetSignalTW6874(0x039, 0x70, 0x00);
	}
	// Turn on CRC counter
	SetSignalTW6874( 0x039, range, 1); 


	errCnt = 0;
	for (kk=0; kk<period; kk++) {	// To fine tune this number.
//	for (kk=0; kk<750; kk++) {	// To fine tune this number.

		Delay(5);
		val = ReadTW6874(ch<<1);
		if ((val & 0x44) != 0x44)	// Not locked
			return 0x7F00;
		if ((val & 0x03) != type)	// type changing
			return 0x7F00;
		errCnt += (val&0x10) >> 4;
	}
	SetSignalTW6874( 0x039, range, 0); // Stop CRC counter
	err=ReadTW6874(0x40+ch);


//Printf("\n\r(checkchcrc): err=%4x;  errCnt=%4x",(WORD)err, (WORD)errCnt);	
	if ((errCnt == 0) && (err == 0))
		return 0;

	val = (err>errCnt? err:errCnt);
	if(err==0xff)
		val = err+errCnt;
	else if((err == 0) && (errCnt>0))
		// val = 0x7e00;
		val = 0x200;


	return val;
	
}

///
//#define LOCK_PER_TH	100
//#define LOCK_DELAY 0
#define LOOP_COUNT 30


U8 ANCDetection(U8 ch, U8 did)
{
//	U8 rDid, rSDid, rDc;
	U16 val, ii;

	//disable and clear fifo
	SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  2);

	//configure and enable
	SetSignalTW6874( 0x806+((U16)ch<<8), 0x70,  did);
/*	
	if(line>0){	//line number specified; //only take one configuration here;
		SetSignalTW6874( 0x810+((U16)ch<<8), 0x70,  line&0xff);
		SetSignalTW6874( 0x811+((U16)ch<<8), 0x70,  line>>8);
	}
*/
//	SetSignalTW6874( 0x821+((U16)ch<<8), 0x76,  0x0);

	SetSignalTW6874( 0x820+((U16)ch<<8), 0x70,  0x0);
/*
	if(pos==0)
		SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0x11);
	else if (pos==1)
		SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0x21);
	else
*/
		SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0x31);

	//wait for ANC data
//	Printf("\n\r: polling .");
	//note: this loop is about 1 frame time in EVB in delay; May be different on other platform.
	for(ii=0;ii<5;ii++){	//try polling here first;	
//		if(ii%20 == 0) Printf(".");
		val = ReadTW6874(0x821+((U16)ch<<8));
//		Printf("\n\r:0x821 returned %2x", (WORD)val);
		if((val&0xc0)&&(val&0x08))
			break;
		Ms_Delay(20);
	}

		// disable and read data;
	SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0);

//	if((val&0xc0==0)&&(val&0x08==0)){
	if(((val&0xc0)==0)||((val&0x08)==0)){
//		Printf("\n\r time out, no ANC data received");
		SetSignalTW6874(0x821+((U16)ch<<8), 0x76, 3);
		SetSignalTW6874(0x821+((U16)ch<<8), 0x00, 1);
		return 0;
	}
	else{
		SetSignalTW6874(0x821+((U16)ch<<8), 0x76, 3);
		SetSignalTW6874(0x821+((U16)ch<<8), 0x00, 1);
		return 1;
	}
}

void CheckChLock(U8 ch)
{
	U16 val, lockCnt, kk;
//	U8 agc;

	lockCnt = 0;
	for(kk=0; kk<(LOOP_COUNT<<1); kk++){
		val = ReadTW6874(ch<<1);
		lockCnt += ((val>>6)&1)&((val>>2)&1);
//		Delay(LOCK_DELAY);
	}
//	lockCnt = (U16)lockCnt*100/LOOP_CNT;

//	Printf("\n\r: ************lockCnt = %2x channel %2x",(WORD)lockCnt, (WORD)ch);
//	if((lockCnt>=LOCK_PER_TH)&&(minErr[ch]==0)){
#ifdef CRC_ENABLE
	val=CheckChCrc(ch, 150);
	if((lockCnt>=LOOP_COUNT)&&(minErr[ch]==0)&&((val<8)||(val==0x200))){
#else
	if((lockCnt>=LOOP_COUNT)&&(minErr[ch]==0)){
#endif
		return;
	} else {
		lock_auto(ch);
//		lock_test(ch);
	}

}

void crc_check25(U8 ch, U8 aoc, U8 agc, U8 step)
{
	U16 val;
	U8 ii,jj;

	for(ii=0;ii<5;ii++){
		Printf("\n\r");
//		val=(U16)aoc-8+(ii<<2);
		val=(U16)aoc-(1<<(step+1))+(ii<<step);
		if(val==0x100)	val=0xff;
		if(val<0x80) continue;
		if(val>0xff) continue;
		SetSignalTW6874(0x385+(ch<<4), 0x70, val);
		for(jj=0;jj<5;jj++){
//			val=(U16)agc-8+(jj<<2);
			val=(U16)agc-(1<<(step+1))+(jj<<step);
			if(val==0x100)	val=0xff;
			if(val<0x80) continue;
			if(val>0xff) continue;
			SetSignalTW6874(0x383+(ch<<4), 0x70, val);

			Delay(15);

			if(ch<2)
				val=0x080+ch*6;
			else
				val=0x090+(ch-2)*6;
			SetSignalTW6874(val,0x60,0x30);
			SetSignalTW6874(val,0x60,0x20);
			Delay(2);

			val=ReadTW6874(ch<<1);
			Printf("%4x/", (WORD)val);
			if((val&0x3)==1){
				if((ANCDetection(ch,0xf4)==1)&&((val&0x80)==0x80))
					val=CheckChCrc(ch, 500);
				else
					val=0xff;
			}
			else
				val=CheckChCrc(ch, 500);

			Printf("%4x	", (WORD)val);

			if(val==0){
/*
				Printf("\n\rpassed at aoc=%2x, agc=%2x\n\r", (WORD)ReadTW6874(0x385+(ch<<4)), (WORD)ReadTW6874(0x383+(ch<<4)));

				if(ch<2)
					val=0x80+ch*6;
				else
					val=0x90+(ch-2)*6;
				SetSignalTW6874(val,0x60,30);
				SetSignalTW6874(val,0x60,20);
//				return;
*/
			}				
		}
	}	
//	Printf("\n\rfailed, cannot find a good setting");
	return;

}


#if 1
U16 crc_check(U8 ch, U8 aoc, U8 agc)
{
//	U16 val;

	SetSignalTW6874(0x385+(ch<<4), 0x70, aoc);
	SetSignalTW6874(0x383+(ch<<4), 0x70, agc);

		    Delay(15);

//   val = CheckChCrc(ch, 150);

//   Printf("\n\r crc=%4x, aoc=%2x, agc=%2x", (WORD)val, (WORD)aoc, (WORD)agc);
	
   return CheckChCrc(ch, 150);
}
#endif
///////////////////

#if 1
#define DIRAC_SUPPORT
//#define G_Thre 50
U16 aocHist[6];
U16 agcHist[6];
U8 lockGrid[26];
/// LOOP_COUNT<=100 to make sure ao/gcHist etc @ 9bit resolution for 5 points;

///////////////////////////////////////////////////////
// mode: 0 - HD; 1 - SD; 3 - Dirac
void crc_auto(U8 ch, U8 mode, U8 aocI, U8 agcI, U8 stepI, U16 period)
{
	U16 val, ii, jj, kk, agc, aoc, agct, aoct, bothLock, aocS, agcS;
	U16 maxLock;
	U8 	lenAgc, lenAoc, startAgc, startAoc;
	short step;

//	U16 agcHist[6];
//	U8 lockGrid[26];


#ifdef AUTO_DEBUG
Printf("\n\r: start crc testing ... @ channel %2x", (WORD)ch);
#endif

///////////construct agc/aoc pattern /////////////////
//	agc=0xc0;
//	aoc=0xc0;
	agc=agcI;
	aoc=aocI;
//	step = 5 // start from 0x20;

//  for (step=5; step>0; step--){		// step starts from 0x20;
  for (step=stepI; step>=0; step--){		
  	Printf("\n\r step=%2x, aoc=%2x, agc=%2x", (WORD)step, (WORD)aoc, (WORD)agc);

	maxLock=0;
	for(jj=0;jj<5;jj++)
		agcHist[jj] = 0;

	for(jj=0; jj<5; jj++){	//aoc;
#ifdef AUTO_DEBUG
		Printf("\n\r");
#endif
//		if(jj==4)
//			aoct=0xff;
//		else
//			aoct = 0x80+(jj<<5);

		aoct = aoc-(1<<(step+1))+(jj<<step);
		if(aoct==0x100)
			aoct=0xff;
		else if((aoct<0x80)||(aoct>0x100))
			continue;

		SetSignalTW6874(0x385+(ch<<4), 0x70, aoct);

		aocHist[jj]=0;
		for (ii=0; ii<5; ii++){	//agc;
//			if(ii==4)
//				agct=0xff;
//			else
//				agct = 0x80+(ii<<5);
			agct = agc-(1<<(step+1))+(ii<<step);
			if(agct==0x100)
				agct=0xff;
			else if((agct<0x80)||(agct>0x100))
				continue;

			SetSignalTW6874(0x383+(ch<<4), 0x70, agct);

		    Delay(15);
			if((mode==3)&&(ANCDetection(ch,0xf4)==1))		//dirac case
				bothLock=CheckChCrc(ch, period);
			else if(mode==0)													// hd case; we don't consider pure sd case for now;
				bothLock=CheckChCrc(ch, period);
			else
				bothLock=0xff;

//			bothLock = CheckChCrc(ch, 30);
			bothLock = bothLock>=0xff?0:(LOOP_COUNT-(bothLock*LOOP_COUNT>>8)); 

			lockGrid[jj*5+ii] = bothLock;

#ifdef AUTO_DEBUG
			Printf("%d	", (WORD)bothLock*100/LOOP_COUNT);
//			Printf("%d(%2x/%2x)	", (WORD)bothLock*100/LOOP_COUNT, (WORD)aoct, (WORD)agct);
#endif
			agcHist[ii] += (bothLock);
				aocHist[jj] += (bothLock);
		}
	}

	aocS = aoc;
	agcS = agc;

/////build histogram //////////////////
	aoc = 0;
	agc = 0;
	for(ii=0;ii<5;ii++){
		if(aoc<aocHist[ii])
			aoc = aocHist[ii];
		if(agc<agcHist[ii])
			agc = agcHist[ii];
	}

		startAoc = 0;
		lenAoc = 0;
		kk=0;
		ii=0;
		maxLock=0;
		val=0;
		aoct = aoc*9/10;
//		aoct = aoc;
		for (jj=0;jj<5;jj++){
			if(aocHist[jj]>=aoct){
//				aoc=aocHist[jj];
				if(val<aocHist[jj])
					val=aocHist[jj];
				if(kk==0)
					ii=jj;
				kk++;
				if((jj==4)&&(((kk>lenAoc)&&(val==maxLock))||(val>maxLock))){
					lenAoc=kk;
					startAoc=ii;
				}
			}
			else{
				if(((kk>lenAoc)&&(val==maxLock))||(val>maxLock)){
					lenAoc = kk;
					startAoc = ii;
					maxLock=val;
				}
				kk=0;
				val=0;
			}
		}
		lenAoc--;


		startAgc = 0;
		lenAgc = 0;
		kk=0;
		ii=0;
//		agct = agc;
		agct = agc*9/10;
		for (jj=0;jj<5;jj++){
			if(agcHist[jj]>=agct){
//				agc=agcHist[jj];
				if(kk==0)
					ii=jj;
				kk++;
				if((jj==4)&&(kk>lenAgc)){
					lenAgc=kk;
					startAgc=ii;
				}
			}
			else{
				if(kk>lenAgc){
					lenAgc = kk;
					startAgc = ii;
				}
				kk=0;
			}
		}
		lenAgc--;

		Printf("\n\r startAoc=%2x, lenAoc=%2x", (WORD)startAoc, (WORD)lenAoc);
		Printf("\n\r startAgc=%2x, lenAgc=%2x", (WORD)startAgc, (WORD)lenAgc);

//			aoc = 0x80+(startAoc<<5)+(lenAoc<<4);			
//			agc = 0x80+(startAgc<<5)+(lenAgc<<4);
			if(step>0){				
				aoc = aocS-(1<<(step+1))+(startAoc<<step)+(lenAoc<<(step-1));			
				agc = agcS-(1<<(step+1))+(startAgc<<step)+(lenAgc<<(step-1));				
			}
			else{
				aoc = aocS-(1<<(step+1))+(startAoc<<step);			
				agc = agcS-(1<<(step+1))+(startAgc<<step);				
			}

/////////////////////////////////////////

	aoc=aoc>0xff?0xff:aoc;
	agc=agc>0xff?0xff:agc;
	aoc=aoc<0x80?0x80:aoc;
	agc=agc<0x80?0x80:agc;

		SetSignalTW6874(0x383+(ch<<4), 0x70, agc);
		SetSignalTW6874(0x385+(ch<<4), 0x70, aoc);

	Delay(15);

	val = CheckChCrc(ch, 100);

	if(val == 0)
		return;
  }
//	return 1;
}
///////////////////////////////////////////////////////
void lock_auto(U8 ch)
{
	U16 val, ii, jj, kk, agc, aoc, agct, aoct, bothLock;
	U16 maxLock;
	U8 	lenAgc, lenAoc, startAgc, startAoc, noCrc, errCnt;
	U8 step, aocS, agcS;
	U8 prevAGC, prevAOC;

#ifdef DIRAC_SUPPORT
	U8	lenVc2Aoc, startVc2Aoc;
#endif
//	U16 agcHist[6];
//	U8 lockGrid[26];

// turn off video output and audio first if signal is not good;
	SetSignalTW6874( 0x044 + (ch<<1), 0x77, 0 );		// turn off output during calibration;

	SetSignalTW6874(0x3c, (ch<<4)|ch, 0x0);				// turn off audio ANC detection;

//////////check if video un-plugged///////////////

	noCrc = 1;
	maxLock=0;
	for(jj=2; jj<9; jj+=2){	//aoc;
		SetSignalTW6874(0x385+(ch<<4), 0x70, 0x80+(jj<<4));
		for (ii=2; ii<9; ii+=2){	//agc;
			SetSignalTW6874(0x383+(ch<<4), 0x70, 0x80+(ii<<4));
		    Delay(15);

			bothLock = 0;
			errCnt = 0;
			for(kk=0; kk<LOOP_COUNT; kk++){
				val = ReadTW6874(ch<<1);
				bothLock += ((val>>6)&1)&((val>>2)&1);
				errCnt += (val&0x54)==0x54?1:0;
			}
			if((noCrc==1)&&(bothLock==LOOP_COUNT)&&(errCnt>0)&&(errCnt<LOOP_COUNT)){	// detect crc when lock is stable;
				for(kk=0;kk<LOOP_COUNT;kk++){
					if((val&0x54)==0x44){
						noCrc=0;
						break;
					}
				}
			}

//			if((bothLock==errCnt)&&(bothLock>0)&&(ReadTW6874(0x40+ch) == 0))
//				noCrc++;
//			SetSignalTW6874( 0x039, ((4+ch)<<4)+(4+ch), 0); // Stop CRC counter
//			Printf("\n\r nocrc = %d/bothLock = %d",(WORD)noCrc, (WORD)bothLock);


			if(maxLock<bothLock)
				maxLock=bothLock;
		}
	}
//	if(maxLock<10){
	if(maxLock*100/LOOP_COUNT<30){
		SetSignalTW6874(0x3c, (ch<<4)|ch, 0x0);				// turn off audio ANC detection;
		return;
	}

/////////////////////////////////

/////////////////test3//////////////
	SetSignalTW6874(0x382+(ch<<4), 0x74, 6);
	switch (ch){
	case 0:
		SetSignalTW6874( 0x081, 0x20, 3 );
		break;
	case 1:
		SetSignalTW6874( 0x087, 0x20, 3 );
		break;
	case 2:
		SetSignalTW6874( 0x091, 0x20, 3 );
		break;
	case 3:
		SetSignalTW6874( 0x097, 0x20, 3 );
		break;
	}

///////////////////////////////

	if(Force_Lock==1)
		noCrc=1;

#if 1
//////////////
#ifndef CRC_SCREEN
// if crc info exists, use crc info to do sw calibration directly.  
	if(noCrc==0){
//		crc_auto(ch, 0,/*only HD*/ 0xc0, 0xc0, 5, 10);
#ifdef AUTO_DEBUG
		Printf("\n\r: start crc testing ... @ channel %2x", (WORD)ch);
#endif
		
		///////////construct agc/aoc pattern /////////////////
			agc=0xc0;
			aoc=0xc0;
			step = 5; // start from 0x20;
			prevAGC = 0x80;
			prevAOC = 0x80;

		  for (step=5; step>0; step--){		// step starts from 0x20;
//		  for (step=stepI; step>=0; step--){		
#ifdef AUTO_DEBUG
		  	Printf("\n\r step=%2x, aoc=%2x, agc=%2x", (WORD)step, (WORD)aoc, (WORD)agc);
#endif
		
			maxLock=0;
			for(jj=0;jj<5;jj++)
				agcHist[jj] = 0;
		
			for(jj=0; jj<5; jj++){	//aoc;
#ifdef AUTO_DEBUG
				Printf("\n\r");
#endif
		
				aoct = aoc-(1<<(step+1))+(jj<<step);
				if(aoct==0x100)
					aoct=0xff;
				else if((aoct<0x80)||(aoct>0x100))
					continue;
		
				SetSignalTW6874(0x385+(ch<<4), 0x70, aoct);
		
				aocHist[jj]=0;
				for (ii=0; ii<5; ii++){	//agc;
					agct = agc-(1<<(step+1))+(ii<<step);
					if(agct==0x100)
						agct=0xff;
					else if((agct<0x80)||(agct>0x100))
						continue;
		
					SetSignalTW6874(0x383+(ch<<4), 0x70, agct);
		
				    Delay(15);
					bothLock=CheckChCrc(ch, 10);
		
		//			bothLock = CheckChCrc(ch, 30);
					bothLock = bothLock>=0xff?0:(LOOP_COUNT-(bothLock*LOOP_COUNT>>8)); 
		
					lockGrid[jj*5+ii] = bothLock;
		
#ifdef AUTO_DEBUG
					Printf("%d	", (WORD)bothLock*100/LOOP_COUNT);
		//			Printf("%d(%2x/%2x)	", (WORD)bothLock*100/LOOP_COUNT, (WORD)aoct, (WORD)agct);
#endif
					agcHist[ii] += (bothLock);
						aocHist[jj] += (bothLock);
				}
			}
		
			aocS = aoc;
			agcS = agc;
		
		/////build histogram //////////////////
			aoc = 0;
			agc = 0;
			for(ii=0;ii<5;ii++){
				if(aoc<aocHist[ii])
					aoc = aocHist[ii];
				if(agc<agcHist[ii])
					agc = agcHist[ii];
			}
		
				startAoc = 0;
				lenAoc = 0;
				kk=0;
				ii=0;
				maxLock=0;
				val=0;
				aoct = aoc*9/10;
		//		aoct = aoc;
				for (jj=0;jj<5;jj++){
					if(aocHist[jj]>=aoct){
		//				aoc=aocHist[jj];
						if(val<aocHist[jj])
							val=aocHist[jj];
						if(kk==0)
							ii=jj;
						kk++;
						if((jj==4)&&(((kk>lenAoc)&&(val==maxLock))||(val>maxLock))){
							lenAoc=kk;
							startAoc=ii;
						}
					}
					else{
						if(((kk>lenAoc)&&(val==maxLock))||(val>maxLock)){
							lenAoc = kk;
							startAoc = ii;
							maxLock=val;
						}
						kk=0;
						val=0;
					}
				}
				lenAoc--;
		
		
				startAgc = 0;
				lenAgc = 0;
				kk=0;
				ii=0;
		//		agct = agc;
				agct = agc*9/10;
				for (jj=0;jj<5;jj++){
					if(agcHist[jj]>=agct){
		//				agc=agcHist[jj];
						if(kk==0)
							ii=jj;
						kk++;
						if((jj==4)&&(kk>lenAgc)){
							lenAgc=kk;
							startAgc=ii;
						}
					}
					else{
						if(kk>lenAgc){
							lenAgc = kk;
							startAgc = ii;
						}
						kk=0;
					}
				}
				lenAgc--;
#ifdef AUTO_DEBUG		
				Printf("\n\r startAoc=%2x, lenAoc=%2x", (WORD)startAoc, (WORD)lenAoc);
				Printf("\n\r startAgc=%2x, lenAgc=%2x", (WORD)startAgc, (WORD)lenAgc);
#endif
		
		//			aoc = 0x80+(startAoc<<5)+(lenAoc<<4);			
		//			agc = 0x80+(startAgc<<5)+(lenAgc<<4);
					if(step>0){				
						aoc = aocS-(1<<(step+1))+(startAoc<<step)+(lenAoc<<(step-1));			
						agc = agcS-(1<<(step+1))+(startAgc<<step)+(lenAgc<<(step-1));				
					}
					else{
						aoc = aocS-(1<<(step+1))+(startAoc<<step);			
						agc = agcS-(1<<(step+1))+(startAgc<<step);				
					}
		
		/////////////////////////////////////////
		
			aoc=aoc>0xff?0xff:aoc;
			agc=agc>0xff?0xff:agc;
			aoc=aoc<0x80?0x80:aoc;
			agc=agc<0x80?0x80:agc;
		
				SetSignalTW6874(0x383+(ch<<4), 0x70, agc);
				SetSignalTW6874(0x385+(ch<<4), 0x70, aoc);

		   if((prevAGC==agc)&&(prevAOC==aoc)&&(step<3))
		   		break;

		   prevAGC = agc;
		   prevAOC = aoc;
		
			Delay(15);
		
			val = CheckChCrc(ch, 100);
		
			if(val == 0)
				break;
		  }			// end of		  for (step=5; step>0; step--)

		/////////////////////////////////////////////////////
#ifdef AUTO_DEBUG
		  	Printf("\n\r step=%2x, aoc=%2x, agc=%2x", (WORD)step, (WORD)aoc, (WORD)agc);
#endif
		if(val > 0){	//crc error>0 for current settings;
			SetSignalTW6874(0x382+(ch<<4), 0x74, 9);
			switch (ch){
			case 0:
				SetSignalTW6874( 0x081, 0x20, 7 );
				break;
			case 1:
				SetSignalTW6874( 0x087, 0x20, 7 );
				break;
			case 2:
				SetSignalTW6874( 0x091, 0x20, 7 );
				break;
			case 3:
				SetSignalTW6874( 0x097, 0x20, 7 );
				break;
			}

			Delay(5);
			if(val<=CheckChCrc(ch,100)){
				SetSignalTW6874(0x382+(ch<<4), 0x74, 6);
				switch (ch){
				case 0:
					SetSignalTW6874( 0x081, 0x20, 3 );
					break;
				case 1:
					SetSignalTW6874( 0x087, 0x20, 3 );
					break;
				case 2:
					SetSignalTW6874( 0x091, 0x20, 3 );
					break;
				case 3:
					SetSignalTW6874( 0x097, 0x20, 3 );
					break;
				}
			}
		}

//		SetSignalTW6874( 0x044 + (ch<<1), 0x77, 1 );	Printf("\n\r display turned on");	// turn on output after calibration;

// set audio and display
	if(val<0x20){	// set audio if CRC error not too bad;
		ForceDisplay = 1;		//force display configuration; only used for Intersil evb;
		if((ReadTW6874(ch<<1)&3)!=1){	//HD case
				DiracInput[ch] = 0;
		/*
			switch(ch){
			case 0:
				set_dirac_func( 1, 8, ReadTW6874(0x83) );
				break;
			case 1:
				set_dirac_func( 2, 8, ReadTW6874(0x89) );
				break;
			case 2:
				set_dirac_func( 3, 8, ReadTW6874(0x93) );
				break;
			case 3:
				set_dirac_func( 4, 8, ReadTW6874(0x99) );
				break;
			}	
		 */
			SetAudioCh(ch+1+((right_audio_chan>>ch)&1)*4, 1, 0);			// loop left or right channel from the port to DAC output depending on RIGHT_ACHAN setting;
//			Printf("\n\r set audio %2x, %2x, %2x", (WORD)(ch+1), (WORD)1, (WORD)0);
		}
		else{
		
			if(DiracAutoFlag)
				DiracInput[ch] = 1;
			/*
			else{
				switch(ch){
				case 0:
					set_dirac_func( 1, 8, ReadTW6874(0x83) );
					break;
				case 1:
					set_dirac_func( 2, 8, ReadTW6874(0x89) );
					break;
				case 2:
					set_dirac_func( 3, 8, ReadTW6874(0x93) );
					break;
				case 3:
					set_dirac_func( 4, 8, ReadTW6874(0x99) );
					break;
				}		
			}
			*/
			SetAudioCh(ch+1+((right_audio_chan>>ch)&1)*4, 0, 0);			// loop left or right channel from the port to DAC output depending on RIGHT_ACHAN setting;
//			Printf("\n\r set audio %2x, %2x, %2x", (WORD)(ch+1), (WORD)0, (WORD)0);
		}
	}
	else
		Printf("\n\r: audio not turned on because of poor connection");
//////////////////


		minErr[ch] = 0;
		return;
	}		//end of	if(noCrc==0){
#endif
//////////////
#endif

#ifdef AUTO_DEBUG
Printf("\n\r: start lock testing ... calibrated agc %2x and aoc %2x @ channel %2x", (WORD)(ReadTW6874(0x389+(ch<<4))|0x80), (WORD)aoc, (WORD)ch);
#endif

///////////construct agc/aoc pattern /////////////////
	maxLock=0;
	agc=0;
	aoc=0;
	for(jj=0;jj<5;jj++)
		agcHist[jj] = 0;

	for(jj=0; jj<5; jj++){	//aoc;
#ifdef AUTO_DEBUG
		Printf("\n\r");
#endif
		if(jj==4)
			aoct=0xff;
		else
			aoct = 0x80+(jj<<5);
		SetSignalTW6874(0x385+(ch<<4), 0x70, aoct);

		aocHist[jj]=0;
		for (ii=0; ii<5; ii++){	//agc;
			if(ii==4)
				agct=0xff;
			else
				agct = 0x80+(ii<<5);
			SetSignalTW6874(0x383+(ch<<4), 0x70, agct);

		    Delay(15);

			bothLock = 0;
			for(kk=0; kk<LOOP_COUNT; kk++){
				val = ReadTW6874(ch<<1);
				bothLock += ((val>>6)&1)&((val>>2)&1);
			}

			lockGrid[jj*5+ii] = bothLock;

#ifdef AUTO_DEBUG
			Printf("%d	", (WORD)bothLock*100/LOOP_COUNT);
#endif
			agcHist[ii] += (bothLock);
				aocHist[jj] += (bothLock);
		}
	}

/////build histogram //////////////////
	aoc = 0;
	agc = 0;
	for(ii=0;ii<5;ii++){
		if(aoc<aocHist[ii])
			aoc = aocHist[ii];
		if(agc<agcHist[ii])
			agc = agcHist[ii];
	}

		startAoc = 0;
		lenAoc = 0;
		kk=0;
		ii=0;
		maxLock=0;
		val=0;
		aoct = aoc*9/10;
//		aoct = aoc;
		for (jj=0;jj<5;jj++){
			if(aocHist[jj]>=aoct){
//				aoc=aocHist[jj];
				if(val<aocHist[jj])
					val=aocHist[jj];
				if(kk==0)
					ii=jj;
				kk++;
				if((jj==4)&&(((kk>lenAoc)&&(val==maxLock))||(val>maxLock))){
					lenAoc=kk;
					startAoc=ii;
				}
			}
			else{
				if(((kk>lenAoc)&&(val==maxLock))||(val>maxLock)){
					lenAoc = kk;
					startAoc = ii;
					maxLock=val;
				}
				kk=0;
				val=0;
			}
		}
		lenAoc--;

//////////////////////////////
#ifdef DIRAC_SUPPORT
		startVc2Aoc = 0;
		lenVc2Aoc = 0;
		kk=0;
		ii=0;
		aoct = aoc;
		for (jj=0;jj<5;jj++){
			if(aocHist[jj]>=aoct){
				if(kk==0)
					ii=jj;
				kk++;
				if((jj==4)&&(kk>lenVc2Aoc)){
					lenVc2Aoc=kk;
					startVc2Aoc=ii;
				}
			}
			else{
				if(kk>lenVc2Aoc){
					lenVc2Aoc = kk;
					startVc2Aoc = ii;
				}
				kk=0;
			}
		}
		lenVc2Aoc--;
#endif
//////////////////////////////


		startAgc = 0;
		lenAgc = 0;
		kk=0;
		ii=0;
		agct = agc;
		for (jj=0;jj<5;jj++){
			if(agcHist[jj]>=agct){
//				agc=agcHist[jj];
				if(kk==0)
					ii=jj;
				kk++;
				if((jj==4)&&(kk>lenAgc)){
					lenAgc=kk;
					startAgc=ii;
				}
			}
			else{
				if(kk>lenAgc){
					lenAgc = kk;
					startAgc = ii;
				}
				kk=0;
			}
		}
		lenAgc--;

		agct = (ReadTW6874(0x389+(ch<<4)))|0x80;
///		Printf("\n\r: startAoc=%2x, startAgc=%2x, lenAoc=%2x, lenAgc=%2x", (WORD)startAoc, (WORD)startAgc, (WORD)lenAoc, (WORD)lenAgc);

/////////histogram analysis //////////////////////
		maxLock = 5*LOOP_COUNT;
		if((agc==maxLock)&&(aoc==maxLock)){	

///			Printf("\n\r****************************0::x***********************");
			kk=ReadTW6874(0x389+(ch<<4))|0x80;

			aoc = 0x80+(startAoc<<5)+(lenAoc<<4);			
			aoc = aoc>0xff?0xff:aoc;
			agc = 0x80+(startAgc<<5)+(lenAgc<<4);				
			agc = agc>0xff?0xff:agc;
			agc = agc<kk?agc:kk;
			///// adjust agc //////////////
			ii=(agcHist[0]>>4);
			jj=(agcHist[4]>>4);
			agc = (agc-8);
			if(ii>jj){
				if (agc >0x80){
					agc = agc-((ii-jj)<<3)/((5*LOOP_COUNT)>>4);
				} else
					agc = 0x80;
			} else{
				agc = agc+((jj-ii)<<3)/((5*LOOP_COUNT)>>4);
			}
			///////////////////////////////
/*
			if(startAgc==0){	
			Printf("\n\r****************************0::0***********************");
			}
			else if (startAgc+lenAgc==4){
			Printf("\n\r****************************0::1***********************");
			}
			if(startAoc==0){
			Printf("\n\r****************************0::2***********************");
			}
			else if(startAoc+lenAoc==4){
			Printf("\n\r****************************0::3***********************");
			}
			else {
			Printf("\n\r****************************0::4***********************");
			}
*/
		}else if (agc==maxLock){
			aoc = 0x80+(startAoc<<5)+(lenAoc<<4);
			aoc = aoc>0xff?0xff:aoc;
			kk=ReadTW6874(0x389+(ch<<4))|0x80;

			if(startAgc==0){
///			Printf("\n\r****************************1::0***********************");
//				aoc = 0x80+(startAoc<<5)+(lenAoc<<4);
				agc = 0x80+(lenAgc<<3);
				kk = kk-0x28>0x80?kk-0x28:0x80;
				agc = agc<kk?agc:kk;
			}else if ((startAgc+lenAgc)==4){
///			Printf("\n\r****************************1::1***********************");
//				aoc = 0x80+(startAoc<<5)+(lenAoc<<4);
				agc = 0xff-(lenAgc<<3);
				kk = (kk+0x20)>0xff?0xff:kk+0x20;
				agc = agc>kk?agc:kk;
			}else{
///			Printf("\n\r****************************1::2***********************");
//				aoc = 0x80+(startAoc<<5)+(lenAoc<<4);
				if((kk>=0x80+(startAgc<<5))&&(kk<=0x80+((startAgc+lenAgc)<<5)))
					agc = kk;
				else
					agc = 0x80+(startAgc<<5)+(lenAgc<<4);
				agc = agc>0xff?0xff:agc;


			    if((agc <= 0xa0)&&(agcHist[0]>agcHist[2]+32)){	// possible short cable case;
					agc -= 8;
				} else if ((agc >= 0xe0)&&(agcHist[4]>agcHist[2]+32)){	// possible long cable case;
					agc += 8;
				} else if((agc < 0xe0)&&(agc > 0xa0)){
				    ii=(agcHist[0]+agcHist[1])>>4;
					jj=(agcHist[3]+agcHist[4])>>4;
					if (ii>jj){		//maybe short cable;
						agc -= (agc-0xa0)*(ii-jj)/(ii+jj);
					} else if (jj>ii){		//maybe long cable;
						agc += (0xe0-agc)*(jj-ii)/(ii+jj);
					}
				}

			}

		}else if (aoc==maxLock){			
			kk=ReadTW6874(0x389+(ch<<4))|0x80;

			aoc = 0x80+(startAoc<<5)+(lenAoc<<4);			
			aoc = aoc>0xff?0xff:aoc;
			agc = 0x80+(startAgc<<5)+(lenAgc<<4);				
			agc = agc>0xff?0xff:agc;
			agc = agc<kk?agc:kk;
			///// adjust agc //////////////
			ii=(agcHist[0]>>4);
			jj=(agcHist[4]>>4);
			agc = (agc-8);
			if(ii>jj){
				if (agc >0x80){
					agc = agc-((ii-jj)<<3)/((5*LOOP_COUNT)>>4);
				} else
					agc = 0x80;
			} else{
				agc = agc+((jj-ii)<<3)/((5*LOOP_COUNT)>>4);
			}
			///////////////////////////////
/*
			if(startAoc==0){
			Printf("\n\r****************************2::0***********************");
			}else if (startAoc+lenAoc==4){
			Printf("\n\r****************************2::1***********************");
			}else 
*/
			if (startAgc+lenAgc==4){
///			Printf("\n\r****************************2::2***********************");
				if((lenAgc>1)&&(startAgc>0))
					agc = agc<0x80+(startAgc<<5)+(lenAgc<<4)?(agc>>1)+0x40+(startAgc<<4)+(lenAgc<<3):agc;
			}
			else{
///			Printf("\n\r****************************2::3***********************");
				if(((lenAoc==0)&&(startAoc==2))){
					aoc = 0xc0;
				} else if ((lenAoc==1)&&(startAoc==1)){
					if (aocHist[2]>aocHist[1])
						aoc = 0xc0;
					else
						aoc = 0xb8;
				} else if ((lenAoc==1)&&(startAoc==2)){
					if (aocHist[2] > aocHist[3])
						aoc = 0xc0;
					else
						aoc = 0xc8;
				}
			}
		}else {
			aoc = 0x80+(startAoc<<5)+(lenAoc<<4);
			aoc = aoc>0xff?0xff:aoc;
			kk=ReadTW6874(0x389+(ch<<4))|0x80;
			if(startAgc==0){
///			Printf("\n\r****************************3::0***********************");
//				aoc = 0x80+(startAoc<<5)+(lenAoc<<4);

				agc = 0x80;

				if ((startAgc==0)&&(lenAgc==0)){
					
					startAoc = 0;
					lenAoc = 0;
					kk=0;
					ii=0;
					val=0;
					aoct = LOOP_COUNT;
			//		aoct = aoc;
					for (jj=0;jj<5;jj++){
						if(lockGrid[jj*5]>=aoct){
							if(kk==0)
								ii=jj;
							kk++;
							if((jj==4)&&(kk>lenAoc)){
								lenAoc=kk;
								startAoc=ii;
							}
						}
						else{
							if(kk>lenAoc){
								lenAoc = kk;
								startAoc = ii;
							}
							kk=0;
							val=0;
						}
					}
					lenAoc--;
			
				}
				aoc = 0x80+(startAoc<<5)+(lenAoc<<4);
				aoc = aoc>0xff?0xff:aoc;

				if(aoc>0xc0)
					aoct = ((aoc&0x7f)+0x1f)>>5;
				else if (aoc<0xc0)
					aoct = (aoc&0x7f)<0x20?0:((aoc&0x7f))>>5;
				else
					aoct = 2;


				if((aoct>0)&&(aoct<4)){
					if(lockGrid[(aoct-1)*5]+lockGrid[(aoct-1)*5+1]>lockGrid[(aoct+1)*5]+lockGrid[(aoct+1)*5+1])
						aoc -= 8;
					else if(lockGrid[(aoct-1)*5]+lockGrid[(aoct-1)*5+1]>lockGrid[(aoct+1)*5]+lockGrid[(aoct+1)*5+1])
						aoc += 8;
					else {
						if((aocHist[2]>aocHist[1])&&(aocHist[2]>aocHist[0])&&(aocHist[2]>aocHist[3])&&(aocHist[2]>aocHist[4]))
							aoc = aoc>0xc0?aoc-8:(aoc==0xc0?0xc0:aoc+8);
					}
				}
			}else if(startAgc+lenAgc==4){
///			Printf("\n\r****************************3::1***********************");
				agc = 0xff;
				aoc = aoc>0xff?0xff:aoc;
				if(aoc>0xc0)
					aoct = ((aoc&0x7f)+0x1f)>>5;
				else if (aoc<0xc0)
					aoct = (aoc&0x7f)<0x20?0:((aoc&0x7f))>>5;
				else
					aoct = 2;
			   if((startAoc>0)&&(aocHist[startAoc-1]<(aocHist[startAoc]>>5))&&(startAoc+lenAoc<4)&&(aocHist[startAoc+lenAoc+1]<(aocHist[startAoc+lenAoc]>>5))){
			   }
//				if((aoct>0)&&(aoct<4)){
				else if((aoct>0)&&(aoct<4)){


					if(lockGrid[(aoct-1)*5+3]+lockGrid[(aoct-1)*5+4]>lockGrid[(aoct+1)*5+3]+lockGrid[(aoct+1)*5+4])
						aoc -= 8;
					else if(lockGrid[(aoct-1)*5+3]+lockGrid[(aoct-1)*5+4]<lockGrid[(aoct+1)*5+3]+lockGrid[(aoct+1)*5+4])
						aoc += 8;
					else {
	//					if((aocHist[2]>>5)>((aocHist[0]+aocHist[1])>>6))	//max at 0xc0
						if((aocHist[2]>aocHist[1])&&(aocHist[2]>aocHist[0])&&(aocHist[2]>aocHist[3])&&(aocHist[2]>aocHist[4]))
							aoc = aoc>0xc0?aoc-8:(aoc==0xc0?0xc0:aoc+8);
					}
				}
			}else if(startAoc==0){
///			Printf("\n\r****************************3::2***********************");
				agc = 0x80+(startAgc<<5)+(lenAgc<<4);
				agc = agc>0xff?0xff:agc;
			}else if(startAoc+lenAoc==4){
///			Printf("\n\r****************************3::3***********************");
				agc = 0x80+(startAgc<<5)+(lenAgc<<4);
				agc = agc>0xff?0xff:agc;
			}else{
///			Printf("\n\r****************************3::4***********************");
//				aoc = 0x80+(startAoc<<5)+(lenAoc<<4);
			//////////////////////////test3///////////////////////////
			   if((agcHist[4]>agcHist[startAgc]*9/10)&&(startAoc>0)&&(aocHist[startAoc-1]<(aocHist[startAoc]>>5))&&(startAoc+lenAoc<4)&&(aocHist[startAoc+lenAoc+1]<(aocHist[startAoc+lenAoc]>>5))){
			   }else{
		    //////////////////////////////////////////////////////////
				if(((lenAoc==0)&&(startAoc==2))){
					aoc = 0xc0;
				} else if ((lenAoc==1)&&(startAoc==1)){
					if (aocHist[2]>aocHist[1])
						aoc = 0xc0;
					else
						aoc = 0xb8;
				} else if ((lenAoc==1)&&(startAoc==2)){
					if (aocHist[2] > aocHist[3])
						aoc = 0xc0;
					else
						aoc = 0xc8;
					}
				}

				if((lenAgc>1)&&(kk>=0x80+(startAgc<<5))&&(kk<=0x80+((startAgc+lenAgc)<<5)))
					agc = kk;
				else
					agc = 0x80+(startAgc<<5)+(lenAgc<<4);
				agc = agc>0xff?0xff:agc;

			    if((agc <= 0xa0)&&(agcHist[0]>agcHist[2]+32)){	// possible short cable case;
					agc -= 8;
				} else if ((agc >= 0xe0)&&(agcHist[4]>agcHist[2]+32)){	// possible long cable case;
					agc += 8;
				} else if((agc < 0xe0)&&(agc > 0xa0)){
				    ii=(agcHist[0]+agcHist[1])>>4;
					jj=(agcHist[3]+agcHist[4])>>4;
					if (ii>jj){		//maybe short cable;
						agc -= (agc-0xa0)*(ii-jj)/(ii+jj);
					} else if (jj>ii){		//maybe long cable;
						agc += (0xe0-agc)*(jj-ii)/(ii+jj);
					}
				}
			}
		}


#ifdef AUTO_DEBUG
	Printf("\n\r agc=%4x, aoc=%4x", (WORD)agc, (WORD)aoc);
#endif
			///// adjust aoc //////////////
			aoc = aoc>0xff?0xff:aoc;
			if (aoc>0xc0){
				aoct = ((aoc&0x7f)+0x1f)>>5;
				ii = (lockGrid[(4-aoct)*5+((agc<0x81?0:agc-0x81)>>5)] + lockGrid[(4-aoct)*5+(((agc&0x7f)+0x20)>>5)])>>3;
				jj = (lockGrid[(aoct)*5+((agc<0x81?0:agc-0x81)>>5)] + lockGrid[(aoct)*5+(((agc&0x7f)+0x20)>>5)])>>3;
				kk = (lockGrid[10+((agc<0x81?0:agc-0x81)>>5)] + lockGrid[10+(((agc&0x7f)+0x20)>>5)])>>3;
///				Printf("\n\r: ii=%d, jj=%d, kk=%d", (WORD)ii, (WORD)jj, (WORD)kk);
				if(ii>=jj)
					aoc = aoc>0xd0?0xd0:aoc;
				else if(kk>=jj){					
					aoc = aoc>0xd8?0xd8:aoc;
				} else if(jj <= ii+kk){
					if((lockGrid[(aoct-1)*5+((agc<0x81?0:agc-0x81)>>5)]+lockGrid[(aoct-1)*5+(((agc&0x7f)+0x20)>>5)]>lockGrid[(aoct)*5+((agc<0x81?0:agc-0x81)>>5)]+lockGrid[(aoct)*5+(((agc&0x7f)+0x20)>>5)]))
						aoc = (aoc-8)>0xd8?aoc-8:0xd8;
				} else if(jj>(ii<<1)+kk){
					if(((lockGrid[(aoct-1)*5+((agc<0x81?0:agc-0x81)>>5)]+lockGrid[(aoct-1)*5+(((agc&0x7f)+0x20)>>5)]<lockGrid[(aoct)*5+((agc<0x81?0:agc-0x81)>>5)]+lockGrid[(aoct)*5+(((agc&0x7f)+0x20)>>5)])))
						aoc = aoc+8>(aoct<<5)+0x80?(aoct<<5)+0x80-1:aoc+8;
///					Printf("\n\r aoc=%2x, aoct = %2x", (WORD)aoc, (WORD)aoct);
				}
				if(lenAoc>0)
					aoc = aoc<(0x80+(startAoc<<5))?0x80+(startAoc<<5):aoc;
			} else if (aoc<0xc0){
//				aoct = ((aoc&0x7f)+0x1f)>>5;
				aoct = (aoc&0x7f)<0x20?0:((aoc&0x7f))>>5;
				ii = (lockGrid[(aoct)*5+((agc<0x81?0:agc-0x81)>>5)] + lockGrid[(aoct)*5+(((agc&0x7f)+0x20)>>5)])>>3;
				jj = (lockGrid[(4-aoct)*5+((agc<0x81?0:agc-0x81)>>5)] + lockGrid[(4-aoct)*5+(((agc&0x7f)+0x20)>>5)])>>3;
				kk = (lockGrid[10+((agc-0x81)>>5)] + lockGrid[10+(((agc&0x7f)+0x20)>>5)])>>3;
///				Printf("\n\r: ii=%d, jj=%d, kk=%d", (WORD)ii, (WORD)jj, (WORD)kk);
				if(jj>=ii)
					aoc = aoc<0xb0?0xb0:aoc;
				else if(kk>=ii){					
					aoc = aoc<0xa8?0xa8:aoc;
				} else if(ii <= kk+jj){
					if((lockGrid[(aoct+1)*5+((agc<0x81?0:agc-0x81)>>5)]+lockGrid[(aoct+1)*5+(((agc&0x7f)+0x20)>>5)]>lockGrid[(aoct)*5+((agc<0x81?0:agc-0x81)>>5)]+lockGrid[(aoct)*5+(((agc&0x7f)+0x20)>>5)]))
						aoc = (aoc+8)<0xa8?aoc+8:0xa8;
				} else if(ii>(jj<<1)+kk){
					if(((lockGrid[(aoct)*5+((agc<0x81?0:agc-0x81)>>5)]+lockGrid[(aoct)*5+(((agc&0x7f)+0x20)>>5)]>lockGrid[(aoct+1)*5+((agc<0x81?0:agc-0x81)>>5)]+lockGrid[(aoct+1)*5+(((agc&0x7f)+0x20)>>5)])))
						aoc  = aoc<(aoct<<5)+0x80+8?(aoct<<5)+0x80:aoc-8;
				}
				if(lenAoc>0)
					aoc = aoc>(0x80+((startAoc+lenAoc)<<5))?0x80+((startAoc+lenAoc)<<5):aoc;
			} else {
				if(lenAoc==0){
					ii = (lockGrid[5+((agc<0x81?0:agc-0x81)>>5)] + lockGrid[5+(((agc&0x7f)+0x20)>>5)])>>3;
					jj = (lockGrid[15+((agc<0x81?0:agc-0x81)>>5)] + lockGrid[15+(((agc&0x7f)+0x20)>>5)])>>3;
				} else {
					ii = (lockGrid[((agc<0x81?0:agc-0x81)>>5)] + lockGrid[(((agc&0x7f)+0x20)>>5)])>>3;
					jj = (lockGrid[20+((agc<0x81?0:agc-0x81)>>5)] + lockGrid[20+(((agc&0x7f)+0x20)>>5)])>>3;
				}
///				Printf("\n\r: ii=%d, jj=%d", (WORD)ii, (WORD)jj);
				if (ii>jj)
					aoc -= 8;
				else if (ii<jj)
					aoc += 8;
				if(lenAoc>0){
					aoc = aoc<(0x80+(startAoc<<5))?0x80+(startAoc<<5):aoc;
					aoc = aoc>(0x80+((startAoc+lenAoc)<<5))?0x80+((startAoc+lenAoc)<<5):aoc;
				}
			}
			///////////////////////////////

/////////////////////////////////////////

	aoc=aoc>0xff?0xff:aoc;
	agc=agc>0xff?0xff:agc;
	aoc=aoc<0x80?0x80:aoc;
	agc=agc<0x80?0x80:agc;

//////////////////////////test3/////////////////////////////////////
	if((agc==0xff)||((agc>=0xe0)&&(agcHist[4]>agcHist[startAgc]*9/10))){
		SetSignalTW6874(0x382+(ch<<4), 0x74, 0xf);
		switch (ch){
		case 0:
			SetSignalTW6874( 0x081, 0x20, 7 );
			break;
		case 1:
			SetSignalTW6874( 0x087, 0x20, 7 );
			break;
		case 2:
			SetSignalTW6874( 0x091, 0x20, 7 );
			break;
		case 3:
			SetSignalTW6874( 0x097, 0x20, 7 );
			break;
		}
		Delay(5);
	}
///////////////////////////////////////////////////////////////
#ifdef AUTO_DEBUG
	Printf("\n\r agc=%4x, aoc=%4x", (WORD)agc, (WORD)aoc);
#endif

	SetSignalTW6874(0x383+(ch<<4), 0x70, agc);
	SetSignalTW6874(0x385+(ch<<4), 0x70, aoc);
	Ms_Delay(5);
	if(DiracAutoFlag==1){
		Ms_Delay(10);
	//	val = ReadTW6874(0x054);
	//	if(((val >> ch) & 0x01) == 0x01)	// if it's Dirac
		jj=0;
		for(ii=0;ii<500;ii++){
			val = ReadTW6874((ch<<1));
			if((val&0x44)==0x44)
				jj++;
			else
				jj=0;
			if(jj>30)
//			if((val&0x3)==1)
				break;
		}
#ifdef AUTO_DEBUG
		Printf("\n\r status = %2x, DiracAutoFlag = %2x", (WORD)val, (WORD)DiracAutoFlag);
#endif
		if(((val & 0x3) == 0x01)){	// if it's SD, treat it as Dirac when it's dirac auto mode;
//			Printf("\n\r (agc&0x7f)*3 = %4x", (WORD)((agc&0x7f)*3));

			{
				if((aocHist[startVc2Aoc]==5*LOOP_COUNT)&&(startVc2Aoc>0)&&(startVc2Aoc+lenVc2Aoc<4)&&(aocHist[startVc2Aoc]/3 > aocHist[startVc2Aoc-1])&&(aocHist[startVc2Aoc+lenVc2Aoc]/3 > aocHist[startVc2Aoc+lenVc2Aoc+1]))
					aoc = 0x80 + (startVc2Aoc<<5) + (lenVc2Aoc<<4);
//				} else {
				else if((startAoc>0)&&(startAoc+lenAoc<4)&&(aocHist[startAoc]/3 > aocHist[startAoc-1])&&(aocHist[startAoc+lenAoc]/3 > aocHist[startAoc+lenAoc+1])){
					aoc = 0x80 + (startAoc<<5) + (lenAoc<<4);
				}
//				aoc = aoc>0xff?0xff:aoc;
			}

//			if((startAgc>0)&&(lenAoc<3))
			{
//				if(aoc>0xc0)
//					startAoc = ((aoc+1)&0x7f)>>5;	//cover 0xff case;
//				else
//					startAoc = ((aoc+0x1f)&0x7f)>>5;

				startAoc = ((aoc+0x10)-0x80)>>5;
	
				kk = ReadTW6874(0x389+(ch<<4))|0x80;
				if(aocHist[startAoc]==5*LOOP_COUNT){
					agct = agc;
					agc = agc<kk?agc:kk;
					if((lenAoc==0)&&(startAgc==4)&&((aocHist[startAoc]<<1)>aocHist[0]+aocHist[1]+aocHist[2]+aocHist[3]+aocHist[4])){
						agc = (agc>>1)+0x60;
//						Printf("\n\r 111111111 agc=%4x", (WORD)agc);
					}
					else{
					// at most 0xb0;
						if(agc>0xb8){
							ii=aocHist[0]+aocHist[1]+aocHist[2]+aocHist[3]+aocHist[4];
							agc = 0xb0+(agc-0xb0)*(LOOP_COUNT-(ii/25))/LOOP_COUNT;
//							Printf("\n\r 222222222 agc=%4x", (WORD)agc);
						}
						else{
							agc = agc<0xb0?agc:0xb0;
//							Printf("\n\r 333333333 agc=%4x", (WORD)agc);
						}
					}
				}  else if(lockGrid[startAoc*5+2]==LOOP_COUNT){
					if((lenAoc==0)&&(startAgc==4)&&((aocHist[startAoc]<<1)>aocHist[0]+aocHist[1]+aocHist[2]+aocHist[3]+aocHist[4])){
						agc = (agc>>1)+0x60;
					}
					else{
					// at most 0xc0;
						if(agc>0xb8){
							ii=aocHist[0]+aocHist[1]+aocHist[2]+aocHist[3]+aocHist[4];
							agc = 0xb0+(agc-0xb0)*(LOOP_COUNT-(ii/25))/LOOP_COUNT;
						}
						else{
							ii=(lockGrid[startAoc*5]+lockGrid[startAoc*5+1])>>1;
							jj=0xc0-(ii<<4)/LOOP_COUNT;
							agc = agc<jj?agc:jj;
						}
					}
				} else{
					// at most 0xe0;
					ii=(lockGrid[startAoc*5+3]+lockGrid[startAoc*5+4])>>1;
					jj=(lockGrid[startAoc*5]+lockGrid[startAoc*5+1]+lockGrid[startAoc*5+2])/3;
					if(ii+jj>0)
						kk=0xe0-(jj<<5)/(ii+jj);
					else kk = 0xd0;
					agc=agc<kk?agc:kk;
				}
			}
/*
//	    	agc = (((agc&0x7f)<<2)/5)|0x80;
			agc = agc>0xc0?0xc0:agc;
			agc = agc>vc2Agc?agc:vc2Agc;	//if vc2Agc is set to 0xd0 instead of 0x80, use vc2Agc;
*/

			SetSignalTW6874(0x385+(ch<<4), 0x70, aoc);
			SetSignalTW6874(0x383+(ch<<4), 0x70, agc);
#ifdef AUTO_DEBUG
			Printf("\n\r (Dirac) agc=%4x, aoc=%4x", (WORD)agc, (WORD)aoc);
#endif

				DiracInput[ch] = 1;
/*
			switch(ch){
			case 0:
				set_dirac_func( 1, ReadTW6874(0x55)&0x7, ReadTW6874(0x83) );
				break;
			case 1:
				set_dirac_func( 2, (ReadTW6874(0x55)>>4)&0x7, ReadTW6874(0x89) );
				break;
			case 2:
				set_dirac_func( 3, ReadTW6874(0x56)&0x7, ReadTW6874(0x93) );
				break;
			case 3:
				set_dirac_func( 4, (ReadTW6874(0x56)>>4)&0x7, ReadTW6874(0x99) );
				break;
			}
*/
		}
		else if((ReadTW6874(0x54)&(1<<ch))!=0){		//Dirac enable bit not set;
	//	Printf("\n\r: SD un plugged %2x %2x %2x %2x", (WORD)channel, (WORD)std, (WORD)(DiracAutoFlag), (WORD)((ReadTW6874(0x54)&(1<<channel))));
				DiracInput[ch] = 0;
/*
			switch(ch){
			case 0:
				set_dirac_func( 1, 8, ReadTW6874(0x83) );
				break;
			case 1:
				set_dirac_func( 2, 8, ReadTW6874(0x89) );
				break;
			case 2:
				set_dirac_func( 3, 8, ReadTW6874(0x93) );
				break;
			case 3:
				set_dirac_func( 4, 8, ReadTW6874(0x99) );
				break;
			}		
*/
		}
	
	}
	else if((ReadTW6874(0x54)&(1<<ch))!=0){
//	Printf("\n\r: SD un plugged %2x %2x %2x %2x", (WORD)channel, (WORD)std, (WORD)(DiracAutoFlag), (WORD)((ReadTW6874(0x54)&(1<<channel))));
				DiracInput[ch] = 0;
/*
		switch(ch){
		case 0:
			set_dirac_func( 1, 8, ReadTW6874(0x83) );
			break;
		case 1:
			set_dirac_func( 2, 8, ReadTW6874(0x89) );
			break;
		case 2:
			set_dirac_func( 3, 8, ReadTW6874(0x93) );
			break;
		case 3:
			set_dirac_func( 4, 8, ReadTW6874(0x99) );
			break;
		}
*/		
	}


#if	1
#ifndef CRC_ENABLE
	if(1){
#else
	if((((ReadTW6874(ch<<1)&3)==1)&&(DiracAutoFlag==1))){		// Dirac case;
#endif
		val = lock_check(ch, aoc, agc);
#ifdef AUTO_DEBUG
		Printf("\n\r ++++++lock=%2x", (WORD)val);
#endif
		if ((val==100)){
			kk=0;
		}
		else {
			agct = aoc;
			kk=1;
			for (ii=0;ii<5;ii++){
				if (ii==2) continue;
				aoct = aoc+0x10-(ii<<3);
				aoct = aoct>0xff?0xff:aoct;
				aoct = aoct<0x80?0x80:aoct;
				jj=lock_check(ch, aoct, agc);
				if (jj==100){
					kk=0;
					break;
				}else if(jj<val){
					val = jj;
					agct = aoct; // just use agct as a temporary holder				
				}
			}
		}

		val = 100-val;
	}
	else {

		val = crc_check(ch, aoc, agc);
///		Printf("\n\r ++++++crc=%2x", (WORD)val);
		if ((val==0)){
			kk=0;
		}else{
			if(val==0x200){	// no crc info available;
				val = lock_check(ch, aoc, agc);
///				Printf("\n\r +++++++++lock=%2x", val);

				if(val==100)
					kk=0;
				else{
					agct = aoc;
					kk=1;
					for (ii=0;ii<5;ii++){
						if (ii==2) continue;
						aoct = aoc+0x10-(ii<<3);
						aoct = aoct>0xff?0xff:aoct;
						aoct = aoct<0x80?0x80:aoct;
						jj=lock_check(ch, aoct, agc);
						if (jj==100){
							kk=0;
							break;
						}else if(jj<val){
							val = jj;
							agct = aoct; // just use agct as a temporary holder				
						}
					}
				}
				val = 100-val;
			}else{
				agct = aoc;
				kk=1;
				for (ii=0;ii<5;ii++){
					if (ii==2) continue;
					aoct = aoc+0x10-(ii<<3);
					aoct = aoct>0xff?0xff:aoct;
					aoct = aoct<0x80?0x80:aoct;
					jj=crc_check(ch, aoct, agc);
					if (jj==0){
						kk=0;
						break;
					}else if(jj<val){
						val = jj;
						agct = aoct; // just use agct as a temporary holder				
					}
				}
			}
		}
	}

	if(kk==1){
		SetSignalTW6874(0x385+(ch<<4), 0x70, agct);
#ifdef AUTO_DEBUG
		Printf("\n\r: final aoc=%2x, agc=%2x", (WORD)agct, (WORD)agc);
#endif
		Ms_Delay(15);
	}
#endif

	minErr[ch] = 0;

#ifdef CRC_SCREEN
	if((ReadTW6874(ch<<1)&3)!=1){	//non dirac case;
		Printf("\n\r: =========== here is the crc count =============");
		Printf("\n\r	crc err = %4x", (WORD)CheckChCrc(ch, 750));
	}
#endif

//	SetSignalTW6874( 0x044 + (ch<<1), 0x77, 1 );	Printf("\n\r display turned on");	// turn on output during calibration;


// set audio and display
	if(val<0x20){	//make sure connection is good enough before turn on audio;
		ForceDisplay = 1;		//force display configuration; only used for Intersil evb;
		if((ReadTW6874(ch<<1)&3)!=1){	//HD case
				DiracInput[ch] = 0;
		/*
			switch(ch){
			case 0:
				set_dirac_func( 1, 8, ReadTW6874(0x83) );
				break;
			case 1:
				set_dirac_func( 2, 8, ReadTW6874(0x89) );
				break;
			case 2:
				set_dirac_func( 3, 8, ReadTW6874(0x93) );
				break;
			case 3:
				set_dirac_func( 4, 8, ReadTW6874(0x99) );
				break;
			}
		*/
			SetAudioCh(ch+1+((right_audio_chan>>ch)&1)*4, 1, 0);			// loop left or right channel from the port to DAC output depending on RIGHT_ACHAN setting;
//			Printf("\n\r set audio %2x, %2x, %2x", (WORD)(ch+1), (WORD)1, (WORD)0);
		}
		else{
		
			if(DiracAutoFlag)
				DiracInput[ch] = 1;
			/*
			else{
				switch(ch){
				case 0:
					set_dirac_func( 1, 8, ReadTW6874(0x83) );
					break;
				case 1:
					set_dirac_func( 2, 8, ReadTW6874(0x89) );
					break;
				case 2:
					set_dirac_func( 3, 8, ReadTW6874(0x93) );
					break;
				case 3:
					set_dirac_func( 4, 8, ReadTW6874(0x99) );
					break;
				}		
			}
		*/
			SetAudioCh(ch+1+((right_audio_chan>>ch)&1)*4, 0, 0);			// loop left or right channel from the port to DAC output depending on RIGHT_ACHAN setting;
//			Printf("\n\r set audio %2x, %2x, %2x", (WORD)(ch+1), (WORD)0, (WORD)0);
		}
	}
	else
		Printf("\n\r: audio not turned on because of poor connection");
//////////////////

//	return 1;
}
#endif


///////////////////////////////////
#if 1
U8 lock_check(U8 ch, U8 aoc, U8 agc)
{
	U16 bothLock, kk;
	U8 val;

	SetSignalTW6874(0x385+(ch<<4), 0x70, aoc);
	SetSignalTW6874(0x383+(ch<<4), 0x70, agc);

		    Delay(15);

			bothLock = 0;
			for(kk=0; kk<400; kk++){
				val = ReadTW6874(ch<<1);
				bothLock += ((val>>6)&1)&((val>>2)&1);
			}

///			Printf("\n\r:      lock percentage %d\n\r", (WORD)(bothLock>>2));

			return (bothLock>>2);
}
#endif


/****************
	Example codes for reading ANC info on TW6874;

			Printf("\r\nUsage: ANC_READ ch pos did line");
			Printf("\r\n\t<ch>  ... 1/2/3/4: SDI port 1/2/3/4 ");
			Printf("\r\n\t<ps>  ... 0/1: Luma/Chroma insertion ");
			Printf("\r\n\t<did> (optional)  ... did to receive ");
			Printf("\r\n\t<line> (optional)  ... which line to extract ");

*****************/

void ReadANC(U8 port, U8 pos, U8 did, U16 line)
{
	U8 rDid, rSDid, rDc, ch;
	U16 val, ii;

	ch = port -1;

	//disable and clear fifo
	SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  2);

	//configure and enable
	SetSignalTW6874( 0x806+((U16)ch<<8), 0x70,  did);
	
	Printf("\n\r: start read anc -- ch %2x, pos %2x, did %2x, line %4x", (WORD)ch, (WORD)pos, (WORD)did, (WORD)line);

	if(line>0){	//line number specified; //only take one configuration here;
		SetSignalTW6874( 0x810+((U16)ch<<8), 0x70,  line&0xff);
		SetSignalTW6874( 0x811+((U16)ch<<8), 0x70,  line>>8);
	}
	SetSignalTW6874( 0x821+((U16)ch<<8), 0x76,  0x0);

	SetSignalTW6874( 0x820+((U16)ch<<8), 0x70,  0xc0);

	if(pos==0)
		SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0x11);
	else if (pos==1)
		SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0x21);
	else
		SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0x31);

	//wait for ANC data
	Printf("\n\r: polling .");
	for(ii=0;ii<500;ii++){	//try polling here first;
//		if(ii%20 == 0) Printf(".");
		val = ReadTW6874(0x821+((U16)ch<<8));
//		if(((val&0x80)&&(pos==1))||((val&0x40)&&(pos==0))||((val&0xc0)&&(pos>1)))
		if((val&0xc0)&&(val&0x08))
			break;
		Delay(2);
	}

	Printf("\n\r: status %2x, pos %2x", (WORD)val, (WORD)pos);

	if(((val & 0xc0) == 0)&&((val & 0x08) == 0)){
		Printf("\n\r time out, no ANC data received");
		// disable and read data;
		SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0);
		return;
	}

	if(pos==0){ // luma case
		rDid = ReadTW6874(0x838+((U16)ch<<8));
		rSDid = ReadTW6874(0x839+((U16)ch<<8));
		rDc = ReadTW6874(0x83a+((U16)ch<<8));
		Printf("\n\r: Luma did %2x; sdid %2x; count %2x\n\r", (WORD)rDid, (WORD)rSDid, (WORD)rDc);

		// disable and read data;
//		SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  10);

		for (ii=0;ii<(rDc>256?256:rDc);ii++)
			Printf("%2x ", (WORD)ReadTW6874(0xc00+((U16)ch<<9)+(ii<<1)));
		Printf("\n\r");
	} else if(pos==1){ //chroma case
		rDid = ReadTW6874(0x83c + ((U16)ch<<8));
		rSDid = ReadTW6874(0x83d + ((U16)ch<<8));
		rDc = ReadTW6874(0x83e + ((U16)ch<<8));
		Printf("\n\r: Chroma did %2x; sdid %2x; count %2x\n\r", (WORD)rDid, (WORD)rSDid, (WORD)rDc);

		// disable and read data;
//		SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  20);

		for (ii=0;ii<(rDc>256?256:rDc);ii++)
			Printf("%2x ", (WORD)ReadTW6874(0xc01+((U16)ch<<9)+(ii<<1)));
		Printf("\n\r");
	} else {		// both
		if((rDc = ReadTW6874(0x83a+((U16)ch<<8)))>0){
			rDid = ReadTW6874(0x838+((U16)ch<<8));
			rSDid = ReadTW6874(0x839+((U16)ch<<8));
			Printf("\n\r: Luma did %2x; sdid %2x; count %2x\n\r", (WORD)rDid, (WORD)rSDid, (WORD)rDc);
			// disable and read data;
//			SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  30);
	
			for (ii=0;ii<(rDc>256?256:rDc);ii++)
				Printf("%2x ", (WORD)ReadTW6874(0xc00+((U16)ch<<9)+(ii<<1)));
			Printf("\n\r");
		}else if ((rDc = ReadTW6874(0x83e + ((U16)ch<<8)))>0){
			rDid = ReadTW6874(0x83c + ((U16)ch<<8));
			rSDid = ReadTW6874(0x83d + ((U16)ch<<8));
			Printf("\n\r: Chroma did %2x; sdid %2x; count %2x\n\r", (WORD)rDid, (WORD)rSDid, (WORD)rDc);

			// disable and read data;
//			SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  30);
	
			for (ii=0;ii<(rDc>256?256:rDc);ii++)
				Printf("%2x ", (WORD)ReadTW6874(0xc01+((U16)ch<<9)+(ii<<1)));
			Printf("\n\r");
		} else
			Printf("\n\r: data length is 0!!");

	}
		// disable and read data;
	SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0);

}

#ifdef RES_OVER_ANC

#define CAM1_DID	0x80
#define CAM1_SUB_DID	0x00
#define	MAX_CAM1_ANC_LEN	50
/******************
		* Try to read resolution info from transmitter;
		* an example of using ANC packet (generated by tw6872);
		* on tw6872, DID set to 0x80, line # to 7, pos to Luma;
*****************************/
void ReadANC_On(U8 ch, U8 did, U8 did2)
{
	//U8 line;

//	line = 7;

	//disable and clear fifo
	SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  2);

	//set did and sub did for extraction; by default sub did is 0;
	SetSignalTW6874( 0x806+((U16)ch<<8), 0x70,  did);
	SetSignalTW6874( 0x807+((U16)ch<<8), 0x70,  did2);
//	Printf("\n\r: set read Res -- ch %2x, pos %2x, did %2x, line %4x", (WORD)ch, (WORD)0, (WORD)0x80, (WORD)0);
/*
	// line number can be set here; or it can be ignored and only did/sub did used for extraction;
	if(line>0){	//line number specified; //only take one configuration here;
		SetSignalTW6874( 0x810+((U16)ch<<8), 0x70,  line&0xff);
		SetSignalTW6874( 0x811+((U16)ch<<8), 0x70,  line>>8);
	}
*/
	//clear ANC detection first;
	SetSignalTW6874( 0x821+((U16)ch<<8), 0x76,  0x0);
	//enable ANC detection;
	SetSignalTW6874( 0x820+((U16)ch<<8), 0x70,  0xc0);

//	if(pos==0)
		// enable extraction in Luma section;
		SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0x11);
}

// disable ANC extraction;
void ReadANC_Off(U8 ch)
{
		// disable 
	SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0);
}


/***
//	ReadANC_Data() extracts data according to given did and sub did;
//	as configured in ReadANC_Resolution_On(), only luma portion extraction is enabled, and TW6872 only inserts ANC packets in luma;	
***/
U8 ReadANC_Data(U8 ch, U8 did, U8 did2)
{
	U8 rDid, rSDid, rDc;
	U16 val, ii;

	//wait for ANC data
//	Printf("\n\r: polling .");

	//if wanted to remove past data in fifo. run below to clear first;
	// this can be done at the beginning or at the end of this routine. need to be done once depending on your application;
	SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0);
	ReadANC_On(ch, did, did2);
	///////////////////////////////////



	for(ii=0;ii<500;ii++){	//try polling here first to detect ANC packet;
//		if(ii%20 == 0) Printf(".");
		val = ReadTW6874(0x821+((U16)ch<<8));
//		if(((val&0x80)&&(pos==1))||((val&0x40)&&(pos==0))||((val&0xc0)&&(pos>1)))
		if((val&0xc0)&&(val&0x08))		// ANC packet detected;
			break;
		Delay(2);
	}

//	Printf("\n\r: status %2x, pos %2x", (WORD)val, (WORD)pos);

	if(((val&0xc0)==0)&&((val&0x08)==0)){
		Printf("\n\r time out, no ANC data received");
		ii=0xff;
	}
	else
//	if(pos==0)
	{ // luma case
		rDid = ReadTW6874(0x838+((U16)ch<<8));		// read did #;
		rSDid = ReadTW6874(0x839+((U16)ch<<8));		// read sdid #;
		rDc = ReadTW6874(0x83a+((U16)ch<<8));		// read packet byte count;
		Printf("\n\r: Luma did %2x; sdid %2x; count %2x\n\r", (WORD)rDid, (WORD)rSDid, (WORD)(rDc-1));

/*
		if(rDc!=1+1){		//count should be payload# + 1;
			SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0);
			ReadANC_Resolution_On(ch);
			return 0xff;
		}

		ii = ReadTW6874(0xc00+((U16)ch<<9));		
		Printf("\n\r ii=%2x ", (WORD)ii);
*/
//		if((rDc<51)&&(rDc>0)){
		if((rDc<=MAX_CAM1_ANC_LEN)&&(rDc>0)&&(rDid==did)&&(rSDid==did2)){	//only read data out when did/sdid matches ones for RESOLUTION ANC packet;
																				// there might be one more byte added for software crc checking;
			for(ii=0; ii<rDc-1; ii++)
				Printf("%c ", (WORD)ReadTW6874(0xc00+((U16)ch<<9)+(ii<<1)));
		}
		else
			Printf("\n\r data counter bigger than expected");
	}

		// disable 
	SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0);
	ReadANC_On(ch, did, did2);

	return ii;
}

U8 ReadANC_Resolution(U8 ch)
{
	U8 rDid, rSDid, rDc;
//	U8 pos, did, line;
	U16 val, ii;

	//wait for ANC data
#ifdef AUTO_DEBUG
//	Printf("\n\r: polling .");
#endif

	SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0);
	ReadANC_On(ch, 0x80, 0x0);
	for(ii=0;ii<500;ii++){	//try polling here first;
//		if(ii%20 == 0) Printf(".");
		val = ReadTW6874(0x821+((U16)ch<<8));
//		if(((val&0x80)&&(pos==1))||((val&0x40)&&(pos==0))||((val&0xc0)&&(pos>1)))
		if((val&0xc0)&&(val&0x08))
			break;
		Delay(2);
	}

//	Printf("\n\r: status %2x, pos %2x", (WORD)val, (WORD)pos);

	if(((val&0xc0)==0)&&((val&0x08)==0)){
#ifdef AUTO_DEBUG
//		Printf("\n\r time out, no ANC data received");
#endif
		// disable 
//		SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0);
//		return;
		ii=0xff;
	}
	else
//	if(pos==0)
	{ // luma case
		rDid = ReadTW6874(0x838+((U16)ch<<8));
		rSDid = ReadTW6874(0x839+((U16)ch<<8));
		rDc = ReadTW6874(0x83a+((U16)ch<<8));
#ifdef AUTO_DEBUG
//		Printf("\n\r: Luma did %2x; sdid %2x; count %2x\n\r", (WORD)rDid, (WORD)rSDid, (WORD)rDc);
#endif

		if((rDid==0x80)&&(rSDid==0x00)){
			// disable and read data;
			if(rDc!=1+1){	//count = payload len + 1 (soft crc)
				SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0);
				ReadANC_On(ch, rDid, rSDid);
				return 0xff;
			}
	
			ii = ReadTW6874(0xc00+((U16)ch<<9));		
		} else		//don't handle it if it's not resolution info;
			ii = 0xff;
#ifdef AUTO_DEBUG
//		Printf("\n\r ii=%2x ", (WORD)ii);
#endif
	}

		// disable 
//	SetSignalTW6874( 0x804+((U16)ch<<8), 0x70,  0);
//	ReadANC_On(ch, rDid, rSDid);

	switch (ii)
	{
		case 0: ii=RES_720P50; break;
		case 1:
		case 2: ii=RES_720P60; break;
		case 3: ii=RES_1080I50; break;
		case 4: 
		case 5: ii=RES_1080I60; break;
		case 6: ii=RES_1080P25; break;
		case 7: 
		case 8: ii=RES_1080P30; break;
		case 9: ii=RES_1080P50; break;
		case 10: 
		case 11: ii=RES_1080P60; break;
		case 12: ii=RES_NTSC; break;
		case 13: ii=RES_PAL; break;
		default: 
			//Printf("\n\r: wrong resolution value from ANC %2x", (WORD)ii); 
			ii= 0xff; break;
	}
	return ii;
}
#endif

/* ///////////////////////////////////////////////////////
	Input Channe --
		1/2/3/4: SDI_L
		5/6/7/8: SDI_R
		9-12: AD input
		13 and up: cascade in
	Output Channel --
		0: ADTR
		1: ADTM
		2: DAC
	Input Mode --
		0: SD
		1: HD
		2: 3G
	//////////////////////////////////////////////////////*/
void SetAudioCh(U8 InCh, U8 InMode, U8 OutCh)
{
	/////////////////////
	// for now we only consider one input channel to I2S ADTR at first time slot
	// also 0x134 always set to 16 bit mode
	/////////////////////
	U8 ch, port;
	ch = (InCh-1)&0x3;	//only consider local inputs;
	port = (InCh-1)>>2; //0: SDI_L; 1: SDI_R; 2: ADC;

	if(OutCh!=0) // not ADTR output
		return;		//for now only consider ADTR;
	
	SetSignalTW6874(0x103, 0x30, ch);	//assuminig only set inut ch to first time slot;
	SetSignalTW6874(0x135, (ch<<4)|ch, 1-(port>>1));
	SetSignalTW6874(0x0f7, (ch<<4)|ch, (port&1));
	if(InMode == 0) //SD
	{
		SetSignalTW6874(0x800+(ch<<8), 0x70, 0x41);
		SetSignalTW6874(0x840+(ch<<8), 0x70, 0x01);
		SetSignalTW6874(0x84a+(ch<<8), 0x70, 0x0);
		SetSignalTW6874(0x84b+(ch<<8), 0x70, 0x0c);
		SetSignalTW6874(0x84c+(ch<<8), 0x70, 0x0);
		SetSignalTW6874(0x84d+(ch<<8), 0x70, 0x10);
		SetSignalTW6874(0x311, (ch<<4)|ch, 0x1);
	} else {
		SetSignalTW6874(0x800+(ch<<8), 0x70, 0x01);
		SetSignalTW6874(0x311, (ch<<4)|ch, 0x0);
	}
	SetSignalTW6874(0x3c, (ch<<4)|ch, 0x0);
	SetSignalTW6874(0x3c, (ch<<4)|ch, 0x1);
}




/****************
//	example codes: check PRBS error counts
*****************/
typedef enum {
	PRBS23 = 0,
	PRBS7,
	PRBS_MODEL_END
} prbs_model_e;

typedef enum {
	PRBS_HD = 0,
	PRBS_SD,
	PRBS_3G,
	PRBS_MODE_END
}prbs_mode_e;

U8 prbs_check(U8 ch, prbs_model_e model, prbs_mode_e mode)
{
	U8 val, ii;

	SetSignalTW6874(0x058, (ch<<4)+ch, 0);	//disable channel
	SetSignalTW6874(0x059, (((ch<<1)+1)<<4)+(ch<<1), mode);	//set mode: HD/SD
	SetSignalTW6874(0x062+(ch<<2), 0x70, 0);			//set PRBS window
	SetSignalTW6874(0x061+(ch<<2), 0x70, 0x40);			// set PRBS window
	SetSignalTW6874(0x058, ((ch+4)<<4)+(ch+4), model&1);	//set model: PRBS7 or PRBS23;
	SetSignalTW6874(0x058, (ch<<4)+ch, 1);	//enable
	Ms_Delay(1);							//delay about 1ms;

	SetSignalTW6874(0x5e, (ch<<4)+ch, 0);	//clear request bit;
	SetSignalTW6874(0x5e, (ch<<4)+ch, 1);	//set request bit;

	//wait until result is available;
	for (ii=0; ii<10; ii++){
		Ms_Delay(1);
		if((ReadTW6874(0x057)&(1<<(ch+4)))!=0)	//the bit equal to 1 indicates that result is available;
			break;
	}

	val = ReadTW6874(0x05a+ch);		// read PRBS result;
	SetSignalTW6874(0x058, (ch<<4)+ch, 0);	//disable

//	Printf("\n\r: prbs returned %2x, mode=%2x", (WORD)val, (WORD)mode);
	return val;
}

