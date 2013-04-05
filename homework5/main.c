/*******************************************************************************************\
| main.c
| Author: Todd Sukolsky
| Initial Build: 3/31/2013
| Last Revised: 4/3/13
|================================================================================
| Description: This is the main program file for homework 5 of EC450, Microprocessors.
|		The goal of this assignment is to make a "frequency multiplier" using the MSP430.
|		See "homework5_writeup" for more information. The module works for frequencies between
|		around 16 Hz to 40 kHz
|--------------------------------------------------------------------------------
| Revisions: 3/31-Initial Build. Made basic timeing mechanisms using timer 0
|			 4/3- Thought I finished, didn't though. Something is wrong with IO declarations
|			 4/5- Fixed issue with port declarations, works now.
|================================================================================
| *NOTES:
\*******************************************************************************/

#include "msp430g2553.h"

#define INPUT_CLK	BIT2		//Pin 1.2, TA0.1
#define OUTPUT_CLK 	BIT1		//PIN1.1, TA0.0

/************************************/
/*		Forward Declarations		*/
/************************************/

void initTimer0();
void initGPIO();

/************************************/
/*			Global Variables		*/
/************************************/
unsigned int period=0;					//The current period, needs to be global
unsigned char overflows=0;				//Used for debugging purposes.

/************************************/
/*			Main Program			*/
/************************************/

void main()
{
	//Set watchdog off and make correct smclk
	WDTCTL = WDTPW + WDTHOLD; 		// Halt the watchdog timer
	BCSCTL1 = CALBC1_16MHZ;   		// 16MHz SMCLK bits.
	DCOCTL  = CALDCO_16MHZ;

	//Initialize Timer0, then turn the CPU off.
	initTimer0();
	initGPIO();
	_bis_SR_register(GIE+LPM0_bits);	// Global interrupts enabled, CPU off
}

/************************************/
/*				Functions			*/
/************************************/
/*******************************************************************************/
void initTimer0(){
	TA0CTL |= TACLR;              		//Clear whatever was in the Control register.
	TA0CTL |= TASSEL_2+ID_3+MC_2;  		//clk/8, continusous using the SMCLK.

	TA0CCTL1 |=CM_3+CAP+CCIE; 			//Enable compare interrupt, capture on the rising AND falling edge of the clock
	TA0CCTL0 |=OUTMOD_4+CCIE;
	
	TA0CCR0=300;						//Initialize the half period to 300
	TA0CTL |= TAIE;						//Enable the interrupt last so we don't get taken out of the init by accident
}
/*******************************************************************************/
void initGPIO(){
	//Input clock is on TA0.1, pin1.2. Output clock is on TA0.0, pin 1.1
	P1SEL |= INPUT_CLK+OUTPUT_CLK;
	P1DIR &= ~INPUT_CLK;			//input
	P1DIR |= OUTPUT_CLK;			//output
}
/*******************************************************************************/
void interrupt outputClock()
{	//Adjust the frequency
	TA0CCR0 += period/3;
}
/*******************************************************************************/
void interrupt inputClock()
{
	static unsigned int risingEdge=0;
	if (TA0IV & 0x02){
		if (TA0CCTL1 & CCI){	//we swaw the rising edge, record at what point that was
			risingEdge=TA0CCR1;
		} else {				//we say the falling edge, set the period.
			period=TA0CCR1-risingEdge;
			TA0CCTL1 &= ~CCIFG;	//kill the flag
		}
	} else if (TA0IV & 0x0A){	//overflow vector, add to global overflows. compare to 10(base10).
		overflows++;
	} else;
}
/*******************************************************************************/
ISR_VECTOR(outputClock,".int09")	//register the outputClock with the timerA0 interrupts
ISR_VECTOR(inputClock,".int08")		//Register the inputClock with general interrupt for timerA1 interrupt
