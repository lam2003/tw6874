/***************************************** (C) COPYRIGHT 2014 Intersil *****************************************
* File Name         : typedefs.h
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

#ifndef	__TYPEDEFS__
#define	__TYPEDEFS__

#ifdef TASKINGC

#define DATA			_data
#define PDATA			_pdat
#define IDATA			idat
#define CODE			_rom
#define REENTRANT		_reentrant
#define	AT(atpos2)		_at( atpos2 )
#define CODE_P			_rom
#define PDATA_P			_pdatykim
#define IDATA_P			idat
#define DATA_P			_data

#define INTERRUPT( num, name )	_interrupt(num) void name (void)

#elif defined KEILC

#define DATA		data
#define PDATA		pdata
#define IDATA		idata
#define XDATA		xdata
#define CODE		code
#define CODE_P
#define PDATA_P
#define IDATA_P
#define DATA_P

#define INTERRUPT( num, name ) name() interrupt num

#endif


#define	BIT0		(0x0001)
#define	BIT1		(0x0002)
#define	BIT2		(0x0004)
#define	BIT3		(0x0008)
#define	BIT4		(0x0010)
#define	BIT5		(0x0020)
#define	BIT6		(0x0040)
#define	BIT7		(0x0080)
#define	BIT8		(0x0100)
#define	BIT9		(0x0200)
#define	BIT10		(0x0400)
#define	BIT11		(0x0800)
#define	BIT12		(0x1000)
#define	BIT13		(0x2000)
#define	BIT14		(0x4000)
#define	BIT15		(0x8000)

//#define	BIT16		(0x00010000)
//#define	BIT17		(0x00020000)
//#define	BIT18		(0x00040000)
//#define	BIT19		(0x00080000)
//#define	BIT20		(0x00100000)
//#define	BIT21		(0x00200000)
//#define	BIT22		(0x00400000)
//#define	BIT23		(0x00800000)
//#define	BIT24		(0x01000000)
//#define	BIT25		(0x02000000)
//#define	BIT26		(0x04000000)
//#define	BIT27		(0x08000000)
//#define	BIT28		(0x10000000)
//#define	BIT29		(0x20000000)
//#define	BIT30		(0x40000000)
//#define	BIT31		(0x80000000)

//==================================================================================
//#define	NULL		0
#define	ON			1
#define	OFF			0

//==================================================================================
#define	SetBit(x,y)			((x) |= (y))         
#define	ClearBit(x,y)		((x) &= ~(y))        
#define	BitSet(x,y)			(((x)&(y))== (y))    
#define	BitClear(x,y)		(((x)&(y))== 0)      
#define	IfBitSet(x,y)		if(((x)&(y)) == (y)) 
#define	IfBitClear(x,y)	if(((x)&(y)) == 0)


#define _between(x, a, b)	(a<=x && x<=b)
#define _swap(a, b)			{ a^=b; b^=a; a^=b; }

typedef	unsigned char	Register;
typedef	unsigned char	BYTE;
typedef	unsigned int	WORD;
typedef	unsigned long	DWORD;


typedef	unsigned char	U8;
typedef	unsigned short	U16;
//typedef	unsigned long	u32;
typedef	signed char		S8;
typedef	signed short		S16;

#define u8		U8
#define u16		U16
#define s8		S8
#define s16		S16


typedef	unsigned char	_u_;

typedef struct 
{
    _u_     B0    :     1;
    _u_     B1    :     1;
    _u_     B2    :     1;
    _u_     B3    :     1;
    _u_     B4    :     1;
    _u_     B5    :     1;
    _u_     B6    :     1;
    _u_     B7    :     1;
} bit_t;


typedef union
{
    bit_t     BIT;
    _u_     ALL;
} reg_bit_u;


#define	TRUE	1
#define	FALSE	0

//#define AUTO	0

typedef enum {
  CH1,
  CH2,
  CH3,
  CH4,	
  FULL_DIVISION
} ch_division_e;

#define HPOSITIVE 0x02
#define HNEGATIVE 0x00
#define VPOSITIVE 0x01
#define VNEGATIVE 0x00
#define INTERLACE 0x10
#define VGA_ON	  0x04
#define PAL4DM	  0x08

//VInputStd

typedef enum {
	NTSC =	1, PAL, SECAM, NTSC4, PALM, PALN, PAL60
} video_dec_format_e;

typedef enum { 
	RES_1080P60,
	RES_1080P50,
	RES_720P50,
	RES_720P60,
	RES_1080I60,
	RES_1080I50,
	RES_1080P30,
	RES_1080P25,
	RES_NTSC,
	RES_PAL,
	RES_720P25,		//kb: added;
	RES_720P30,		//kb: added;
	RES_NONE = 0xFF
} hd_dis_resolution_e;

/* //Original
typedef enum {
	DIREC_RES_720P50,
	DIREC_RES_720P60,
	DIREC_RES_1080P25,
	DIREC_RES_1080P30
} direc_dis_resolution_e;
*/

//Eric change
typedef enum {
	DIRAC_RES_720P60,
	DIRAC_RES_720P50,
	DIRAC_RES_1080I60,
	DIRAC_RES_1080I50,
	DIRAC_RES_NONE1,
	DIRAC_RES_NONE2,
	DIRAC_RES_1080P30,
	DIRAC_RES_1080P25
} dirac_dis_resolution_e;

typedef enum {
	CH_NUM0 = 0,
	CH_NUM1,
	CH_NUM2,
	CH_NUM3
} channel_number_e;

enum {
	VP1 = 1,
	VP2,
	VP3,
	VP4,
	FPGA_TEST
};

enum {
	SDI1 = 1,
	SDI2,
	SDI3,
	SDI4
};

#define Dram_LCD_0	  0x00
#define Dram_LCD_90	  0x01
#define Dram_LCD_180	0x02
#define Dram_LCD_270	0x03

enum {
	q_4,
	q_4_I, 
	q_6,  
	q_6_I, 
	q_7,  
	q_7_I, 
	q_8,  
	q_8_I, 
	q_10, 
	q_10_I, 
	q_12, 
	q_12_I, 
	q_28, 
	q_28_I, 
	q_16, 
	q_16_I
} ;

typedef struct format_ {
	u8 val, std;
} format_t;

typedef struct CLKType {
	u32 freq;
	u8 n, m, q, phase;
} _ClkType;

typedef enum { 
	DI_OFF, 
	DI_2D, 
	DI_3D,	
	DI_WEAVE 
} _DIType;

typedef struct
{
	U16 addr;
	U8 value;
} reg_setting2_t;

typedef struct
{
	U8 addr;
	U8 value;
} reg_setting_t;

#define MAXINPUTSTD	PAL60	// 

#define UNKNOWN	0xfe
#define NOINPUT	0	//0xff


typedef struct _BITMAP{
	unsigned short height;				//vertical size
	unsigned short width;				// horizontal size			
	unsigned char *bmpdata;				//bitmap data pointer
} BITMAP;

typedef struct mTimingType {
	u32 vclk;
	u16 htt, vtt; // Htotal-1, VTotal-1
	u16 hde, vde; // HActive, VActive
	u16 hfp, vfp; // frontporch
	u8  hs, vs;   // Syncwidth
	u8  ctrl;	    // bit0:Vsync, bit1:Hsync, bit2:interlace/VGA
	u8  *str;
	u8  vid; 	    //video ID for HDMI avi info
	u8  vsdelay;
} _mTimingType;

#define NIL			0xff

#endif	/* __TYPEDEFS__ */
