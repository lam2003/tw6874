/***************************************** (C) COPYRIGHT 2014 Intersil *****************************************
* File Name         : hst.h
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

#ifndef	__HST_H__
#define	__HST_H__

					 
#define	TW2828_dummyIndex	(0xFF)
#define	TW6874_dummyIndex	(0xFF)
#define	FPGA_dummyIndex		(0xFF)

#define TW2828_SLAVE_ID0 	0x58

#define TW6874_SLAVE_ID1	0xd2
#define SIL9034_SLAVE_ID0	0x39
#define	FPGA_SLAVE_ID0		0x28

// Default User Define : 720p60 //1080p60
#define DEFAULT_MUX_RES		RES_1080P60 //RES_720P60

//==================================================================================
//						Host definition & variable description
//==================================================================================
//...	w78le516 port define
#if 0
	#define	MSE_CLK		P3_2
	#define	MSE_DAT		P1_6
	
	#define	I2C_SCL		P3_4
	#define	I2C_SDA		P3_5
	#define	I2C_ADDR	P2_5	// FIXME forgot to sense this header at the micro-controller. Set S1 to match the I2C_ADDR header

	#define SPI_CS0		P1_1
	#define SPI_MOSI	P1_2	
	#define SPI_MISO	P1_3
	#define SPI_SCLK	P1_4
	#define SPIL		P1_5	

	#define	KEY0		P0_0	// cycle through ports 1-4 for display
	#define	KEY1		P0_1	// toggle between FPGA test pattern and TW6874 output port for display
	#define	KEY2		P0_2	// config an SDI input after it has valid data connected
	#define	KEY3		P0_3
	#define	KEY4		P0_4
	
	#define	JP_VDO		P4_0
	
	#define	RSTn			P1_7
	#define	RSTn_2828		P2_2
	#define	RSTn_6874		P2_1
#endif
//==================================================================================
//						Host function prototype description
//==================================================================================
extern void assert_reset_device(void);
extern void	Init_Video_Format(void);
void InitPorts(void);

int 	key_function( void );  // Added by Richard


#endif	//... #ifndef	__HST_H__
