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

#define IND_PORT PORTB
#define IND_DDR DDRB
#define IND_SCK 7
#define IND_DO 6
#define IND_DI 5
#define IND_ST 4
#define IND_CLR 3
#define IND_STROBE 2
#define LED1 1
#define LED2 0

#define KEYS_PORT PORTD
#define KEYS_PIN PIND
#define KEYS_DDR DDRD
#define ENC1 4
#define ENC2 5
#define ENC_MASK (1<<ENC2 | 1<<ENC1)
#define ENC_S0 (1<<ENC2 | 1<<ENC1)
#define ENC_S1 (1<<ENC2 | 0<<ENC1)
#define ENC_S2 (0<<ENC2 | 0<<ENC1)
#define ENC_S3 (0<<ENC2 | 1<<ENC1)

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
#define SYM_G 1
#define SYM_A 2
#define SYM_S 3
#define SYM_P 4
#define SYM_R 5
#define SYM_O 6
#define SYM_F1 7
#define SYM_F2 8
#define SYM_F3 9
#define SYM_R1 10
#define SYM_R2 11
#define SYM_R3 12
#define SYM_R4 13
#define SYM_H 14
#define SYM_E 15
#define SYM_L 16


const int8_t symbols[] = {
    0b0000000, //[empty]
    0b1000110, //G
    0b1110111, //A
    0b1111001, //S
    0b1110110, //P
    0b1100111, //R
    0b1111110, //O
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

#define DIGIT_UPD_MS 3
#define  NUM_OF_DIGITS 4

#define NEXT_DIGIT_FLAG 0
#define KEYSCAN_FLAG 1

volatile uint8_t ActionFlags = 0;
volatile uint8_t Displayed[NUM_OF_DIGITS];

// ============== Interrupts =============

ISR(TIMER0_COMPA_vect) {
    static uint8_t NextDigitCnt = DIGIT_UPD_MS;
    ActionFlags |= 1<<KEYSCAN_FLAG;
	NextDigitCnt--;
    if(NextDigitCnt == 0) {
        NextDigitCnt = DIGIT_UPD_MS;
        ActionFlags |= 1<<NEXT_DIGIT_FLAG;
    }
}

// ============== Init ===============
void init(void) {
    KEYS_PORT |= 1<<ENC1 | 1<<ENC2;
    
    IND_PORT = 1<<IND_DI;
    IND_DDR  = 1<<IND_SCK | 1<<IND_DO | 1<<IND_ST | 1<<IND_CLR | 1<<IND_STROBE | 1<<LED1 | 1<<LED2;
    IND_PORT |= 1<<IND_CLR;
    
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

// ============== Main ===============
int main(void) {
    uint8_t CurrentDigit = 0;
    int8_t EncCnt = 0;
    uint8_t EncState = ENC_S0;
    int8_t Value = 0;
    
    init();
    
    Displayed[3] = symbols[SYM_EMPTY];
    Displayed[2] = symbols[SYM_EMPTY];
    Displayed[1] = symbols[SYM_EMPTY];
    Displayed[0] = symbols[SYM_EMPTY];
    
    while (1) {
        uint8_t EncNew;
        // Get current status
        
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
        
        Displayed[3] = digits[EncCnt + 5];
        
        if (EncState == ENC_S0) {
            if (EncCnt > 0) Value++;
            if (EncCnt < 0) Value--;
            EncCnt = 0;
        }            
        
        Displayed[0] = digits[Value & 0xF];
        Displayed[1] = digits[Value>>4  & 0xF];
		
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



