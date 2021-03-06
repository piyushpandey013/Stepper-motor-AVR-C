/*
Copyright (c) 2012, fabiodive@gmail.com All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, self
list of conditions and the following disclaimer. Redistributions in binary
form must reproduce the above copyright notice, self list of conditions and
the following disclaimer in the documentation and/or other materials
provided with the distribution. Neither the name of fabiodive@gmail.com nor
the names of its contributors may be used to endorse or promote products
derived from self software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

fabiodive@gmail.com

--

This code is developed for personal use and study, it use the delay function 
instead of timers and interrupts, it is suitable for a brief introduction 
to the AVR programming world.

There is a potentiometer, source of desired speed and sense rotation,
2 leds for display which sense of rotation is running and USART
communications for monitoring the parameters.

*/

#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <stdlib.h>

#define F_CPU 16000000UL 

#define BAUDRATE 57600
#define BAUD_PRESCALLER (((F_CPU / (BAUDRATE * 16UL))) - 1)

//Declaration of our functions
void USART_init(void);
unsigned char USART_receive(void);
void USART_send(unsigned char data);
void USART_putstring(char* StringPtr);

//String[] is in fact an array but when we put the text between the " " 
//symbols the compiler threats it as a String and automatically puts 
//the null termination character in the end of the text
//char String[]="Hello world!!";
char String[16];

// Delay function, _delay_ms doesn't accept variables but constants
void delay_ms(uint16_t count) {
  while(count--) {
    _delay_ms(1);
  }
} 

void InitADC()
{
 //set prescaller to 128
 ADCSRA |= (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
 // Select Vref=AVcc
 // Avcc(+5v) as voltage reference
 ADMUX |= (1<<REFS0);
 ADMUX &= ~(1<<REFS1);
 //ADC in free-running  mode
 ADCSRB &= ~((1<<ADTS2)|(1<<ADTS1)|(1<<ADTS0));
 //Signal source, in this case is the free-running
 ADCSRA |= (1<<ADATE);
 //Power up the ADC
 ADCSRA |= (1<<ADEN);
 //Start converting
 ADCSRA |= (1<<ADSC);
}

uint16_t ReadADC(uint8_t ADCchannel)
{
 //select ADC channel with safety mask
 ADMUX = (ADMUX & 0xF0) | (ADCchannel & 0x0F);
 //single conversion mode
 ADCSRA |= (1<<ADSC);
 // wait until ADC conversion is complete
 while( ADCSRA & (1<<ADSC) );
 return ADC;
}

int main (void)
{
  uint16_t delay;
  uint16_t adc_value;
  
  // Call the USART initialization code
  USART_init();
 
  // Set sense LEDs in output mode
  DDRD |= (1<<PD3);
  DDRD |= (1<<PD4);

  // Set output mode of stepper driver pins
  // PD5=clock, PD6=enable, PD7=cw/ccw
  DDRD |= (1<<PD5);
  DDRD |= (1<<PD6);
  DDRD |= (1<<PD7);

  // put ON control pins
  PORTD |= (1<<PD5);
  PORTD |= (1<<PD6);
  PORTD |= (1<<PD7);

  // initialize ADC
  InitADC();

  while (1) {

      // reading potentiometer value
      // adc_value = ReadADC(0);
      adc_value = ADCW;

      if ((adc_value > 611) || (adc_value < 412)) {

        // enable stepper driver
        PORTD |= (1 << PD6);

        if (adc_value > 611) { 
          // sense of rotation, bit = 1
          PORTD |= (1 << PD7);
          // sense LEDs
          PORTD |= (1 << PD3);
          PORTD &= ~(1 << PD4);
          delay = 9 - ((adc_value - 612) / 51);
        }

        if (adc_value < 412) {
          // invert sense of rotation, bit = 0
          PORTD &= ~(1 << PD7);
          // sense LEDs
          PORTD &= ~(1 << PD3);
          PORTD |= (1 << PD4);
          delay = 1 + (adc_value / 51);
        }
        
        // swap clock pin
        PORTD ^= (1 << PD5);

        // convert integer to string 
        itoa(delay, String, 10);
        USART_putstring(String);

        // delay
        delay_ms(delay);

      } else {
        // Not in working range,
        // disabling stepper driver
        PORTD &= ~(1 << PD5);
        PORTD &= ~(1 << PD6);

        // sense LEDs off
        PORTD &= ~(1 << PD3);
        PORTD &= ~(1 << PD4);
      }
  }
  return 0;
}

void USART_init(void){
 UBRR0H = (uint8_t)(BAUD_PRESCALLER>>8);
 UBRR0L = (uint8_t)(BAUD_PRESCALLER);
 UCSR0B = (1<<RXEN0)|(1<<TXEN0);
 UCSR0C = (3<<UCSZ00);
}

unsigned char USART_receive(void){
  while(!(UCSR0A & (1<<RXC0)));
  return UDR0;
}

void USART_send(unsigned char data){
  while(!(UCSR0A & (1<<UDRE0)));
  UDR0 = data;
}

void USART_putstring(char* StringPtr){
  while(*StringPtr != 0x00){
    USART_send(*StringPtr);
    StringPtr++;
  }
  
  USART_send('\n');
}

