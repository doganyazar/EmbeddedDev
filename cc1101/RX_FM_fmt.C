

#include <16f648a.h>


          /* >>> --  <<< */
          /* >>> Prototyper  <<< */

void send_uart (unsigned char TxByte);
void cc_strobe_DBG (unsigned char reg_adress);
void cc_reset (void);
void cc_init_fm (void);
void cc_init_asyn (void);
void cc_strobe (unsigned char reg_adress);
char rx9600(void);
void soft_tx (unsigned char txbyte);
void usart_init_sync_slave (void);
void cc_cal (void);
unsigned char cc_read (unsigned char reg_adress);
void cc_write (unsigned char reg_adress, unsigned char reg_data);
void blink_led (void);
void puthex (unsigned char hexbyte);
unsigned char hex2ascii (unsigned char hex);
void delayx500us (char delay);
void delay10x (unsigned char multiplikator);
void read_ee (unsigned char adress);
void write_ee (unsigned char adress);


          /* >>> --  <<< */
          /* >>> Globala variabler  <<< */
          /* >>> DEFINES  <<< */
          /* >>> CC1101  <<< */

          /* >>> REGISTERS (rd/wr)  <<< */
#define IOCFG2                  0x00
#define IOCFG1                  0x01
#define IOCFG0                  0x02

#define FIFOTHR                 0x03

#define SYNC1                   0x04
#define SYNC0                   0x05

#define PKTLEN                  0x06
#define PKTCTRL1                0x07
#define PKTCTRL0                0x08

#define ADDR                    0x09
#define CHANNRL                 0x0a
#define FSCTRL1                 0x0b
#define FSCTRL0                 0x0c

#define FREQ2                   0x0d
#define FREQ1                   0x0e
#define FREQ0                   0x0f

#define MDMCFG4                 0x10
#define MDMCFG3                 0x11
#define MDMCFG2                 0x12
#define MDMCFG1                 0x13
#define MDMCFG0                 0x14

#define DEVIATN                 0x15
#define MCSM2                   0x16
#define MCSM1                   0x17
#define MCSM0                   0x18

#define FOCCFG                  0x19
#define BSCFG                   0x1a

#define AGCTRL2                 0x1b
#define AGCTRL1                 0x1c
#define AGCTRL0                 0x1d

#define WOREVT1                 0x1e
#define WOREVT0                 0x1f
#define WORCTRL                 0x20

#define FREND1                  0x21
#define FREND0                  0x22

#define FSCAL3                  0x23
#define FSCAL2                  0x24
#define FSCAL1                  0x25
#define FSCAL0                  0x26

#define RCCTRL1                 0x27
#define RCCTRL0                 0x28

#define FSTEST                  0x29
#define PTEST                   0x2a
#define AGCTEST                 0x2b

#define TEST2                   0x2c
#define TEST1                   0x2d
#define TEST0                   0x2e
          /* >>> -----  <<< */
          /* >>> COMMAND STROBES (wr)  <<< */
#define BURST_WR                0x40       //offset + for burst mode
                                           //only for PATABLE & TXFIFO

#define SRES                    0x30
#define SFSTXON                 0x31
#define SXOFF                   0x32
#define SCAL                    0x33
#define SRX                     0x34
#define STX                     0x35
#define SIDLE                   0x36
#define SWOR                    0x37
#define SAFC                    0x38
#define SPWD                    0x39
#define SFRX                    0x3a
#define SFTX                    0x3b
#define SSWORRST                0x3c
#define SNOP                    0x3d
          /* >>> -----  <<< */
          /* >>> STATUS-REGISTERS (rd) (burts bit has to be "1"  <<< */

#define PARTNUM                 0x70
#define VERSION                 0x71
#define FREQEST                 0x72
#define LQIL                    0x73
#define RSSI                    0x74
#define MARCSTATE               0x75
#define WORTIME1                0x76
#define WORTIME0                0x77
#define PKTSTATUS               0x78
#define VCO_VC_DAC              0x79
#define TXBYTES                 0x7a
#define RXBYTES                 0x7b

          /* >>> -----  <<< */
          /* >>> FIFO (rd/wr)  <<< */

#define PATABLE                 0x3e
#define TXFIFO                  0x3f

#define RXFIFO                  0x3f

          /* >>> -----  <<< */

          /* >>> ----  <<< */
          /* >>> MISC  <<< */


#define HIGH              0     //for cc1100 level trans invert signal
#define LOW               1


#define WD_TIME_24h 63858  // 24h= 86400sek / 1353ms = 63858 => low battery

#define TCXO_ON     0
#define TCXO_OFF    1

#define LED_ON       0x00
#define LED_OFF      0x01


#define ETTA  1
#define NOLLA 0

#define FOREVER 1
#define TRUE 1
#define FALSE 0

          /* >>> ----  <<< */
          /* >>> EE  <<< */

// ******** EEPROM MAP **********

#define EE_START_FIRST_TIME 0x10
#define FQ_OFFSET           0x11        //channel number


          /* >>> PORTAR (ON CHIP) cc5x  <<< */
          /* >>> Port A  <<< */

#pragma bit      FQ_INC             = PORTA.0  // Fq tunning
#pragma bit      FQ_DEC             = PORTA.1  //
#pragma bit      SI                 = PORTA.2  //
#pragma bit      CSn                = PORTA.3  //
#pragma bit      AM                 = PORTA.4  // for future FM/AM switch 4 channels
#pragma bit      LED_H              = PORTA.6  //         (OS2 ut)
#pragma bit      LED_V              = PORTA.7  //         (OS1 in)


          /* >>> Port B  <<< */

#pragma bit      RA_RX          = PORTB.0   // Also "Interrupt in"
#pragma bit      SO             = PORTB.1
#pragma bit      SCLK           = PORTB.2
#pragma bit      SERVICE        = PORTB.3
#pragma bit      TX_3           = PORTB.4
#pragma bit      LED_FB         = PORTB.5
#pragma bit      CODE_A1        = PORTB.6 //code1 code2 ON = "1"= code2 (ver 5)
#pragma bit      CODE_A2        = PORTB.7 //FM AM       ON = "1"= AM (ver 5)



          /* >>> VARIABLER  <<< */

unsigned char gtemp, cc_stat;










          /* >>> MAIN   <<< */
void main(void)
{

          /* >>> DEKLARATIONER  <<< */
unsigned char temp, rx_bytes;


          /* >>> POR (Power On Reset)  <<< */

          /* >>> INITIERA PORTAR; HèRDVARA OCH VARIABLER  <<< */
          /* >>> C628 Port   <<< */

TRISA =  0b00010011;                       
TRISB =  0b11001011;
          /* >>> -----  <<< */

OPTION = 0xe; //pre=WD (tocs 0), RBPU=en, PSA=1(WD), PS0..2 110(pre:64)  */
// Prescaler for WD
// Prescaler timer interrupt 50ms

CMCON = 0x07;                      // RA0-3 = digital i/o


          /* >>> UART "ON CHIP" KOMMUNIKATION (9600 8N1)  <<< */
  TXSTA = 0x24;      // UART mode.
  SPBRG = 25;        // 9600    
  RCSTA = 0x90;      // enable RB1, RB2


PORTB = 0xff;        //software uart init "1"
PORTA = 0xf0;        //

CSn = HIGH;
SCLK = HIGH;    
SI = LOW;

clrwdt();            // watch dog bone


 read_ee(EE_START_FIRST_TIME);  //is it POR for the first time?
 if(EEDATA == 0xff){            // set to common fq-tune value
   delay10x(50);
   EEDATA = 118;                //channel offset
   write_ee(FQ_OFFSET);
   EEDATA = 0x00;
   write_ee(EE_START_FIRST_TIME);

 }/*if(EE...*/


   cc_reset();
   cc_init_fm();
   read_ee(FQ_OFFSET);          //fetch channel offset (fq-tune data)
   cc_write(CHANNRL,EEDATA);

   cc_write(IOCFG0,0x07);       //GD0; 07 = CRC OK 
   cc_write(IOCFG1,0x0a);       //SO or GD1; when CSn=1 0x0a= PLL lock
   cc_write(IOCFG2,0x06);       //GD2; SYNK out


   cc_strobe(SRX);                //RX on
   cc_strobe(SIDLE);
   cc_cal();
   cc_strobe(SRX);
   
   
   

while(FOREVER){

          /* >>> MAIN LOOP  <<< */

clrwdt();            // watch dog bone


    delayx500us(2);
    if(RA_RX == 1){             //check for radiotelegram 
                                //it is not good idea to check radiotelegram
                                //becuse this this do some radio-smog noise
                                //to antenna and also internal noise on the
                                // cc1101 chip
      rx_bytes = cc_read(RXBYTES);
      if(rx_bytes == 8){         //packet size
        temp = cc_read(RXFIFO);  //get radio byte 0
        send_uart(temp);

        temp = cc_read(RXFIFO);  //1
        send_uart(temp);
        
        temp = cc_read(RXFIFO);  //2
        send_uart(temp);
        
        temp = cc_read(RXFIFO);  //3
        send_uart(temp);
        
        temp = cc_read(RXFIFO);  //4
        send_uart(temp);                 
        
        temp = cc_read(RXFIFO);  //5
        send_uart(temp);
        
        temp = cc_read(RXFIFO);  //6
        send_uart(temp);
        
        temp = cc_read(RXFIFO);  //7
        send_uart(temp);
        
        temp = cc_read(RXFIFO);  //get radio
        send_uart(temp);


        cc_strobe(SIDLE);
        cc_strobe(SFRX);             //flush rx buffer
        cc_strobe(SRX);
      }/*if(rx_bytes == 8...*/

      else{
        delay10x(2);
        cc_strobe(SIDLE);
        cc_strobe(SFRX);             //flush rx buffer
        cc_strobe(SRX);
        temp = cc_read(RXFIFO); //reset RA flag
        delay10x(1);
        cc_strobe(SIDLE);
        cc_strobe(SFRX);             //flush rx buffer
        cc_strobe(SRX);
      }/*else !=8 byte */

    }/*if(RA_RX data found*/






} /*while FOREVER*/
} /*main*/







          /* >>> Funktioner  <<< */
		  
		  
		  
		  
		  
          /* >>> write_ee (byte)   <<< */

void write_ee (unsigned char adress)
{

EEADR = adress;
WREN = 1;

EECON2 = 0x55;
EECON2 = 0xaa;
WR = 1;                         //start write
delay10x(2);                    //wait min 20ms
WREN = 0;                       

          
} /*write_ee*/





          /* >>> ---  <<< */
          /* >>> read_ee (byte)   <<< */

void read_ee (unsigned char adress)
{



EEADR = adress;
RD = 1;
          
} /*read_ee*/






          /* >>> ---  <<< */
          /* >>> delayx500us  <<< */

void delayx500us (char delay)
{

#define   OUTER_LOOPS           164

char temp;


       
       clrwdt();
       
       do
       {
         temp=OUTER_LOOPS;
         do{}while(--temp);
       }
       while(--delay);

}/*delayx500us*/







          /* >>> ---  <<< */
          /* >>> delay10x  10ms <<< */

void delay10x (unsigned char multiplikator)
{

unsigned char i;


for (i=1; i <= multiplikator; i++){
  clrwdt();                   
  delayx500us(20);
}/*for d1*/

}/*delay10x*/





          /* >>> ---  <<< */
          /* >>> RS232 RUTINER  <<< */
          /* >>> send_uart  <<< */

void send_uart (unsigned char TxByte)
{

do{                             //wait txbuffer empty
  nop();                        
}while(TXIF == 0);

TXREG = TxByte;

          
} /*send_uart*/







          /* >>> ---  <<< */
          /* >>> CHIPCON CC1101  <<< */

          /* >>> cc_reset  <<< */

void cc_reset (void)
{

  delayx500us(20);
  CSn = HIGH;
  SCLK = HIGH;
  SI = LOW;

  CSn = LOW;
  CSn = LOW;
  CSn = LOW;
  CSn = LOW;
  CSn = HIGH;
  delayx500us(1);

  SCLK = LOW;        //restore SCLK to FALSE (inactive) state
  CSn = LOW;
  cc_strobe(SRES);
          
} /*cc_reset*/





          /* >>> ----  <<< */
          /* >>> cc_init_fm  FM, 868,1959 MHz ch = 118 DataSpeed 1,2kb +10dBm  <<< */

void cc_init_fm (void)
{
 
   cc_write(IOCFG0,0x2d);       //TX pin input to modulator
   cc_write(IOCFG1,0x0a);       //SO or GD1 when CSn=1 0x0a= PLL lock
   cc_write(IOCFG2,0x0a);       // 0x06= Sync RX

   cc_write(FIFOTHR,0x00);
   cc_write(SYNC1,0xd3);
   cc_write(SYNC0,0x91);
   
   cc_write(PKTLEN,0x08);

   cc_write(PKTCTRL1,0x08);     //autoflush

   cc_write(PKTCTRL0,0x44);     //CRC en, white ON
   
   cc_write(ADDR,0x00);
   cc_write(CHANNRL,118);       //channel offset 
   cc_write(FSCTRL1,0x06);
   cc_write(FSCTRL0,0x00);
   
   cc_write(FREQ2,0x21);
   cc_write(FREQ1,0x46);
   cc_write(FREQ0,0xe4);

   cc_write(MDMCFG4,0xf5);      // BW 
   cc_write(MDMCFG3,0x83);      // speed
   cc_write(MDMCFG2,0x9b);      // FM
   cc_write(MDMCFG1,0x20);      // preamble,20 = 4 bytes, bit 0,1 exp channelspac
   cc_write(MDMCFG0,0x00);      //channel spacing

   cc_write(DEVIATN,0x15);

   cc_write(MCSM2,0x07);
   cc_write(MCSM1,0x0c);         //always clear, IDLE after TX, stay RX
   cc_write(MCSM0,0x18);        //cal IDLE to RX or TX

   cc_write(FOCCFG,0x16);
   cc_write(BSCFG,0x6c);

   cc_write(AGCTRL2,0x03);
   cc_write(AGCTRL1,0x40);
   cc_write(AGCTRL0,0x91);
   cc_write(WOREVT1,0x87);
   cc_write(WOREVT0,0x6b);

   cc_write(WORCTRL,0xf8);
   cc_write(FREND1,0x56);
   cc_write(FREND0,0x10);

   cc_write(FSCAL3,0xe9);
   cc_write(FSCAL2,0x2a);
   cc_write(FSCAL1,0x00);
   cc_write(FSCAL0,0x1f);

   cc_write(TEST2,0x81);
   cc_write(TEST1,0x35);
   cc_write(TEST0,0x09);

   cc_write(PATABLE,0xc0);     //+10dbm
          
} /*cc_init_fm*/




          
          /* >>> cc_strobe  <<< */
void cc_strobe (unsigned char reg_adress)
{
          /* >>> DEKLARATIONER  <<< */
unsigned char cc_temp, i, cc_stat;
          /* >>> -----  <<< */

clrwdt();                  
reg_adress &=0x7f;              //bit A7 = "0" for write
CSn = LOW;


          /* >>> SKICKA ADRESS  <<< */

for(i=0; i < 8; i++){
  cc_temp = reg_adress & 0x80;     //start with bit R/W bit
  if(cc_temp == 0x80)
    SI = HIGH;
  else
    SI = LOW;
  nop();
  nop();
  nop();
  nop();
  nop();
  SCLK = HIGH;
  nop();
  nop();
  nop();
  nop();
  nop();
  SCLK = LOW;
  reg_adress <<= 1;

}/*for i*/
CSn = HIGH;
          
} /*cc_strobe*/





          /* >>> ----  <<< */
          /* >>> cc_cal       <<< */

void cc_cal (void)
{
 
cc_strobe(SFSTXON);
delayx500us(4);

} /*cc_cal*/



          /* >>> cc_read  <<< */
unsigned char cc_read (unsigned char reg_adress)
{

unsigned char cc_data, cc_temp, i;


reg_adress |=0x80;              //bit A7 = "1" for read
SCLK = LOW;
nop();
nop();
CSn = LOW;


          /* >>> SKICKA ADRESS OCH LéS STATUS  <<< */

for(i=0; i < 8; i++){
  cc_temp = reg_adress & 0x80;     //start with bit R/W bit in pos 7
  if(cc_temp == 0x80)
    SI = HIGH;
  else
    SI = LOW;
  nop();
  nop();
  nop();
  nop();
  nop();
  SCLK = HIGH;
  nop();
  nop();
  nop();
  nop();
  nop();

  cc_stat <<= 1;                
  if(SO == 1)
    cc_stat += 0x01;

  SCLK = LOW;
  reg_adress <<= 1;

}/*for i*/


          /* >>> READ DATA  <<< */

for(i=0; i < 8; i++){          
  cc_data <<= 1;
  SCLK = HIGH;
  nop();
  nop();
  nop();
  nop();
  nop();
  if(SO == 1)
    cc_data += 0x01;
  SCLK = LOW;
}/*for data*/

CSn = HIGH;
return(cc_data);
          
} /*cc_read*/






          /* >>> ----  <<< */
          /* >>> cc_write  <<< */

void cc_write (unsigned char reg_adress, unsigned char reg_data)
{

unsigned char cc_temp, i;


clrwdt();                
reg_adress &=0x7f;              //bit A7 = "0" for write
SCLK = LOW;
nop();
nop();
nop();
CSn = LOW;


          /* >>> SKICKA ADRESS OCH LéS STATUS  <<< */

for(i=0; i < 8; i++){
  cc_temp = reg_adress & 0x80;     //start with bit R/W bit
  if(cc_temp == 0x80)
    SI = HIGH;
  else
    SI = LOW;
  nop();
  nop();
  nop();
  nop();
  nop();
  SCLK = HIGH;
  nop();
  nop();
  nop();
  nop();
  nop();

  cc_stat <<= 1;               
  if(SO == 1)
    cc_stat += 0x01;

  SCLK = LOW;
  reg_adress <<= 1;

}/*for i*/


          /* >>> SKICKA DATA  <<< */
for(i=0; i < 8; i++){
  cc_temp = reg_data & 0x80;
  if(cc_temp == 0x80)
    SI = HIGH;
  else
    SI = LOW;
  nop();
  nop();
  nop();
  nop();
  nop();
  SCLK = HIGH;
  nop();
  nop();
  nop();
  nop();
  nop();
  SCLK = LOW;
  reg_data <<= 1;
}/*for i*/

CSn = HIGH;
          
} /*cc_write*/




