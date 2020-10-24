
#include <stdio.h>
#include <string.h>
#include <avr/eeprom.h>

#include "RFID.h"
void uart_init(){
	UBRRH = 0x00;
	UBRRL = 0xC;//set baurate 2400
	//UBRRL = 0x67;//set baurate 2400 crytal 4m
	
	/* Enable receiver and transmitter */
	UCSRB = (1<<RXEN)|(1<<TXEN);
	/* Set frame format: 8data, 1stop bit */
	UCSRC =	(1<<URSEL)|(1 << UCSZ1) | (1 << UCSZ0);
}
void USART_Transmit( unsigned char data ){
	/* Wait for empty transmit buffer */
	while ( !( UCSRA & (1<<UDRE)) );
	/* Put data into buffer, sends the data */
	UDR = data;
}
unsigned char USART_Receive( void ){
	/* Wait for data to be received */
	while ( !(UCSRA & (1<<RXC)) );
	/* Get and return received data from buffer */
	return UDR;
}
void SPI_MasterInit(void)
{
	DDRB |= (1<<SCK_PIN)|(1<<MOSI_PIN)|(1<<SS);
	SPCR |=	(1<<SPE)|(1<<MSTR)|(1<<SPR0);
	sbi(PORTB,SS);
}

void _SendString(char str[])
{
	int i =0;
	
	while (str[i] != 0x00)
	{
		USART_Transmit(str[i]);
		i++;
	}
}
void string2hexString(uint8_t* input, uint8_t* output){
	int loop;
	int i;
	
	i=0;
	loop=0;
	
	while(input[loop] != '\0')
	{
		sprintf((char*)(output+i),"%02X", input[loop]);
		loop+=1;
		i+=2;
	}
	//insert NULL at the end of the output string
	output[i++] = '\n';
}
void eeprom_init(){
	for(unsigned char i=0;i<100;i++){
		eeprom_write_byte((uint8_t*)i,0);
	}
}

MFRC522 rfid(2,6);
unsigned char indexEEPROM =0;

int main(void)
{
	
	SPI_MasterInit();
	uart_init();
	eeprom_init();
	DDRD = 0x80;
	rfid.begin();
	_SendString("START");
	uint8_t status;
	uint8_t data[MAX_LEN];
	uint8_t dataHex[MAX_LEN];
	while(1)
	{
		
		memset( data, '\0', sizeof(char)*MAX_LEN );
		status = rfid.requestTag(MF1_REQIDL, data);
		if (status == MI_OK) {
			status = rfid.antiCollision(data);
			int i=0;
			string2hexString(data,dataHex);
			while(data[i] != '\0')
			{
				USART_Transmit(dataHex[2*i]);
				USART_Transmit(dataHex[2*i+1]);
				eeprom_write_byte((uint8_t*)indexEEPROM,data[i]);
				indexEEPROM++;
				i++;
				
			}
			USART_Transmit('\n');
			if(indexEEPROM==20){
			//	USART_Transmit(eeprom_read_byte((const uint8_t*)5));
				for(int j = 0;j<20;j++){
					uint8_t c= eeprom_read_byte((const uint8_t*)j);
					USART_Transmit(c);
				}
					
			}
			
			sbi(PORTD,7);
			rfid.selectTag(data);
			// Stop the tag and get ready for reading a new tag.
			rfid.haltTag();
			cbi(PORTD,7);
		}
	}
}

