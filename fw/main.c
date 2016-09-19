/* 
 * File:   main.c
 * Author: jsukuba
 *
 * Created on Piatok, 2015, júl 3, 11:59
 */


//VERY VERY PRELIMINARY VERSION, RIGHT NOW TAILORED FOR KX2 AND NOTHING ELSE


#include <xc.h>
#include <stdio.h>
#include "hw.h"

#define ERARRNG_MASS_ERASE 0x40


#define	H_WAIT	0
#define	H_SIZE	1
#define	H_ADDR	2
#define	H_TYPE	3
#define	H_DATA	4
#define	H_END	5

#define		tx_s	tx_msg

unsigned char enter_mon_mode (unsigned char * secbytes);
unsigned char mon_read(unsigned int addr);
unsigned char mon_write(unsigned int addr, unsigned char data);
unsigned int mon_read_sp (void);
unsigned char mon_writes(unsigned int addr, unsigned char * data, unsigned char len);
unsigned int mon_run (unsigned int PC, unsigned int A, unsigned int CC, unsigned int HX);
unsigned int mon_call_A (unsigned int mon, unsigned char ctrlbyt, unsigned char accu, unsigned int faddr, unsigned int laddr);
unsigned int mon_call_B (unsigned int mon, unsigned char ctrlbyt, unsigned char accu, unsigned int faddr, unsigned int len);
unsigned char hexa_bin (unsigned char data);
unsigned int parse_char (unsigned char data, unsigned int * lowest);
unsigned int bin_cont (unsigned char b3, unsigned char b2, unsigned char b1, unsigned char b0);
unsigned int mon_run_exit (unsigned int PC, unsigned int A, unsigned int CC, unsigned int HX);

unsigned char prog_flash (unsigned int addr, unsigned char len, unsigned char * buffer);
unsigned char erase_flash (void);
unsigned char enter_mon (void);

extern unsigned char swu_tx(unsigned char val);
extern unsigned char swu_rx(unsigned char val);
volatile char tmp;
unsigned char swu_cnt@0x40,swu_dat@0x41,swu_tmp@0x42,swu_ret@0x43,swu_flg@0x44;

unsigned char tx_msg[30],secbytes[8],mem_arr[64];
unsigned char g1,g2;

unsigned int CPUSPEED;	// 2 x Fbus freq, e.g.  ext osc 16 MHz -> Fbus == 4 Mh => CPUSPEED==2
unsigned int RAM;
unsigned int FLASH;	// Flash start address
unsigned int FLASH_END;	// Flash end address
unsigned int VECTORS;	// Vectors address
unsigned int PUTBYTE;	// Receive byte routine address
unsigned int GETBYTE;	// Receive byte routine address
unsigned int RDVRRNG;	// Read/verify flash routine address
unsigned int ERARRNG;	// Erase flash routine address
unsigned int PRGRNGE;	// Flash programming routine address
unsigned int FLBPR;	// Flash block proctection register address
unsigned int MONRTN;	// Monitor mode return jump address
unsigned int EADDR;	// For FLBPR in Flash the mass erase must use FLBPR as the erase address
unsigned int MONTYPE;	// 0 - type A, 1 - type B instance, difference in ROM routines calling, see AN2874
// These will be calculated
unsigned int MONDATA;	// Flashing routines parameter block address (==RAM+8)
unsigned int CTRLBYT;	// Address of flashing routine control variable (==MONDATA+0)
unsigned int CPUSPD;	// Address of flashing routine cpu speed variable (==MONDATA+1)
unsigned int LADDR;	// Address of flashing routine last address variable (==MONDATA+2)
unsigned int DATABUF;	// Flashing routines data buffer address (==MONDATA+4)
unsigned int PAGESIZE;	// Databuffer size
unsigned int WORKRAM;	// Work storage needed for calling flashing routines
unsigned int WORKTOP;	// Topmost work storage address

unsigned char state_main,rx_b;
unsigned int addr,addr_highest,addr_lowest,parser_retval,addr_max,addr_min,read_len;

unsigned char h_len, h_type, h_data[5];
unsigned int	h_addr;
unsigned char h_array[64];
char h_temp[10];
unsigned char h_state, h_substate, h_data_c,h_array_p;
unsigned char fl_empty;


unsigned int i,sp,addr,len,n;
int main(void)
	{
	TRISCbits.TRISC1 = 0;
	LED_T = 0;
	dly_ms(5);
	
	hw_init();

	tx_str((char *)("HC08 programmer\n"));

	FLASH=0xF600;
	FLASH_END=0xFDFF;
	VECTORS = 0xFFDC;
	PUTBYTE=0x0000; 
	GETBYTE=0x1000;
	RDVRRNG=0x1003;
	ERARRNG=0x1006;
	PRGRNGE=0x1009;
	MONRTN=0x0000;  
	FLBPR=0xFF7E;
	EADDR=FLASH;     
	RAM = 0x40;
	MONTYPE = 0;
	PAGESIZE = 64;
	
	
	
	
	MONDATA = RAM+8;
	CTRLBYT  =  MONDATA+0;
	CPUSPD  =  MONDATA+1;
	LADDR  =  MONDATA+2;
	DATABUF  =  MONDATA+4;
	WORKRAM = DATABUF + PAGESIZE;
	WORKTOP = 0xF0; // this leaves 16 bytes for stack, the deepest ROM routines use 11 so there some for myself too

	
	
	addr_highest = 2;
	addr_lowest=0xFFFF;
	
	while (1)
		{
		if (rx_ready())
			{
			rx_b =  rx_get();
			if ((rx_b>='a')|(rx_b>='z'))
				{
				if (rx_b=='b')
					state_main=1;				
				if (rx_b=='c')
					state_main=2;				
				if (rx_b=='p')
					state_main=3;		
				if (rx_b=='f')
					state_main=4;		
				if (rx_b=='v')
					state_main=5;		
				if (rx_b=='e')
					state_main=6;	
				/*
				if (rx_b=='t')
					state_main=7;		
				 */
				if (rx_b=='i')
					state_main=8;		
				if (rx_b=='r')
					state_main=9;		
				if (rx_b=='x')
					state_main=10;		

				}
			else
				{
				parser_retval = parse_char(rx_b,&addr_min);
				if (parser_retval>0)
					{
					addr_max = parser_retval;
					sprintf (tx_msg,"hex received, %X %X\n\n",addr_max,addr_min);
					tx_str (tx_msg);
					}
				}
			}
		if (state_main==1)
			{
			sprintf (tx_s,"reading %d bytes from buffer:\n",addr_max);
			tx_str (tx_s);
			for (addr=0xF600;addr<0xF700;addr=addr+1)
				{
				if ((addr%16)==0) 
					{
					sprintf(tx_s,"\n%4.4X: ",addr);
					tx_str (tx_s);
					}
				mem_read_range(addr,mem_arr,1);
				sprintf(tx_s,"%2.2X ",mem_arr[0]);
				tx_str (tx_s);
				}
			tx_str((char *)("\n\n"));
			state_main=0;
			}
		if (state_main==2)
			{
			tx_str ("clearing buffer:\n");
			mem_start_write(FLASH);
			for (addr=FLASH;addr<0xFFFF;addr=addr+1)
				{
				mem_write_byte(0xFF);
				if ((addr%2048)==0) 
					tx_str ("#");
				}
			mem_end_write();
			tx_str((char *)("\nOK\n\n"));
			state_main=0;
			}
		
		if (state_main==3)
			{
			enter_mon();
			tx_str ("erasing:\n");
			erase_flash();

			sprintf (tx_s,"programming FLASH:\n",addr_max);
			tx_str (tx_s);
			for (addr=FLASH;addr<FLASH_END;addr=addr+PAGESIZE)
				{
				fl_empty = 1;
				mem_read_range(addr,mem_arr,PAGESIZE);
				for (i=0;i<PAGESIZE;i++)
					if (mem_arr[i]!=0xFF) fl_empty = 0;
				if (fl_empty == 1)
					{
//					sprintf (tx_s,"%X at %X empty\n",PAGESIZE,addr);
//					tx_str (tx_s);
					}
				else
					{
					sprintf (tx_s,"%X at %X\n",PAGESIZE,addr);
					tx_str (tx_s);
					prog_flash(addr,PAGESIZE,mem_arr);
					}
				
				}
			tx_str ("programming vectors:\n");
			addr = VECTORS;
			while (addr<(0-PAGESIZE))
				{
				sprintf (tx_s,"%X at %X\n",PAGESIZE,addr);
				tx_str (tx_s);
				mem_read_range(addr,mem_arr,PAGESIZE);
				prog_flash(addr,PAGESIZE,mem_arr);
				addr = addr + PAGESIZE;
				}
			if (addr!=0xFFFF)
				{
				len = 0-addr;
				sprintf (tx_s,"%X at %X\n",len,addr);
				tx_str (tx_s);
				mem_read_range(addr,mem_arr,len);
				prog_flash(addr,len,mem_arr);
				}
			mem_read_range(0xFFFE,mem_arr,2);
			addr = mem_arr[0];
			addr = addr<<8;
			addr = addr + mem_arr[1];
			sprintf (tx_s,"running from %X\n",addr);
			tx_str (tx_s);
			state_main=0;
			mon_run_exit(addr,0,0,0);
			
			tx_str((char *)("\nOK\n\n"));
			state_main=0;
			}
		if (state_main==4)
			{
			enter_mon();
			tx_str ("reading from FLASH:\n");
			for (addr=0xF600;addr<0xF680;addr=addr+1)
				{
				if ((addr%16)==0) 
					{
					sprintf(tx_s,"\n%4.4X: ",addr);
					tx_str (tx_s);
					}
				g1 = mon_read(addr);
				sprintf (tx_msg," %2.2X",g1);
				tx_str((char *)(tx_msg));
				}
			tx_str((char *)("\n\n"));
			state_main=0;
			}
		
		if (state_main==5)
			{
			enter_mon();
			tx_str ("reading vectors:\n");
			addr=VECTORS-1;
			if (((addr+1)%8)!=0)
				{
				sprintf(tx_s,"\n%4.4X: ",addr);
				tx_str (tx_s);
				}	
			for (addr=VECTORS-1;addr<0xFFFF;addr=addr+1)
				{
				if (((addr+1)%8)==0) 
					{
					sprintf(tx_s,"\n%4.4X: ",addr+1);
					tx_str (tx_s);
					}
				g1 = mon_read(addr+1);
				sprintf (tx_msg," %2.2X",g1);
				tx_str((char *)(tx_msg));
				}
			tx_str((char *)("\n\n"));
			state_main=0;
			}		

		if (state_main==6)
			{
			enter_mon();
			tx_str ("erasing FLASH:\n");
			erase_flash();
			sprintf (tx_s,"OK:\n");
			tx_str (tx_s);
			state_main=0;
			}
		
		if (state_main==7)
			{	
			enter_mon();
			tx_str ("programming TEST FLASH:\n");
			for (i=0;i<PAGESIZE;i++)
				mem_arr[i] = i;
			prog_flash(FLASH,PAGESIZE,mem_arr);
			tx_str ("OK:\n");
			state_main=0;
			}

		if (state_main==8)
			{	
			sprintf (tx_s,"INFO:\n");
			tx_str (tx_s);
	
			sprintf (tx_s,"FLASH 0x%4.4X RAM 0x%4.4X\n",FLASH,RAM);
			tx_str (tx_s);
			sprintf (tx_s,"DB 0x%4.4X MONDAT 0x%4.4X\n",DATABUF,MONDATA);
			tx_str (tx_s);

			state_main=0;
			}

		if (state_main==9)
			{
			enter_mon();
			sprintf (tx_s,"reading from RAM:\n");
			tx_str (tx_s);
			for (addr=0x40;addr<0x100;addr=addr+1)
				{
				if ((addr%16)==0) 
					{
					sprintf(tx_s,"\n%4.4X: ",addr);
					tx_str (tx_s);
					}
				g1 = mon_read(addr);
				sprintf (tx_msg," %2.2X",g1);
				tx_str((char *)(tx_msg));
				}
			tx_str((char *)("\n\n"));
			state_main=0;
			}
		
		if (state_main==10)
			{
			tx_str ("entering prog mode:\n");
			enter_mon();
			mem_read_range(0xFFFE,mem_arr,2);
			addr = mem_arr[0];
			addr = addr<<8;
			addr = addr + mem_arr[1];
			sprintf (tx_s,"running from %X\n",addr);
			tx_str (tx_s);
			state_main=0;
			mon_run_exit(addr,0,0,0);
			
			tx_str((char *)("\nOK\n\n"));
			state_main=0;
			}
		}	
	}


unsigned char enter_mon (void)
	{
//	led(1);
	tx_str ("entering prog mode:\n");
	LATCbits.LATC1 = 1;
	dly_ms(1);
	LATCbits.LATC1 = 0;
	for (i=0;i<8;i++)
		secbytes[i] = 0xFF;
	enter_mon_mode(secbytes);
	sp = mon_read_sp();
	g1 = mon_read(0x0040);
	sp = mon_read_sp();
	if (g1&0x40)
		{
		sprintf (tx_s,"Entered, SP 0x%4.4X:\n",sp);
		tx_str (tx_s);
		}
//	led(0);
	}

unsigned char erase_flash (void)
	{
//	mon_call_A(ERARRNG , ERARRNG_MASS_ERASE, 0, EADDR, 0);
	if (MONTYPE == 0)
		mon_call_A(ERARRNG , ERARRNG_MASS_ERASE, 0, EADDR, 0);
	if (MONTYPE == 1)
		mon_call_B(ERARRNG , 0, 0, 0xFFFF, 0);
	}

unsigned char prog_flash (unsigned int addr, unsigned char len, unsigned char * buffer)
	{
	mon_writes(DATABUF,buffer,len);
//	mon_call_A(PRGRNGE, 0 , 0, addr , addr+len-1);
	if (MONTYPE == 0)
		mon_call_A(PRGRNGE, 0 , 0, addr , addr+len-1);
	if (MONTYPE == 1)
		mon_call_B(PRGRNGE, 0 , 0, addr , len);
	}


/*
mass erase - callMonitor(ERARRNG , ERARRNG_MASS_ERASE, 0, EADDR, 0);
program - callMonitor(PRGRNGE, 0 , 0, addr , addr+n-1);
read at verify - callMonitor(RDVRRNG, 0 , 1 , addr , addr+n-1);
page erase - callMonitor(ERARRNG, ERARRNG_PAGE_ERASE, 0, a, 0);
*/

unsigned int mon_call_A (unsigned int mon, unsigned char ctrlbyt, unsigned char accu, unsigned int faddr, unsigned int laddr)
	{
	mem_arr[0] = ctrlbyt;
	mem_arr[1] = 4;
	mem_arr[2] = laddr>>8;
	mem_arr[3] = laddr&0xFF;
	mon_writes(MONDATA,mem_arr,4);

	mem_arr[0] = 0xCD;
	mem_arr[1] = mon>>8;
	mem_arr[2] = mon&0xFF;
	mem_arr[3] = 0x83;
	mon_writes(WORKRAM,mem_arr,4);
	
	mon_run(WORKRAM,accu,0x00,faddr);
	}

/*
int cca=callMonitor_B(RDVRRNG, 0 , 1 , addr , n);
callMonitor_B(PRGRNGE, 0 , 0, addr , n);
callMonitor_B(ERARRNG , 0, 0, 0xFFFF, 0);
	MONDATA = RAM+8;
	CTRLBYT  =  MONDATA+0;
	CPUSPD  =  MONDATA+1;
	LADDR  =  MONDATA+2;
	DATABUF  =  MONDATA+4;
	WORKRAM = DATABUF + PAGESIZE;
*/

unsigned int mon_call_B (unsigned int mon, unsigned char ctrlbyt, unsigned char accu, unsigned int faddr, unsigned int len)
	{
	mem_arr[0] = 4;
	mem_arr[1] = len;
	mem_arr[2] = faddr>>8;
	mem_arr[3] = faddr&0xFF;
	mon_writes(MONDATA,mem_arr,4);

	mem_arr[0] = 0x45;				//LDHX mondata
	mem_arr[1] = MONDATA>>8;
	mem_arr[2] = MONDATA&0xFF;
	mem_arr[3] = 0xCD;				//jsr mon
	mem_arr[4] = mon>>8;
	mem_arr[5] = mon&0xFF;
	mem_arr[6] = 0x83;				//swi
	mon_writes(WORKRAM,mem_arr,7);
	
	mon_run(WORKRAM,accu,0x00,faddr);
	}



unsigned int mon_run (unsigned int PC, unsigned int A, unsigned int CC, unsigned int HX)
	{
	unsigned char mem_fragment[10];
	sp = mon_read_sp();
	mem_fragment[0]=HX>>8;
	mem_fragment[1]=CC;
	mem_fragment[2]=A;
	mem_fragment[3]=HX&0xFF;
	mem_fragment[4]=PC>>8;
	mem_fragment[5]=PC&0xFF;
	mon_writes(sp+1,mem_fragment,6);
	swu_tx(0x28);
	swu_rx(0);	
	swu_rx(0);	
	}

unsigned int mon_run_exit (unsigned int PC, unsigned int A, unsigned int CC, unsigned int HX)
	{
	unsigned char mem_fragment[10];
	sp = mon_read_sp();
	mem_fragment[0]=HX>>8;
	mem_fragment[1]=CC;
	mem_fragment[2]=A;
	mem_fragment[3]=HX&0xFF;
	mem_fragment[4]=PC>>8;
	mem_fragment[5]=PC&0xFF;
	mon_writes(sp+1,mem_fragment,6);
	swu_tx(0x28);
	swu_rx(0);	
	}

unsigned char mon_read(unsigned int addri)
	{
	unsigned char d1,d2;
	
	__delay_us(500);
	swu_tx(0x4A);
	d1 = swu_rx(0);	

	__delay_us(100);
	swu_tx(addri>>8);
	d1 = swu_rx(0);	

	d2 = addri&0xFF;
	
	__delay_us(100);
	swu_tx(d2);
	d1 = swu_rx(0);	
	
	d1 = swu_rx(0);	
	return d1;
	}

unsigned char mon_write(unsigned int addr, unsigned char data)
	{
	unsigned char d1,d2;
	
	
	__delay_us(100);
	swu_tx(0x49);
	d1 = swu_rx(0);	
	
	__delay_us(100);
	swu_tx(addr>>8);
	d1 = swu_rx(0);	
	
	__delay_us(100);
	swu_tx(addr>>0);
	d1 = swu_rx(0);	
	
	__delay_us(100);
	swu_tx(data);
	d1 = swu_rx(0);	
	
	dly_ms(2);
	
	return d1;
	}


unsigned char mon_writes(unsigned int addr, unsigned char * data, unsigned char len)
	{
	unsigned char d1,d2;
	
	
	__delay_us(100);
	swu_tx(0x49);
	d1 = swu_rx(0);	
	
	__delay_us(100);
	swu_tx(addr>>8);
	d1 = swu_rx(0);	
	
	__delay_us(100);
	swu_tx(addr>>0);
	d1 = swu_rx(0);	

	__delay_us(100);
	swu_tx(*data++);
	d1 = swu_rx(0);	
	len--;
	dly_ms(2);
	
	while (len>0)
		{
		__delay_us(100);
		swu_tx(0x19);
		d1 = swu_rx(0);	

		__delay_us(100);
		swu_tx(*data++);
		d1 = swu_rx(0);	
		len--;
		
		dly_ms(2);
		}
//	dly_ms(2);	
	return d1;
	}




unsigned int mon_read_sp (void)
	{
	unsigned char d1,d2;
	unsigned int rv;
	__delay_us(200);
	swu_tx(0x0C);
	d1 = swu_rx(0);	
	d1 = swu_rx(0);	
	d2 = swu_rx(0);	
	rv = d1;
	rv = rv<<8;
	rv = rv| d2;
	return (rv-1);
	
	}


unsigned char enter_mon_mode (unsigned char * secbytes)
	{
	unsigned char i,rv;
	unsigned char d1,d2;
	rv = 0;
	for (i=0;i<8;i++)
		{
//		sprintf (tx_msg,"B %d ",i);
//		tx_str((char *)(tx_msg));
		swu_tx(secbytes[i]);
		swu_flg =0;
		d1 = swu_rx(0);
		if (i==7)
			{
			swu_flg =0;
			d2 = swu_rx(0);
			}
//		sprintf (tx_msg,"R %x \n",d1);
//		tx_str((char *)(tx_msg));
		}
	if (swu_flg!=0)
		{
		sprintf (tx_msg,"BRK %x \n",d2);
		rv = 1;
		}
	else
		{
		sprintf (tx_msg,"--- %x \n",d2);
		}	
	tx_str((char *)(tx_msg));
	return rv;
	}




/*
 	while (1)
		{
		swu_ret = swu_rx(0);
		tmp++;
		}
	
	while (1)
		{
		swu_tx('A');
		dly_ms(100);
		}

 
 
 */


unsigned int parse_char (unsigned char data, unsigned int * lowest)
{
unsigned int t1,t2;
unsigned int retval;
retval = 0;
if (data=='S')
	{
	h_state = H_TYPE;
	h_substate = 0;
	#ifdef	IHX_PARSER_DEBUG_MSG
	tx_data(0x0D);
	tx_data('B');
	#endif
	mem_end_write();
	return retval;
	}

if (h_state==H_TYPE)
	{
	h_array_p=0;
	#ifdef	IHX_PARSER_DEBUG_MSG
	tx_data('T');
	#endif
	if (data == '9')
		{
		h_state = H_END;	
		h_substate = 0;
		#ifdef	IHX_PARSER_DEBUG_MSG
		tx_data('~');
		#endif
		mem_end_write();
		retval = 0;
		}
	else if (data == '1')
		{
		h_state = H_SIZE;	
		h_substate = 0;
		}
	else
		{
		h_state = H_WAIT;
		}
	return retval;
	}


if (h_state==H_SIZE)
	{
	h_data[h_substate++] = hexa_bin(data);
	if (h_substate==2)
		{
		h_state = H_ADDR;
		h_substate = 0;
		h_len = bin_cont(h_data[1],h_data[0],0,0) - 3;
		#ifdef	IHX_PARSER_DEBUG_MSG
		tx_data('S');
		#endif
		#ifndef	IHX_PARSER_DEBUG_MSG
		tx_data('@');
		#endif
		}
	return retval;
	}

if (h_state==H_ADDR)
	{
	h_data[h_substate++] = hexa_bin(data);
	if (h_substate==4)
		{
		h_state = H_DATA;
		h_substate = 0;
		h_addr = bin_cont(h_data[3],h_data[2],h_data[1],h_data[0]);
		#ifdef	IHX_PARSER_DEBUG_MSG
		tx_data('A');
		#endif
		mem_start_write(h_addr);
		if (addr_lowest>(h_addr)) 
			{
			if (addr_lowest!=0) 
				addr_lowest=(h_addr);
			}
		}

	return retval;
	}


if (h_state==H_DATA)
	{
	h_data[h_substate++] = hexa_bin(data);
	if (h_substate==2)
		{
		h_data_c = bin_cont(h_data[1],h_data[0],0,0);
		h_array[h_array_p++] = h_data_c;
		h_substate = 0;
		#ifdef	IHX_PARSER_DEBUG_MSG
		tx_data('D');
		#endif
		mem_write_byte(h_data_c);
		if (h_array_p==h_len)
			{
			if (addr_highest<(h_addr+h_len-1)) 
				addr_highest=(h_addr+h_len-1);
			//h_len, h_addr, h_array
//			for (d2=0;d2<h_len;d2++) 
//				mem_arr[h_addr + d2] = h_array[d2];
			h_state = H_WAIT;
			}
		else
			{

			}
		}
	return retval;
	}
if (h_state==H_END)
	{
	h_substate++;
	if ((h_substate==8)|(data<' '))
		{
		#ifdef	IHX_PARSER_DEBUG_MSG
		tx_data('!');
		tx_data('e');
		tx_data('n');
		tx_data('d');
		tx_data(0x0D);
		#endif
		retval = addr_highest;
		addr_highest = 2;
		*lowest = addr_lowest;
		addr_lowest=0xFFFF;
		h_state = H_WAIT;
		}
	return retval;
	}
return retval;
}


unsigned char hexa_bin (unsigned char data)
{
if ((data>='0')&(data<='9'))
	return data - '0';
if ((data>='A')&(data<='F'))
	return data - 'A' + 10;
else
	return 0;

}

unsigned int bin_cont (unsigned char b3, unsigned char b2, unsigned char b1, unsigned char b0)
{
unsigned int t1;
t1 = b0;
t1 = t1<<4;
t1 = t1 | b1;
t1 = t1<<4;
t1 = t1 | b2;
t1 = t1<<4;
t1 = t1 | b3;
return t1;
}



