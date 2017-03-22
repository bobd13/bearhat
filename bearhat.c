/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.        Joerg Wunsch
 * ----------------------------------------------------------------------------
 *
 * Simple AVR demonstration.  Controls a LED that can be directly
 * connected from OC1/OC1A to GND.  The brightness of the LED is
 * controlled with the PWM.  After each period of the PWM, the PWM
 * value is either incremented or decremented, that's all.
 *
 * $Id: demo.c 1637 2008-03-17 21:49:41Z joerg_wunsch $

 created 3/3/14 BD from "AVRlibc simple projects/demo.c"
 - recompiling works (after reprogramming and repairing chips for programmer -
 sigh
 updated 3/4/14
 - remove reliance on compat.h by changing names and using bit names
 - the following checked out visual timing ok:
	TCCR0A =  _BV(COM0A1) | _BV(WGM00) | _BV(WGM01);
	TCCR0B = _BV(CS02) | _BV(CS00);
	- previously replaced "|=" with "=" 'cause it appears prescale selection
	not cleard
	- this gives a PWM period = 0.25 sec with 1 MHz CPU
	- for 512 PWMs ~1 second, need 131 kHz clkio, try 1/8 prescale
	- matches effect of "AVRlibc simple projects/demo.c"

	- now lets go after power
	- added power reduction settings, still ok - looks like it might be a little
	lower, but pulling led still @ ~250 uA (after it stabilizes)
	
	- Fuses were as factory 0x62, 0xdf, 0xff -> 8 MHz, 1/8
	- change to 128 kHz and no 1/8
	- Fuses are 0xe4, 0xdf, 0xff
	- note - 1 bit on SPI programming must be >= 4 CPU clocks

	- change clkio from 1/8 to no prescale
	- program fuses
	$ avrdude -c usbtiny -p t85 -U lfuse:w:0xe4:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

	- from now on, must add -B 32

bobd@HP-Mini:~/AVR uC/projects/bearhat$ avrdude -c usbtiny -p t85 -U lfuse:w:0xe4:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

avrdude: AVR device initialized and ready to accept instructions

Reading | ################################################## | 100% 0.01s

avrdude: Device signature = 0x1e930b
avrdude: reading input file "0xe4"
avrdude: writing lfuse (1 bytes):

Writing | ################################################## | 100% 0.02s

avrdude: 1 bytes of lfuse written
avrdude: verifying lfuse memory against 0xe4:
avrdude: load data lfuse data from input file 0xe4:
avrdude: input file 0xe4 contains 1 bytes
avrdude: reading on-chip lfuse data:

Reading | ################################################## | 100% 0.00s

avrdude: verifying ...
avrdude: 1 bytes of lfuse verified
avrdude: reading input file "0xdf"
avrdude: writing hfuse (1 bytes):

Writing | ################################################## | 100% 0.00s

avrdude: 1 bytes of hfuse written
avrdude: verifying hfuse memory against 0xdf:
avrdude: load data hfuse data from input file 0xdf:
avrdude: input file 0xdf contains 1 bytes
avrdude: reading on-chip hfuse data:

Reading | ################################################## | 100% 0.00s

avrdude: verifying ...
avrdude: 1 bytes of hfuse verified
avrdude: reading input file "0xff"
avrdude: writing efuse (1 bytes):

Writing | ################################################## | 100% 0.00s

avrdude: 1 bytes of efuse written
avrdude: verifying efuse memory against 0xff:
avrdude: load data efuse data from input file 0xff:
avrdude: input file 0xff contains 1 bytes
avrdude: reading on-chip efuse data:

Reading | ################################################## | 100% 0.00s

avrdude: verifying ...
avrdude: 1 bytes of efuse verified

avrdude: safemode: Fuses OK

avrdude done.  Thank you.

bobd@HP-Mini:~/AVR uC/projects/bearhat$


Took multiple tries, but:
bobd@HP-Mini:~/AVR uC/projects/bearhat$ avrdude -c usbtiny -p t85 -B 100 -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h

avrdude: AVR device initialized and ready to accept instructions

Reading | ################################################## | 100% 0.02s

avrdude: Device signature = 0x1e930b
avrdude: reading lfuse memory:

Reading | ################################################## | 100% 0.01s

avrdude: writing output file "<stdout>"
0xe4
avrdude: reading hfuse memory:

Reading | ################################################## | 100% 0.01s

avrdude: writing output file "<stdout>"
0xdf
avrdude: reading efuse memory:

Reading | ################################################## | 100% 0.01s

avrdude: writing output file "<stdout>"
0xff

avrdude: safemode: Fuses OK

avrdude done.  Thank you.

	- now stable with TIMER0 running, no led, ~36 uA! in idle (no watchdog stuff)
	- led was peaking ~2.6 mA before, now 2.2 mA (eyeball average of 1.5 mA)


 */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

// #include "iocompat.h"		/* Note [1] */

enum { UP, DOWN };

ISR (TIMER0_OVF_vect)		/* Note [2] */
{
	static uint16_t pwm = 0;	/* Note [3] */
	static uint8_t direction = UP;

	switch (direction)		/* Note [4] */
    {
		case UP:
			if (++pwm == 255)
				direction = DOWN;
			break;

		case DOWN:
			if (--pwm == 0)
				direction = UP;
			break;
    }

	OCR0A = pwm;			/* Note [5] */
}

void
ioinit (void)			/* Note [6] */
{
	/* Timer 0 is 8-bit PWM */
	TCCR0A =  _BV(COM0A1) | _BV(WGM00) | _BV(WGM01);
	// clkio / 1024
	// TCCR0B = _BV(CS02) | _BV(CS00);
	// clkio 
	TCCR0B = _BV(CS00);
	// clkio / 8
	// TCCR0B = _BV(CS01) ;

	/* Set PWM value to 0. */
	OCR0A = 0;

	/* Enable OC0A as output. */
	DDRB = _BV(DDB0);

	/* Enable timer 1 overflow interrupt. */
	TIMSK = _BV(TOIE0);

	// Power Consumption Reduction
	// Analog Comparator
	ACSR = _BV(ACD);

	// Power Reduction Register
	PRR = _BV(PRTIM1) | _BV(PRUSI) | _BV(PRADC);

	sei ();
}

int
main (void)
{

	ioinit();

	/* loop forever, the interrupts are doing the rest */

	while(1) {
		sleep_mode();
	}

	return (0);
}
