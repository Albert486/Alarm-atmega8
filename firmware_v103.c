#define F_CPU 8000000UL  // 8 MHz

#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/twi.h>
#include <avr/io.h>

#define TEMPO_ODLICZANIA 277777
#define TIMEOUT 12000

volatile unsigned char command, stan = 0;

ISR(TWI_vect) // Wywolanie przerwania
{
	uint8_t status = TWSR & 0xFC; // maskowanie bitów
	switch(status) // Sprawdzenie jaki status wywołał przerwanie
	{
		case 0xA8: // Odebrano Adres z żądaniem odczytu, wyslanie ACK i wystawienie danych na TWDR
		TWDR = stan;
        break;
        
		case 0x60: // 0x60 - Odebrany adres i wysłany ACK
        break;
    
		case 0x80: // Odebrany Bajt i wysłąny ACK
        command = TWDR; // Wyświetla bajt na porcie b
		break;
	}
 
    TWCR |= (1<<TWINT); // aktywacja TWI - TWINT ustawiany po każdej akcji
}

void twstart(void)
{
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while ((TWCR & (1<<TWINT)) == 0); // Start
}

void twsend(unsigned char data)
{
	TWDR = data; // TWDR - TWI Data Register Cały bajt danch do wysyłania
	TWCR = (1<<TWINT)|(1<<TWEN);
	while ((TWCR & (1<<TWINT)) == 0); // Wyślij bajt
}

void twstop(void)
{
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN); // Stop
}

void _delay_s(unsigned int s)
{
	unsigned int i;
	
	for (i = 0; i < s; i++)
	{
		_delay_ms(500);
		_delay_ms(500);
	}
}

void send_char(char c)
{
	while(!(UCSRA & (1 << UDRE)));
	
	UDR = c;
}

void send_string(char tekst[])
{
	int i = 0;
	
	while (tekst[i] != 0x00)
	{
		send_char(tekst[i]);
		i++;
	}
}

unsigned char read_char(void)
{
	unsigned int to = 0;
	
	while(!(UCSRA & (1 << RXC)) && (to < TIMEOUT))
	{
		to++;
	}
	
	if(to < TIMEOUT)
	return UDR;
	else
	return 127;
}

unsigned char read_sms(void)
{
	_delay_ms(50);
	send_string("AT+CMGF=1");	// ustawienie modułu sim
	send_char(13);
	_delay_ms(50);
	send_string("AT+CMGL=\"ALL\"");	// odczytanie wszystkich wiadomosci
	send_char(13);
}

unsigned char del_sms(void)
{
	_delay_ms(50);
	send_string("AT+CMGD=1,4");	// usuwanie wszystkich wiadomosci
	send_char(13);
	_delay_ms(50);
}


int main(void)
{
	sei(); // Włączanie przerwań
	
	unsigned char chr;
	unsigned char data[10];
	unsigned char zl[5];
	unsigned char znak = ' ';
	unsigned char timer_led = 0;
	unsigned long timer_al = 0;
	unsigned long timer = 0;
	unsigned char alarm_zw = 0, uzbrojono = 0;
	unsigned char passwd[12];
	unsigned char i;

	
	for (i = 0; i < 5; i++)
	zl[i] = ' ';
	
	for (i = 0; i < 10; i++)
	data[i] = ' ';
	
	// ======= PORTTY ==== //
	DDRB = 128;
	DDRD = 242;
	DDRC = 127;
	
	PORTB = 126;
	PORTD = 15;
	PORTC = 127;
	
	// -------------------- //
	
	// ========= PROTOKOLY ===== //
	// TWAR: (8)- [A][A][A][A][A][A][A][R/W] -(1)
	TWAR = 232; // Ustawianie adresu urządzenia (dodać 1 dla trybu odbioru)
  
	// TWCR - TWI Control Register: (8)- [TWINT][TWEA][TWSTA][TWSTO][TWWC][TWEN][-][TWIE] -(1)
	TWCR |= (1<<TWEN) | // TWI Enable (wł interfejs i2c)
			(1<<TWEA) | // Wyślij bit potwierdzenia (ACK)
			(1<<TWIE);  // TWI Interrupt enable (wł przerwań)
	
	// TWSR - TWI Status Register (8)- [TWS7][TWS6][TWS5][TWS4][TWS3][-][TWPS1][TWPS0] -(!)
	// [TWS7] - [TWS3] - Status interfejsu i2c, [TWPS1-0] - Preskaler
	// ----
	// [TWPS1]: [TWPS0]:	Preskaler:
	//    0        0       		1
	//    0        1			4
	//	  1		   0			16
	//    1        1 			64
	TWSR = 3;
	
	// TWBR - TWI Bit Rate (8)- [TWBR7] .. [TWBR0]
	// Wzór na prędkość transmisji:
	// SCLfreq = CPUclock / (16 + 2 * TWBR * 4^TWPS)
	TWBR = 0b10000011;
	
	// UART //
	UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0); // Standardowe ustawienie 8-bit
	UBRRL = 51; // UBRRL = (F_CPU/(16*BautRate)-1)  przyklad:(8000000/(16*9600))-1
	UCSRB = (1 << RXEN) | (1 << TXEN); // Włącze RX, włącz TX
	// --- //


	
	
	
	for (i = 0; i < 30; i++)
	{
		PORTD = 240;
		_delay_ms(500);
		PORTD = 15;
		_delay_ms(500);
	}
	
	
	send_string("AT");
	send_char(13);
	
	_delay_ms(500);
	_delay_ms(500);
	
	send_string("AT");
	send_char(13);
	
	_delay_ms(500);
	_delay_ms(500);
	
	send_string("AT");
	send_char(13);
	
	_delay_ms(500);
	_delay_ms(500);
		
	send_string("AT+CREG?");
	send_char(13);
	
	while(1)
	{
		if(timer > 750)
		{
			timer = 0;
			
			send_string("AT+CREG?");
			send_char(13);
		}
			
			
		// ================== ALARM NATYCHMIASTOWY================== //
		if(  ((bit_is_set(PINB, PB0)) || (bit_is_set(PINB, PB1))) && (uzbrojono == 1)  )
		{	
			PORTB |= (1 << PD7);
			PORTD |= (1 << PD4);
			PORTD |= (1 << PD7);
			uzbrojono = 0;
		}
		
		// =================== ALARM ZWLOCZNY ======================= //
		if ((bit_is_set(PINB, PB2) && (uzbrojono == 1)))
		{
			PORTD |= (1 << PD4);
			
			while((timer_al < 1000) && (bit_is_set(PIND, PD3)))
			{
				_delay_ms(15);
				timer_al++;
			}
			
			timer_al = 0;
			
			if(bit_is_clear(PIND, PD3))
			{
				uzbrojono = 0;
				PORTD &= ~(1 << PD4);
			}
			else
			{
				PORTB |= (1 << PD7);
				PORTD |= (1 << PD4);
				PORTD |= (1 << PD7);
				uzbrojono = 0;
			}
		}

		// ==================== OBWODY - LED =================== //
		if(   (bit_is_set(PINB, PB0)) || (bit_is_set(PINB, PB1)) || (bit_is_set(PINB, PB2))  )
			PORTD |= (1 << PD6);
		else
			PORTD &= ~(1 << PD6);
		
		
		// ====================== UZBRAJANIE WEW. ========== //
		if ((bit_is_clear(PIND, PD2) && (uzbrojono == 0)))
		{
			PORTD |= (1 << PD4);
			PORTD &= ~(1 << PD7);
			
			
			while((timer_al < 1000) && (bit_is_set(PIND, PD3)))
			{
				_delay_ms(15);
				timer_al++;
			}
			
			timer_al = 0;
			
			if(bit_is_clear(PIND, PD3))
			{
				uzbrojono = 0;
				PORTD &= ~(1 << PD4);
			}
			
			uzbrojono = 1;
		}
		
		// ======================= ROZBRAJANIE WEW. ======== //
		if (bit_is_clear(PIND, PD3))
		{
			uzbrojono = 0;
			PORTD &= ~(1 << PD7);
			PORTB &= ~(1 << PD7);
			PORTD &= ~(1 << PD4);
		}
		
		// ===================== Kontrolka UZBROJENIE ===== //
		if(uzbrojono == 1)
		{
			if(timer_led > 100)
				timer_led = 0;
				
			if(timer_led < 50)
				PORTD |= (1 << PD4);
				
			if((timer_led > 50) && (timer_led < 100))
				PORTD &= ~(1 << PD4);
		}
		else
		if(uzbrojono == 0)
			PORTD &= ~(1 << PD4);
		
		
		// ====================== ZAPYTANIE STAN KONTA ====== //
		if(command == 60)
		{
			command = 0;

			send_string("AT+CUSD=1");
			send_char(13);
			_delay_ms(50);
			send_string("AT+CUSD=1,\"*101#\"");
			send_char(13);
		}
			
		// ********************************* NASLUCHIWANIE RS232 - komend ** //
	
		chr = read_char();
		
		if(chr == '+')								// ROOT "+"
		{
			chr = read_char();
			if(chr == 'C')
			chr = read_char();
			if(chr == 'M')
			{
				chr = read_char();
				if(chr == 'T')
				chr = read_char();
				if(chr == 'I')
				read_sms();
			}
			else
			
			if(chr == 'R')
			chr = read_char();
			if(chr == 'E')
			chr = read_char();
			if(chr == 'G')
			chr = read_char();
			if(chr == ':')
			chr = read_char();
			if(chr == ' ')
			chr = read_char();
			if(chr == '0')
			chr = read_char();
			if(chr == ',')
			chr = read_char();
			if((chr == '1') || (chr == '5'))
			PORTD |= (1 << PD5);
			else
			PORTD &= ~(1 << PD5);

		}
		else
		
		if((chr == 'I') || (chr == 'i') || (chr == 'R'))		// INFORMACJA
		{
			chr = read_char();
			if((chr == 'N') || (chr == 'n') || (chr == 'I'))
			chr = read_char();
			if((chr == 'F') || (chr == 'f') || (chr == 'N'))
			chr = read_char();
			if((chr == 'O') || (chr == 'o') || (chr == 'G'))
			{
				send_string("AT+CMGF=1");
				send_char(13);
				
				_delay_ms(50);
				
				send_string("AT+CMGS=\"+48777777777\"");
				send_char(13);
				
				_delay_ms(50);
			
				send_string("Wszystko gro! :-)");
				send_char(26);
				
			}
		}
		else
		
		if((chr == 'A') || (chr == 'a'))		// Kanał PC0
		{
			chr = read_char();
			if(chr == '1')
			{
				PORTC &= ~(1 << PC0);
				del_sms();
			}
			else
			if(chr == '0')
			{
				PORTC |= (1 << PC0);
				del_sms();
			}
		}
		else
		
		if((chr == 'B') || (chr == 'b'))		// Kanał PC1
		{
			chr = read_char();
			if(chr == '1')
			{
				PORTC &= ~(1 << PC1);
				del_sms();
			}
			else
			if(chr == '0')
			{
				PORTC |= (1 << PC1);
				del_sms();
			}
		}
		else
		
		if((chr == 'C') || (chr == 'c'))		// Kanał PC2
		{
			chr = read_char();
			if(chr == '1')
			{
				PORTC &= ~(1 << PC2);
				del_sms();
			}
			else
			if(chr == '0')
			{
				PORTC |= (1 << PC2);
				del_sms();
			}
		}
		else
		
		if((chr == 'D') || (chr == 'd'))		// Kanał PC3
		{
			chr = read_char();
			if(chr == '1')
			{
				PORTC &= ~(1 << PC3);
				del_sms();
			}
			else
			if(chr == '0')
			{
				PORTC |= (1 << PC3);
				del_sms();
			}
		}
		
		timer_led++;
		timer++;
	}
}
