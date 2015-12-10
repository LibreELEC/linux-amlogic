#ifndef _SM1828_H__
#define _SM1828_H__

#ifdef _SM1828_H__

//#include "hi_type.h"
//#include "hi_gpio.h"
//#include "hi_unf_ecs.h"
//#include "hi_error_ecs.h"
//#include "priv_keyled.h"
#include <mach/gpio.h>
#define SM1628_DISPLAY_REG_NUM  (14)
/*sm1628 command*/
#define CMD1_DISP_MODE_1   (0x3)  
#define CMD1_LED_ADDR_BASE (0xc0)
#define CMD1_LED_WRITE_INC (0x40)
#define CMD1_DIP_ON        (0x8a)
#define CMD1_DIP_OFF       (0x80)

//#define GROUP   (5)
//#define CLK     (2)
//#define STB     (3)
//#define DAT     (4)
//#define LED     (5)
//#define KEY     (6)
#define LED_STB GPIOX_17
#define LED_CLK GPIOX_18
#define LED_DAT GPIOX_19

#define HI_SM1628_MINOR  MISC_DYNAMIC_MINOR
#define SM1628_LIGHT_OFF     	  		 _IO('S' , 0 )
#define SM1628_LIGHT_ON_ZERO        		 _IO('S' , 1 )

//#define  GPIO_CLOCK_SET(val)    hi_gpio_write_bit(GROUP, CLK, val)
//#define  GPIO_DATA_SET(val)     hi_gpio_write_bit(GROUP, DAT, val)
//#define  GPIO_STB_SET(val)      hi_gpio_write_bit(GROUP, STB, val)
//#define  GPIO_LED_SET(val)      hi_gpio_write_bit(GROUP, LED, val)
//#define  GPIO_KEY_GET(val)      hi_gpio_read_bit(GROUP, KEY, &val)
#define  GPIO_CLOCK_SET(val)    gpio_set_value(LED_CLK,val)
#define  GPIO_DATA_SET(val)      gpio_set_value(LED_DAT,val)
#define  GPIO_STB_SET(val)        gpio_set_value(LED_STB,val)

typedef   unsigned char    HI_U8;
typedef   unsigned int     HI_U32;
typedef   signed   int     HI_S32;
#define   HI_SUCCESS          0

HI_U8 v_LedCode[SM1628_DISPLAY_REG_NUM] = {0x1f};
HI_U8 CHANGE_Code[7] = {0xff,0xff,0xff,0xff,0xff,0xff,0x00};
static HI_U32 LedDigDisDot_sm1628[10]  = {0xbf, 0x86, 0xf3, 0xe7, 0xce, 0xed, 0xfd, 0x87, 0xff, 0xef};

#define udelay_sm1628(x)  do{	HI_U8 s;	for(s = 0; s < x; s++);}while(0) 

typedef struct zeko_time_s{
        HI_U8 hour;
        HI_U8 minute;
        HI_U8 second;
}ZEKO_TIME;


#endif
#endif
