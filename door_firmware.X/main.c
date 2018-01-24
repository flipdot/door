
#define _XTAL_FREQ 16000000
#include <xc.h>
#include <stdlib.h>

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection Bits (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = ON        // Watchdog Timer Enable (WDT enabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = ON        // Internal/External Switchover Mode (Internal/External Switchover Mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PPS1WAY = ON     // Peripheral Pin Select one-way control (The PPSLOCK bit cannot be cleared once it is set by software)
#pragma config ZCDDIS = ON      // Zero-cross detect disable (Zero-cross detect circuit is disabled at POR and can be enabled with ZCDSEN bit.)
#pragma config PLLEN = ON       // Phase Lock Loop enable (4x PLL is always enabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LPBOR = OFF      // Low-Power Brown Out Reset (Low-Power BOR is disabled)
#pragma config LVP = ON         // Low-Voltage Programming Enable (Low-voltage programming enabled)

#define SAMPLES 30

#define DOOR_OPEN_IO    LATBbits.LATB6
#define DOOR_OPEN_TRIS  TRISBbits.TRISB6

#define DOOR_CLOSE_IO   LATBbits.LATB7
#define DOOR_CLOSE_TRIS TRISBbits.TRISB7

#define TRIS_OUTPUT     0
#define TRIS_INPUT      1

#define DOOR_SWITCH_DELAY() for(i = 0; i < 200; i++){ \
                                __delay_ms(1);        \
                                CLRWDT();             \
                            }             


void sendChar(char cTx);
void initUart(void);

void interrupt interrupts(void){
    char cRX;
    unsigned char i;

    if(PIR1bits.RCIF){
        cRX = RC1REG;
        if(cRX == '1'){
            DOOR_OPEN_IO = 1;
            DOOR_SWITCH_DELAY();
            DOOR_OPEN_IO = 0;
        }else if(cRX == '0'){
            DOOR_CLOSE_IO = 1;
            DOOR_SWITCH_DELAY();
            DOOR_CLOSE_IO = 0;
        }else{  // echo
            sendChar(cRX);
        }
        PIR1bits.RCIF = 0;          //clear IF
        return;
    }
    while(1);                       // unhandled interrupt -> stop 
}

void sendChar(char cTx){
    TXREG1 = cTx;
    while(!PIR1bits.TXIF);
    PIR1bits.TXIF = 0;
}

void initUart(void){
    CLRWDT();
    RC1STA = 0;
    
    
    NOP();
    TX1STAbits.TXEN = 1;
    NOP();
    TX1STAbits.SYNC = 0;
    NOP();
    RC1STAbits.SPEN = 1;
    NOP();

    RC1STAbits.SREN = 1;
    NOP();
    RC1STAbits.CREN = 1;
    NOP();
    SPBRG1 = 25;
}

void main(void) {
    unsigned short i = 0;
    unsigned short j = 0;
    char cBuff[20];
    char *pBuff;
    unsigned short res;
    unsigned short mean[SAMPLES];
    unsigned long sum;

    OSCCON = 0xFF;


    ANSELC = 0x00;
    GIE = 0;                      // dis int
    PPSLOCK = 0x55;               // magic pps pattern
    PPSLOCK = 0xAA;               // magic pps pattern
    PPSLOCKbits.PPSLOCKED = 0x00; // unlock PPS

    RXPPSbits.RXPPS = 0x0014;     //RC4->EUSART:RX;
    RC5PPSbits.RC5PPS = 0x0014;   //RC5->EUSART:TX;

    PPSLOCK = 0x55;               // magic pps pattern
    PPSLOCK = 0xAA;               // magic pps pattern
    PPSLOCKbits.PPSLOCKED = 0x01; // lock PPS

    initUart();

    PIE1bits.RCIE   = 1;          // en uart rx int
    NOP();
    INTCONbits.PEIE = 1;          // en periphery int
    NOP();
    INTCONbits.GIE  = 1;          // global int enable
    GIE = 1;                      // global int enable

    CLRWDT();


    TRISAbits.TRISA0 = TRIS_INPUT;
    
    ANSELDbits.ANSD0 = 1;         // AN20 - en analog input
    ANSELA  = 0;                  // PORTA all digital

    DOOR_CLOSE_TRIS  = TRIS_OUTPUT; 
    DOOR_CLOSE_IO    = 0;
    DOOR_OPEN_TRIS   = TRIS_OUTPUT;
    DOOR_OPEN_IO     = 0;

    CLRWDT();

    ADCON1 = 0x80;                // left justify adc
    ADCON0bits.ADON = 1;          // enable adc

    sendChar('b');
    sendChar('\n');
    sendChar('\r');

    while(1){
        for(i = 0; i < 10; i++){
            __delay_ms(10);
        }
        CLRWDT();
        ADCON0bits.CHS = 20;      // select A20
        ADCON0bits.GO  = 1;       // start adc conversion
        while(ADCON0bits.GO);     // wait for adc 
        res = ADRES&0x3FF;        // get 10bit result
        mean[j++] = res;          
        if (j >= SAMPLES){
            while(--j){
                sum += mean[j];
            }
            sum /= SAMPLES;
            CLRWDT();
            utoa(cBuff, sum, 10);

            pBuff = cBuff;
            while(*pBuff){
                sendChar(*pBuff++);
            }
            sendChar(' ');
            sendChar('-');
            sendChar(' ');
            sendChar(PORTAbits.RA0 + '0');
            sendChar('\n');
            sendChar('\r');
        }
        
        if(RC1STAbits.OERR == 1){  //hel* stresstesting fixes
            sendChar('r');
            sendChar('\n');
            sendChar('\r');
            initUart();
        }
        CLRWDT();
    }
    return;
}
