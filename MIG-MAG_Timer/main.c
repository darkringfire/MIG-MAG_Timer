/*
 * MIG-MAG_Timer.c
 *
 * Created: 21.04.2016 15:35:57
 * Author : dark
 */ 

//------------- DEF -------------------------

#define F_CPU 1000000UL
#define _NOP() do { __asm__ __volatile__ ("nop"); } while (0)
#define HI(x) ((x)>>8)
#define LO(x) ((x)& 0xFF)

#define IND_PORT	PORTB
#define IND_DDR		DDRB
#define IND_SCK		7
#define IND_DO		6
#define IND_DI		5
#define IND_ST		4

#define OUT_PORT	PORTA
#define OUT_DDR		DDRA
#define WIRE		0
#define GAS			1

#define KEY_CLICK_F 0
#define KEY_PRESS_F 1
#define ENC_INC_F	2
#define ENC_DEC_F	3
#define KEYS_PORT	PORTD
#define KEYS_PIN	PIND
#define KEYS_DDR	DDRD
#define BUTTON		2
#define KEY			3
#define ENC1		4
#define ENC2		5
#define ENC_MASK	(1<<ENC2 | 1<<ENC1)
#define ENC_S0		(1<<ENC2 | 1<<ENC1)
#define ENC_S1		(1<<ENC2 | 0<<ENC1)
#define ENC_S2		(0<<ENC2 | 0<<ENC1)
#define ENC_S3		(0<<ENC2 | 1<<ENC1)

//------------- INC -------------------------

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

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

#define SYM_EMPTY 0
#define SYM_G	1
#define SYM_A	2
#define SYM_S	3
#define SYM_P	4
#define SYM_R	5
#define SYM_O	6
#define SYM_V	7
#define SYM_F1	8
#define SYM_F2	9
#define SYM_F3	10
#define SYM_R1	11
#define SYM_R2	12
#define SYM_R3	13
#define SYM_R4	14
#define SYM_H	15
#define SYM_E	16
#define SYM_L	17


const int8_t symbols[] = {
    0b0000000, //[empty]
    0b1000110, //G
    0b1110111, //A
    0b1111001, //S
    0b1110110, //P
    0b1100111, //R
    0b1111110, //O
    0b1111111, //V
    0b1000000, //F1
    0b0000001, //F2
    0b0001000, //F3
    0b0101011, //R1
    0b1001011, //R2
    0b1101010, //R3
    0b1101001, //R4
    0b0110111, //H
    0b1001111, //E
    0b0001110, //L
};

#define  NUM_OF_DIGITS 4

#define NEXT_DIGIT_FLAG 0
#define KEYSCAN_FLAG 1

volatile uint8_t ActionFlags = 0;
volatile uint8_t Displayed[NUM_OF_DIGITS];

// DEBUG
int8_t Value1 = 0;
int8_t Value2 = 0;

// ============== Interrupts =============

ISR(TIMER0_COMPA_vect) {
    ActionFlags |= 1<<KEYSCAN_FLAG;
    ActionFlags |= 1<<NEXT_DIGIT_FLAG;
}

// ============== Init ===============
void init(void) {
    KEYS_PORT |= 1<<ENC1 | 1<<ENC2 | 1<<KEY | 1<<BUTTON; // Pull-Up Resistors
	
	OUT_PORT |= 1<<WIRE | 1<<GAS; // Out
	OUT_DDR |= 1<<WIRE | 1<<GAS; // Direction  - Out
    
    IND_PORT = 1<<IND_DI; // Pull-Up
    IND_DDR  = 1<<IND_SCK | 1<<IND_DO | 1<<IND_ST; // Direction  - Out
    
    TCCR0A = 0b10 << WGM00;
    TCCR0B = 0 << WGM02 | 0b010 << CS00; // 001 - 1; 010 - 8; 011 - 64; 100 - 256; 101 - 1024
    OCR0A = 124;
    TIMSK = 1 << OCIE0A;
    
    sei();
}

void SPIOut(uint8_t a) {
    USIDR = a;
    USISR |= 1<<USIOIF;
    while (~USISR & 1<<USIOIF) {
        USICR = 0b01<<USIWM0 | 0b10<<USICS0 | 1<<USICLK | 1<<USITC;
    }
}

void SendSymbol(uint8_t n, uint8_t a) {
    SPIOut(1<<n);
    SPIOut(~a);

    IND_PORT |= 1<<IND_ST;
    IND_PORT &= ~(1<<IND_ST);
}

uint8_t ReadKeyState() {
    static uint16_t KeyTime = 0;
    static uint8_t EncState = ENC_S0;
    static int8_t EncCnt = 0;
    uint8_t KeyFlags = 0;
    
	// get key status
	if (KEYS_PIN & 1<<KEY) {
    	if (KeyTime > 10 && KeyTime < 1000) {
        	// Click
        	KeyFlags |= 1<<KEY_CLICK_F;
    	}
    	KeyTime = 0;
    	} else {
    	if (KeyTime == 1000) {
        	// Press
        	KeyFlags |= 1<<KEY_PRESS_F;
    	}
    	if (KeyTime < 1000+1) {
        	KeyTime++;
    	}
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

void ModeProcessing(uint8_t KeyFlags) {
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
	
	static uint8_t CurrentMode = MODE_WORK;
	static uint8_t State = STATE_IDLE;
	uint8_t NewMode = MODE_NOCHANGE;

	// Define Action and NewMode
	switch (CurrentMode) {
		case MODE_WORK: {
			// TODO: Show Delay1 & Delay2
			Displayed[3] = digits[Value1>>4  & 0xF];
			Displayed[2] = digits[Value1 & 0xF];
			Displayed[1] = digits[Value2>>4  & 0xF];
			Displayed[0] = digits[Value2 & 0xF];

			if (KeyFlags & 1<<KEY_CLICK_F) NewMode = MODE_SET1;
			if (KeyFlags & 1<<KEY_PRESS_F) NewMode = MODE_BYPASS_WIRE;
			break;
		}
		case MODE_SET1: {
			// TODO: Blink Delay1
			Displayed[3] = digits[Value1>>4  & 0xF];
			Displayed[2] = digits[Value1 & 0xF];
			Displayed[1] = Displayed[0] = symbols[SYM_EMPTY];
					
			if (KeyFlags & 1<<ENC_INC_F) {
				// TODO: Increase Delay1
				Value1++;
			}
			if (KeyFlags & 1<<ENC_DEC_F) {
				// TODO: Decrease Delay1
				Value1--;
			}
			if (KeyFlags & 1<<KEY_CLICK_F) NewMode = MODE_SET2;
			if (KeyFlags & 1<<KEY_PRESS_F) NewMode = MODE_BYPASS_WIRE;
			break;
		}
		case MODE_SET2: {
			// TODO: Blink Delay2
			Displayed[3] = Displayed[2] = symbols[SYM_EMPTY];
			Displayed[1] = digits[Value2>>4  & 0xF];
			Displayed[0] = digits[Value2 & 0xF];
					
			if (KeyFlags & 1<<ENC_INC_F) {
				// TODO: Increase Delay2
				Value2++;
			}
			if (KeyFlags & 1<<ENC_DEC_F) {
				// TODO: Decrease Delay2
				Value2--;
			}
			if (KeyFlags & 1<<KEY_CLICK_F) {
				NewMode = MODE_WORK;
				// TODO: Save Delay1 & Delay2 to EEPROM
			}
			if (KeyFlags & 1<<KEY_PRESS_F) NewMode = MODE_BYPASS_WIRE;
			break;
		}
		case MODE_BYPASS_WIRE: {
			// TODO: Show "WIRE"
			Displayed[3] = symbols[SYM_P];
			Displayed[2] = symbols[SYM_R];
			Displayed[1] = symbols[SYM_O];
			Displayed[0] = symbols[SYM_V];
					
			if (KeyFlags & 1<<KEY_CLICK_F) NewMode = MODE_BYPASS_GAS;
			if (KeyFlags & 1<<KEY_PRESS_F) NewMode = MODE_WORK;
			break;
		}
		case MODE_BYPASS_GAS: {
			// TODO: Show "GAS"
			Displayed[3] = symbols[SYM_G];
			Displayed[2] = symbols[SYM_A];
			Displayed[1] = symbols[SYM_S];
			Displayed[0] = symbols[SYM_EMPTY];
					
			if (KeyFlags & 1<<KEY_CLICK_F) NewMode = MODE_BYPASS_WIRE;
			if (KeyFlags & 1<<KEY_PRESS_F) NewMode = MODE_WORK;
			break;
		}
	}
			
			
	// Switch mode
	if (NewMode != MODE_NOCHANGE) {
		CurrentMode = NewMode;
		State = STATE_IDLE;
	}
	switch (NewMode) {
		case MODE_WORK: {
			break;
		}
		case MODE_SET1: {
			break;
		}
		case MODE_SET2: {
			break;
		}
		case MODE_BYPASS_WIRE: {
			break;
		}
		case MODE_BYPASS_GAS: {
			break;
		}
	}
}

// ============== Main ===============
int main(void) {
    init();
    uint8_t KeyFlags;
    uint8_t CurrentDigit = 0;
	
    
    // DEBUG
    //int8_t ClickCnt = 0;
    //int8_t PressCnt = 0;
    
	// TODO: Read Delay1 & Delay2 from EEPROM
	
    //Displayed[3] = Displayed[2] = Displayed[1] = Displayed[0] = symbols[SYM_EMPTY];
    
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



