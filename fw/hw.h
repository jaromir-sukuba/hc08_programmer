#define	LED		LATC0
#define	LED_T	TRISC0

//#define IHX_PARSER_DEBUG_MSG 1

#define	MCS		LATC6
#define	MCS_T	TRISC6

#define	_XTAL_FREQ	(7372800*1)
#define	BAUD_RATE	115200UL

void dly_ms(unsigned int ms);
unsigned char spi_byte (unsigned char dat);
void enter_progmode_pins (unsigned char mode, unsigned char rst_log);
void exit_progmode_pins (unsigned char rst_log);
void tx_data (unsigned char dat);
void tx_str (char * data);
unsigned char rx_ready (void);
unsigned char rx_get (void);
unsigned char hwspi_byte(unsigned char data);
void hw_init (void);
void led (unsigned char data);
unsigned int get_vdd (void);
void set_cs (unsigned char data);
void mem_start_write (unsigned int addr);
void mem_write_byte (unsigned char data);
void mem_end_write (void);
void mem_read_range (unsigned int addr, unsigned char * buf, unsigned char len);




