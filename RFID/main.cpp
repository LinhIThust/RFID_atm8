
#include <stdio.h>
#include <string.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#define MAX_EEPROM 251
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
//UDR=Temp;

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

void send_to_pc(){
	char temp[2];
	uint8_t c;
	for(int j = 1;j<MAX_EEPROM;j++){
		c=eeprom_read_byte((const uint8_t*)j);
		sprintf(temp,"%02X", c);
		USART_Transmit(temp[0]);
		USART_Transmit(temp[1]);
		if(j % 5==0) USART_Transmit('\n');
	}
	USART_Transmit('\n');
}
MFRC522 rfid(2,6);
int main(void)
{
	
	SPI_MasterInit();
	uart_init();
	DDRD = 0x80;
	rfid.begin();
	_SendString("START");
	uint8_t status;
	uint8_t data[MAX_LEN];
	uint8_t indexEEPROM =1;
	bool check =true;
	bool check_write =false;
	char temp[2];
	if(eeprom_read_byte(0)=='Y'){
		while ( !(UCSRA & (1<<RXC)));
		eeprom_write_byte(0,'N');
		send_to_pc();
		_delay_ms(100);
		
	}
	while(1)
	{
		memset( data, '\0', sizeof(char)*MAX_LEN );
		status = rfid.requestTag(MF1_REQIDL, data);
		
		if (status == MI_OK && indexEEPROM <MAX_EEPROM) {
			if(!check_write){
				check_write =true;
				eeprom_write_byte(0,'Y');
			}
				
			status = rfid.antiCollision(data);
			int i=0;
			while(data[i] != '\0')
			{
				//sprintf(temp,"%02X", data[i]);
				//USART_Transmit(temp[0]);
				//USART_Transmit(temp[1]);
				if(indexEEPROM >MAX_EEPROM) break;
				eeprom_write_byte((uint8_t*)indexEEPROM,data[i]);
				indexEEPROM++;
				i++;
				
			}
			//USART_Transmit('\n');
			sbi(PORTD,7);
			rfid.selectTag(data);
			// Stop the tag and get ready for reading a new tag.
			rfid.haltTag();
			cbi(PORTD,7);
		}
		if(indexEEPROM ==MAX_EEPROM && check){
			eeprom_write_byte(0,'Y');
			check=false;
		}
	}
}

