/*
 * MIG-MAG_Timer.c
 *
 * Created: 21.04.2016 15:35:57
 * Author : dark
 */ 

#define F_CPU 16000000UL
//------------- INC -------------------------
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

//------------- DEF -------------------------
#define _NOP() do { __asm__ __volatile__ ("nop"); } while (0)
#define HI(x) ((x)>>8)
#define LO(x) ((x)& 0xFF)

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

    #define OUT_PORT	PORTA
    #define OUT_DDR		DDRA
    #define WIRE		0
    #define GAS			1

    #define KEYS_PORT	PORTD
    #define KEYS_PIN	PIND
    #define KEYS_DDR	DDRD
    #define BUTTON		2
    #define KEY			3
    #define ENC1		4
    #define ENC2		5
#endif // _AVR_IOTN2313_H_
#ifdef _AVR_IOM328P_H_
    #define IND_PORT    PORTB
    #define IND_DDR     DDRB
    #define IND_SCK     5
    #define IND_MOSI    3
    #define IND_STORE   2

    #define OUT_PORT	PORTD
    #define OUT_DDR		DDRD
    #define WIRE		6
    #define GAS			7

    #define KEYS_PORT	PORTD
    #define KEYS_PIN	PIND
    #define KEYS_DDR	DDRD
    #define BUTTON		5
    #define KEY			4
    #define ENC1		2
    #define ENC2		3
#endif // _AVR_IOM328P_H_

#define KEY_CLICK_F 0
#define KEY_PRESS_F 1
#define ENC_INC_F	2
#define ENC_DEC_F	3
#define BUTTON_F    4
#define ENC_MASK	(1<<ENC2 | 1<<ENC1)
#define ENC_S0		(1<<ENC2 | 1<<ENC1)
#define ENC_S1		(1<<ENC2 | 0<<ENC1)
#define ENC_S2		(0<<ENC2 | 0<<ENC1)
#define ENC_S3		(0<<ENC2 | 1<<ENC1)

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

// ============ Global ===============

const int8_t digits[] = {
    0b1111110, //0
    0b0110000, //1
    0b1101101, //2
    0b1111001, //3
    0b0110011, //4
    0b1011011, //5
    0b1011111, //6
    0b1110000, //7
    0b1111111, //8
    0b1111011, //9
    0b1110111, //A
    0b0011111, //B
    0b1001110, //C
    0b0111101, //D
    0b1001111, //E
    0b1000111, //F
};

#define SYM_EMPTY 0b0000000
#define SYM_G	0b1000110
#define SYM_A	0b1110111
#define SYM_S	0b1111001
#define SYM_P	0b1110110
#define SYM_R	0b1100111
#define SYM_O	0b1111110
#define SYM_V	0b1111111
#define SYM_F1	0b1000000
#define SYM_F2	0b0000001
#define SYM_F3	0b0001000
#define SYM_R1	0b0101011
#define SYM_R2	0b1001011
#define SYM_R3	0b1101010
#define SYM_R4	0b1101001
#define SYM_H	0b0110111
#define SYM_E	0b1001111
#define SYM_L	0b0001110


volatile uint8_t Cnt100ms = C100MS_INIT;
volatile uint8_t ActionFlags = 0;
volatile uint8_t Displayed[NUM_OF_DIGITS];

uint8_t eeDelay1 EEMEM = 5;
uint8_t eeDelay2 EEMEM = 10;

uint8_t Delay1;
uint8_t Delay2;

// ============== Interrupts =============

ISR(TIMER0_COMPA_vect) {
    ActionFlags |= 1<<KEYSCAN_FLAG;
    ActionFlags |= 1<<NEXT_DIGIT_FLAG;
    Cnt100ms--;
    if (Cnt100ms == 0) {
        ActionFlags |= 1<<C100MS_FLAG;
        Cnt100ms = C100MS_INIT;
    }        
}

// ============== Init ===============
void init(void) {
    // Ports setup
    KEYS_PORT |= 1<<ENC1 | 1<<ENC2 | 1<<KEY | 1<<BUTTON; // Pull-Up Resistors
	//OUT_PORT |= 1<<WIRE | 1<<GAS; // Out
	OUT_DDR |= 1<<WIRE | 1<<GAS; // Direction  - Out
    
    // SPI setup
    IND_DDR  |= 1<<IND_SCK | 1<<IND_MOSI | 1<<IND_STORE; // Direction  - Out
    #ifdef _AVR_IOM328P_H_
        // TODO: mega SPI setup
        /* Enable SPI, Master, set clock rate fck/4 */
        SPCR = 1<<SPE | 1<<MSTR | 0b00<<SPR0;
    #endif // _AVR_IOM328P_H_
    
    // Timer setup
    TCCR0A = 0b10 << WGM00;
    TCCR0B = 0 << WGM02;
    #if F_CPU == 1000000UL
        TCCR0B |= 0b010 << CS00; // 001 - 1; 010 - 8; 011 - 64; 100 - 256; 101 - 1024
        OCR0A = 124;
    #endif
    #if F_CPU == 8000000UL
        TCCR0B |= 0b011 << CS00; // 001 - 1; 010 - 8; 011 - 64; 100 - 256; 101 - 1024
        OCR0A = 124;
    #endif
    #if F_CPU == 16000000UL
        TCCR0B |= 0b011 << CS00; // 001 - 1; 010 - 8; 011 - 64; 100 - 256; 101 - 1024
        OCR0A = 249;
    #endif
    #ifdef _AVR_IOTN2313_H_
        TIMSK = 1<<OCIE0A;
    #endif // _AVR_IOTN2313_H_
    #ifdef _AVR_IOM328P_H_
        TIMSK0 = 1<<OCIE0A;
    #endif // _AVR_IOM328P_H_
    
	Delay1 = eeprom_read_byte(&eeDelay1);
	Delay2 = eeprom_read_byte(&eeDelay2);
	
    sei();
}

void SPIOut(uint8_t cData) {
    #ifdef _AVR_IOTN2313_H_
        USIDR = cData;
        USISR |= 1<<USIOIF;
        while (~USISR & 1<<USIOIF) {
            USICR = 0b01<<USIWM0 | 0b10<<USICS0 | 1<<USICLK | 1<<USITC;
        }
    #endif // _AVR_IOTN2313_H_
    #ifdef _AVR_IOM328P_H_
        /* Start transmission */
        SPDR = cData;
        /* Wait for transmission complete */
        while(~SPSR & 1<<SPIF);
    #endif // _AVR_IOM328P_H_
}

void SendSymbol(uint8_t n, uint8_t a) {
    SPIOut(1<<n);
    SPIOut(~a);

    IND_PORT |= 1<<IND_STORE;
    IND_PORT &= ~(1<<IND_STORE);
}

uint8_t ReadKeyState() {
    static uint16_t KeyTime = 0;
    static uint8_t EncState = ENC_S0;
    static int8_t EncCnt = 0;
    uint8_t KeyFlags = 0;
    
	// get key status
	if (~KEYS_PIN & 1<<KEY) {
    	if (KeyTime == 1000) {
        	// Press
        	KeyFlags |= 1<<KEY_PRESS_F;
    	}
    	if (KeyTime < 1000+1) {
        	KeyTime++;
    	}
    } else {
    	if (KeyTime > 10 && KeyTime < 1000) {
        	// Click
        	KeyFlags |= 1<<KEY_CLICK_F;
    	}
    	KeyTime = 0;
	}
	if (~KEYS_PIN & 1<<BUTTON) {
        KeyFlags |= 1<<BUTTON_F;
    }        
			
	uint8_t EncNew;
	// Get new Encoder status
			
	EncNew = KEYS_PIN & ENC_MASK;
			
	switch (EncState) {
    	case ENC_S0: {
        	if (EncNew == ENC_S1) EncCnt++;
        	if (EncNew == ENC_S3) EncCnt--;
        	break;
    	}
    	case ENC_S1: {
        	if (EncNew == ENC_S2) EncCnt++;
        	if (EncNew == ENC_S0) EncCnt--;
        	break;
    	}
    	case ENC_S2: {
        	if (EncNew == ENC_S3) EncCnt++;
        	if (EncNew == ENC_S1) EncCnt--;
        	break;
    	}
    	default: {
        	if (EncNew == ENC_S0) EncCnt++;
        	if (EncNew == ENC_S2) EncCnt--;
        	break;
    	}
	}
	EncState = EncNew;
			
	if (EncState == ENC_S0) {
    	if (EncCnt > 0) KeyFlags |= 1<<ENC_INC_F;
    	if (EncCnt < 0) KeyFlags |= 1<<ENC_DEC_F;
    	EncCnt = 0;
	}
    return KeyFlags;
}

uint8_t WorkStateSwitch(uint8_t State, uint8_t KeyFlags) {
    static uint8_t DelayCnt = 0;

    switch (State) {
        case STATE_IDLE: {
            if (KeyFlags & 1<<BUTTON_F) {
                Cnt100ms = C100MS_INIT;
                ActionFlags &= ~(C100MS_FLAG);
                DelayCnt = Delay1;
                State = STATE_WORK_PRE_GAS;
            }
            break;
        }
        case STATE_WORK_PRE_GAS: {
            if (KeyFlags & 1<<BUTTON_F) {
                // Delay
                if (ActionFlags & 1<<C100MS_FLAG) {
                    ActionFlags &= ~(1<<C100MS_FLAG);
                    DelayCnt--;
                }
                if (DelayCnt == 0) {
                    State = STATE_WORK_WELD;
                }
            } else {
                State = STATE_IDLE;
            } // if button
            break;
        } // case before
        case STATE_WORK_WELD: {
            if (~KeyFlags & 1<<BUTTON_F) {
                Cnt100ms = C100MS_INIT;
                ActionFlags &= ~(1<<C100MS_FLAG);
                DelayCnt = Delay2;
                State = STATE_WORK_POST_GAS;
            }
            break;
        }
        case STATE_WORK_POST_GAS: {
            if (KeyFlags & 1<<BUTTON_F) {
                State = STATE_WORK_WELD;
            } else {
                // Delay
                if (ActionFlags & 1<<C100MS_FLAG) {
                    ActionFlags &= ~(1<<C100MS_FLAG);
                    DelayCnt--;
                }
                if (DelayCnt == 0) {
                    State = STATE_IDLE;
                }
            } // if button
            break;
        } // case post
    } // switch state
    return State;
}

uint8_t BypassStateSwitch(uint8_t CurrentMode, uint8_t KeyFlags) {
    uint8_t State = STATE_IDLE;
    if (KeyFlags & 1<<BUTTON_F) {
        switch (CurrentMode) {
            case MODE_BYPASS_GAS: {
                State = STATE_BYPASS_GAS;
                break;
            }
            case MODE_BYPASS_WIRE: {
                State = STATE_BYPASS_WIRE;
                break;
            }
        }
    }
    return State;
}    

void StateProcessing(uint8_t State) {
    if (State == STATE_IDLE || State == STATE_BYPASS_WIRE) {
        OUT_PORT &= ~(1<<GAS);
    } else {
        OUT_PORT |= 1<<GAS;
    }
    if (State == STATE_WORK_WELD || State == STATE_BYPASS_WIRE) {
        OUT_PORT |= 1<<WIRE;
    } else {
        OUT_PORT &= ~(1<<WIRE);
    }        
}

void IncreaseDelay(uint8_t *Delay) {
	// TODO: Increase
	(*Delay)++;
}

void DecreaseDelay(uint8_t *Delay) {
	// TODO: Decrease
	(*Delay)--;
}

void ModeProcessing(uint8_t KeyFlags) {
	
	static uint8_t CurrentMode = MODE_WORK;
	static uint8_t State = STATE_IDLE;
	uint8_t NewMode = MODE_NOCHANGE;

	// Define Action and NewMode
	switch (CurrentMode) {
		case MODE_WORK: {
			// TODO: Show Delay1 & Delay2 in decimal
			Displayed[3] = digits[Delay1>>4  & 0xF];
			Displayed[2] = digits[Delay1 & 0xF];
			Displayed[1] = digits[Delay2>>4  & 0xF];
			Displayed[0] = digits[Delay2 & 0xF];

			if (KeyFlags & 1<<KEY_CLICK_F) NewMode = MODE_SET1;
			if (KeyFlags & 1<<KEY_PRESS_F) NewMode = MODE_BYPASS_WIRE;
            
            State = WorkStateSwitch(State, KeyFlags);
			break;
		}
		case MODE_SET1: {
			// TODO: Blink Delay1
			Displayed[3] = digits[Delay1>>4  & 0xF];
			Displayed[2] = digits[Delay1 & 0xF];
			Displayed[1] = Displayed[0] = SYM_EMPTY;
					
			if (KeyFlags & 1<<ENC_INC_F) IncreaseDelay(&Delay1);
			if (KeyFlags & 1<<ENC_DEC_F) DecreaseDelay(&Delay1);
			if (KeyFlags & 1<<KEY_CLICK_F) NewMode = MODE_SET2;
			if (KeyFlags & 1<<KEY_PRESS_F) NewMode = MODE_BYPASS_WIRE;
			break;
		}
		case MODE_SET2: {
			// TODO: Blink Delay2
			Displayed[3] = Displayed[2] = SYM_EMPTY;
			Displayed[1] = digits[Delay2>>4  & 0xF];
			Displayed[0] = digits[Delay2 & 0xF];
					
			if (KeyFlags & 1<<ENC_INC_F) IncreaseDelay(&Delay2);
			if (KeyFlags & 1<<ENC_DEC_F) DecreaseDelay(&Delay2);
			if (KeyFlags & 1<<KEY_CLICK_F) {
				eeprom_write_byte(&eeDelay1, Delay1);
				eeprom_write_byte(&eeDelay2, Delay2);
				NewMode = MODE_WORK;
			}
			if (KeyFlags & 1<<KEY_PRESS_F) {
				Delay1 = eeprom_read_byte(&eeDelay1);
				Delay2 = eeprom_read_byte(&eeDelay2);
				NewMode = MODE_WORK;
			}
			break;
		}
		case MODE_BYPASS_WIRE: {
			// Show "WIRE"
			Displayed[3] = SYM_EMPTY;
			Displayed[2] = SYM_P;
			Displayed[1] = SYM_R;
			Displayed[0] = SYM_O;
					
			if (KeyFlags & 1<<KEY_CLICK_F) NewMode = MODE_BYPASS_GAS;
			if (KeyFlags & 1<<KEY_PRESS_F) NewMode = MODE_WORK;
            
            State = BypassStateSwitch(CurrentMode, KeyFlags);
			break;
		}
		case MODE_BYPASS_GAS: {
			// Show "GAS"
			Displayed[3] = SYM_EMPTY;
			Displayed[2] = SYM_G;
			Displayed[1] = SYM_A;
			Displayed[0] = SYM_S;
					
			if (KeyFlags & 1<<KEY_CLICK_F) NewMode = MODE_BYPASS_WIRE;
			if (KeyFlags & 1<<KEY_PRESS_F) NewMode = MODE_WORK;
            
            State = BypassStateSwitch(CurrentMode, KeyFlags);
			break;
		}
	}
			
	// Switch mode
	if (NewMode != MODE_NOCHANGE) {
		CurrentMode = NewMode;
		State = STATE_IDLE;
	}
    
    StateProcessing(State);
}

// ============== Main ===============
int main(void) {
    init();
    uint8_t KeyFlags;
    uint8_t CurrentDigit = 0;
	
    while (1) {
		if (ActionFlags & 1<<KEYSCAN_FLAG) {
			ActionFlags &= ~(1<<KEYSCAN_FLAG);
			
            KeyFlags = ReadKeyState();
            
			ModeProcessing(KeyFlags);
		}
			
        
		
        if (ActionFlags & 1<<NEXT_DIGIT_FLAG) {
            ActionFlags &= ~(1<<NEXT_DIGIT_FLAG);
            SendSymbol(CurrentDigit, Displayed[CurrentDigit]);
            if(CurrentDigit) {
                CurrentDigit--;
            } else {
                CurrentDigit = NUM_OF_DIGITS;
            }
        }
        
    }
}



