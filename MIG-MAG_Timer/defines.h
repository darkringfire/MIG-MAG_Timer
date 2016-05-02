#ifndef DEFINES_H
#define DEFINES_H

//------------- DEF -------------------------
#define _NOP() do { __asm__ __volatile__ ("nop"); } while (0)
#define HI(x) ((x)>>8)
#define LO(x) ((x)& 0xFF)
#define soft_reset()            \
    while(1) {                  \
        wdt_enable(WDTO_15MS);  \
        while(1) {}             \
    }    

#define  NUM_OF_DIGITS 4

#define NEXT_DIGIT_FLAG 0
#define KEYSCAN_FLAG    1
#define C100MS_FLAG     2

#define C100MS_INIT 100

#ifdef _AVR_IOTN2313_H_
#define IND_PORT	PORTB
#define IND_DDR		DDRB
#define IND_SCK		7
#define IND_MOSI	6
#define IND_STORE	4

#define OUT_PORT    PORTA
#define OUT_DDR     DDRA
#define WIRE        0
#define GAS         1

#define KEYS_PORT   PORTD
#define KEYS_PIN    PIND
#define KEYS_DDR    DDRD
#define BUTTON      2
#define KEY         3
#define ENC_CCW     4
#define ENC_CW      5
#endif // _AVR_IOTN2313_H_
#ifdef _AVR_IOM328P_H_
#define IND_PORT    PORTB
#define IND_DDR     DDRB
#define IND_SCK     5
#define IND_MOSI    3
#define IND_STORE   2

#define OUT_PORT    PORTB
#define OUT_DDR     DDRB
#define WIRE        1
#define GAS         0

#define KEYS_PORT	PORTD
#define KEYS_PIN	PIND
#define KEYS_DDR	DDRD
#define ENC_CCW		4
#define ENC_CW		5
#define BUTTON		6
#define KEY			7
#endif // _AVR_IOM328P_H_

#define KEY_CLICK_F     0
#define KEY_PRESS_F     1
#define KEY_LONGPRESS_F 2
#define ENC_INC_F       3
#define ENC_DEC_F       4
#define BUTTON_F        5
#define ENC_MASK    (1<<ENC_CW | 1<<ENC_CCW)
#define ENC_S0      (1<<ENC_CW | 1<<ENC_CCW)
#define ENC_S1      (1<<ENC_CW | 0<<ENC_CCW)
#define ENC_S2      (0<<ENC_CW | 0<<ENC_CCW)
#define ENC_S3      (0<<ENC_CW | 1<<ENC_CCW)

#define MODE_NOCHANGE		0
#define MODE_WORK			1
#define MODE_SET1			2
#define MODE_SET2			3
#define MODE_BYPASS_WIRE	4
#define MODE_BYPASS_GAS		5

#define STATE_IDLE			0

#define STATE_WORK_PRE_GAS	1
#define STATE_WORK_WELD		2
#define STATE_WORK_POST_GAS	3
#define STATE_BYPASS_WIRE   4
#define STATE_BYPASS_GAS    5

#endif


