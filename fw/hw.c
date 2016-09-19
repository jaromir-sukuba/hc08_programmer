#include <xc.h>
#include "hw.h"

// CONFIG1
#pragma config FOSC = HS        // Oscillator Selection Bits (HS Oscillator, High-speed crystal/resonator connected between OSC1 and OSC2 pins)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable (Brown-out Reset disabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = ON        // Internal/External Switchover Mode (Internal/External Switchover Mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LPBOR = OFF      // Low-Power Brown Out Reset (Low-Power BOR is disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)

unsigned int tst;
void hw_init (void)
{
TRISB6 = 0;
TRISC7 = 0;
SSP1CON1 = 0x20;
SSP1STAT = 0x40;
MCS_T = 0;
MCS = 1;
ANSELA = 0x00;
ANSELB = 0x00;
ANSELC  = 0x00;
LED_T = 0;
TXSTA = 0x24;
RCSTA = 0x90;
BRG16 = 1;
SPBRGL = (_XTAL_FREQ/BAUD_RATE/4) - 1;
}


unsigned int get_vdd (void)
{
unsigned long res,res2;
GO_nDONE = 1;
while (GO_nDONE==1);
res2 = (((unsigned long)(ADRESH))<<8) | ADRESL;
res = 1023;
res = res * 1024;
res = res / res2;
return res;
}


//**********************************************************

unsigned char hwspi_byte(unsigned char data)
{
//led(1);
SSP1IF=0;
SSPBUF = data;
while (SSP1IF==0);
//led(0);
return SSPBUF;
}

void mem_start_write (unsigned int addr)
{
MCS = 0;
hwspi_byte(0x02);
hwspi_byte(addr>>8);
hwspi_byte(addr>>0);
}

void mem_write_byte (unsigned char data)
{
hwspi_byte(data);
}

void mem_end_write (void)
{
MCS = 1;
}

void mem_read_range (unsigned int addr, unsigned char * buf, unsigned char len)
{
unsigned char i;
MCS = 0;
hwspi_byte(0x03);
hwspi_byte(addr>>8);
hwspi_byte(addr>>0);
for (i=0;i<len;i++)
	buf[i] = hwspi_byte(0x00);
MCS = 1;
}

//**********************************************************

unsigned char rx_ready (void)
{
if (RCIF)
	return 1;
else
	return 0;
}

unsigned char rx_get (void)
{
return RCREG;
}

void tx_str (char * data)
{
while (*data!=0) tx_data(*data++);
}

void tx_data (unsigned char dat)
{
while (TRMT==0);
TXREG = dat;
}




void dly_ms(unsigned int ms)
{
unsigned int i;
for (i=0;i<ms;i++) __delay_ms(1);
}

void led (unsigned char data)
{
if (data==0)
	LED = 0;
else
	LED = 1;
}


