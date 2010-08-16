/* ATMega AVR 328/168 18 channel servo controller
* 
* Copyright (C) 2010 Dave Riess <davexriess@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; you may obtain a copy of the GNU General
* Public License Version 2 or later at the following locations:
*
* http://www.opensource.org/licenses/gpl-license.html
* http://www.gnu.org/copyleft/gpl.html
*
*
* Channel/Pin mapping is as follows:
* channels 0 through 5 --> pins PD2 through PD7 (Pins D0 and D1 reserved for UART)
* channels 6 through 11 --> pins PB0 through PB5
* channels 12 through 18 --> pins PC0 through PC5
*/

#define F_CPU 16000000
#define TRUE  1
#define FALSE  0

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//Ptypes
void PortInit();
void SortData();
 
//Global Vars
volatile int N;
int SERVO_DATA[18];							//stores the positions; indexed by servo channel
int PULSE_DATA[18];							//stores positions in an ordered list
volatile unsigned char PORTMASK[18][3];	                        //[rank], [BMASK, CMASK, DMASK]

int main( void )
{
	int i;

	PortInit();

	//Initialize Timer
	TCCR1A = 0x00;
	TCCR1B = 0x0A;		//CTC Enable + Prescale of 8
	OCR1A  = 40000;		//40000 = 50Hz
	TIMSK1 = 0x06;		//Timer Enable
	
	sei();

	while(1)		//main while loop - this is where the servos get their positin assignments
	{
		/*---independent position test---*/
		/*
		int test = 1300;
		for (i = 0; i < 18; i++)
		{
			servo_position[i] = test;
			test += 200;
			if (test > 4800) {test = 1300;}
		}
		*/
		/*--------------------------------*/
		
		/*-------wiggle servo test--------*/		
		/*
		for (i = 0; i < 18; i++)
		{
			servo_position[i] = 4800;
		}
		_delay_ms(2000);
		for (i = 0; i < 18; i++)
		{
			servo_position[i] = 1300;
		}
		_delay_ms(2000);
		for (i = 0; i < 18; i++)
		{
			servo_position[i] = 4800;
		}
		_delay_ms(2000);
		for (i = 0; i < 18; i++)
		{
			servo_position[i] = 1300;
		}
		_delay_ms(2000);
		*/
		/*--------------------------------*/
	}

	return 0 ;
}

//Period ISR: this occurs at the end of every period (50Hz)
ISR(TIMER1_COMPA_vect)
{
	//set all chennels high	
	PORTB |= 0x3F;
	PORTC |= 0x3F;
	PORTD |= 0xFC;
	TCNT1 = 0;
	N = 0;
	OCR1B = PULSE_DATA[0];
}

//Pulse ISR: services every unique pulse width
ISR(TIMER1_COMPB_vect)
{
	PORTB &= ~PORTMASK[N][0];
	PORTC &= ~PORTMASK[N][1];
	PORTD &= ~PORTMASK[N][2];
	//set up next pulse
	N ++;
	OCR1B = PULSE_DATA[N];
}


//SortData does two things
	// a) finds the next pulse width that needs servicing and queues it into OCR1B
	// b) adjusts the port masks (BMASK, CMASK, DMASK) so the proper channels are set low
void SortData()
{
	int i;
	int c;

	
	//vars for bubble sorting
	int temp;
	int sorted;
	int pass;
	int start;
	
	//copy SERVO_DATA into PULSE_DATA
	for (i=0; i < 18; i++) {
	         PULSE_DATA[i] = SERVO_DATA[i];
	}

	//step1: sort PULSE_DATA low to high
		
	sorted = FALSE;
	pass = 1;
	start = 0;
	
	while ((sorted == FALSE) && (pass < 18)) {
		sorted = TRUE;
		for (i=0; i < (18 - pass); i++) {
			if (PULSE_DATA[i] > PULSE_DATA[i+1]) {
				//exchange places
				temp = PULSE_DATA[i];
				PULSE_DATA[i] = PULSE_DATA[i+1];
				PULSE_DATA[i+1] = temp;
				sorted = FALSE;
			}
		}
		pass ++;
	}
	
	//step2: zero out any duplicates
	sorted = FALSE;
	pass = 1;
	start  = 0;

	while ((sorted == FALSE) && (pass < 18)) {
		sorted = TRUE;
		for (i=0; i < (18 - pass); i++) {
			if (PULSE_DATA[i] == PULSE_DATA[i+1]) {
				//zero duplicates
				PULSE_DATA[i+1] = 0;
				start ++;
				sorted = FALSE;
			}
		}
		pass ++;
	}	
	
	//step3: move zeros to the end
	sorted = FALSE;
	pass = 1;
	
	while ((sorted == FALSE) && (pass < 18)) {
		sorted = TRUE;
		for (i=0; i < (18 - pass); i++) {
			if (PULSE_DATA[i] > PULSE_DATA[i+1]) {
				//exchange places
				temp = PULSE_DATA[i];
				PULSE_DATA[i] = PULSE_DATA[i+1];
				PULSE_DATA[i+1] = temp;
				sorted = FALSE;
			}
		}
		pass ++;
	}
	
	//go down PULSE_DATA list and pull channels from SERVO_DATA associated with each position..
	//put these position values into the appropriate PORTMASK
	for (c = 0; c < 18; c++)         // loop through PULSE_DATA.. 18 should be a sizeOf() macro
	{
	  for (i = 0; i < 18; i++)         // loop through SERVO_DATA
		{
			if (SERVO_DATA[i] == PULSE_DATA[c])
			{
				switch (i/6) {
					case 0:
						//this is PORTD
						PORTMASK[c][0] |= (1 << (i+2));
						break;
					case 1:
						//this is PORTB
						PORTMASK[c][1] |= (1 << (i-6));
						break;
					case 2:
						//this is PORTC
						PORTMASK[c][2] |= (1 << (i-12));
						break;
				}
			}
		}
	}
	
	// PORT masks setup, now wait for COMPA ISR

	/*
	//identify next relevant position
	for (i = 0; i < 18; i++)
	{
		if ((servo_position[i] > prev) && (servo_position[i] <= next))
		{
			next = servo_position[i];
		}
	}
	
	//set port masks for use during next COMPB
	for (i = 0; i < 18; i++)
	{
		if (servo_position[i] == next)
		{
			switch (i/6) {
				case 0:
					//this is PORTD
					DMASK &= ~(1 << (i+2));
					break;
				case 1:
					//this is PORTB
					BMASK &= ~(1 << (i-6));
					break;
				case 2:
					//this is PORTC
					CMASK &= ~(1 << (i-12));
					break;
			}
		}
	}
	
	OCR1B = next;
	*/
}

void PortInit()
{
	DDRB = 0xFF;	PORTB = 0x00;			
	DDRC = 0xFF;	PORTC = 0x00;			
	DDRD = 0xFF;	PORTD = 0x00;
}
