/*
 * 20210309_tiny13_timer.c
 *
 * Created: 2021-03-09 21:08:38
 * Author : uwezi
 */ 

// output on PB3, active high
// trigger on PB1 / INT0 active low

#define F_CPU 1200000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <avr/sleep.h>

volatile int16_t timeout = 0;
volatile int16_t settime = 0;
volatile uint8_t do_adc  = 0;

ISR(INT0_vect)
{
  timeout = settime;
  PORTB |= (1 << PB3);
}


// timer interrupt about 586/s
ISR(TIM0_OVF_vect)
{
  int16_t dummy;
  do_adc ++;
  if (do_adc > 200)
  {
    do_adc = 0;
    PORTB |= (1 << PB2);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC)) { }
    PORTB &= ~(1 << PB2);
    dummy = 7*settime + ADC;
    settime = dummy / 8;
  }
  if (timeout > 0)
  {
    timeout --;
  }
  else
  {
    PORTB &= ~(1 << PB3);
  }
}

// PB0 unused, pull-up
// PB1 trigger
// PB2 alive blink
// PB3 transistor out
// PB4 ADC in
// PB5 reset

int main(void)
{
  DDRB   = (1 << PB3) | (1 << PB2);
  // pull-up on PB1
  PORTB |= (1 << PB5) | (1 << PB1) | (1 << PB0);
  
  // disable comparator
  ACSR = (1 << ACD);
  
  // prepare sleep
  set_sleep_mode(SLEEP_MODE_IDLE);
  
  // ADC channel ADC2 
  // internal ref
  // normal adjustment
  // 1:8 prescaler 150 kHz
  ADMUX  = (0 << REFS0) | (0 << ADLAR) | (1 << MUX1) | (0 << MUX0);
  ADCSRA = (1 << ADEN) | (0 << ADSC) | (0 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
  // first conversion
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC)) { }
  settime = ADC;
  
  // timer 0 normal counting
  // 1:8 prescaler
  // 1.2 MHz / 8 / 256 = 585.9375 Hz
  TCCR0A = 0b00000000;
  TCCR0B = (0 << CS02) | (1 << CS01) | (0 << CS00);
  TIMSK0 = (1 << TOIE0); 
  
  // falling edge triggers INT0
  MCUCR |= (1 << ISC01) | (0 << ISC00);
  GIMSK |= (1 << INT0); 
  sei();
  
  while (1) 
  {
    sleep_enable();
    sleep_cpu();
  }
}


