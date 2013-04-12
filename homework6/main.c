/*******************************************************************************\
| sukolsky_hwk6.c
| Author: Todd Sukolsky
| Initial Build: 4/12/2013
| Last Revised: 4/12/2013
| Copyright of Todd Sukolsky
|================================================================================
| Description: This program is EC450s homweork 6. THe description is to have the MSP430
|	take some  analog signal reading and then print the result through the UART
|	ports to a listening receiver, ie minicom or PuTty
|--------------------------------------------------------------------------------
| Revisions:
|	4/12: Initial build. Had to tweak the sprintf since it doesn't take float values. See comments.
|================================================================================
| *NOTES:
\*******************************************************************************/

#include "msp430g2553.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#define ADC_INPUT_BIT_MASK 0x10
#define ADC_INCH 0x0A				//1010=10=internal temperature sensor

#define RED (1 << 0);
#define GREEN (1 << 6);

//Macros
#define __startADC() ADC10CTL0 |= ADC10SC

void initUart();
void initWDT();
void initADC();
void initGPIO();

void main()
{
	//Set watchdog off and make correct smclk
	WDTCTL = WDTPW + WDTHOLD; 		// Halt the watchdog timer
	BCSCTL1 = CALBC1_1MHZ;   		// 1MHz SMCLK bits.
	DCOCTL  = CALDCO_1MHZ;
	
	//Initialize everything we need.
	initUart();
	initADC();
	initWDT();
	initGPIO();

	__bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, interrupts enabled
}

//Initialize the GPIOS
void initGPIO(){
	P1DIR=RED+GREEN;
	P1OUT |= RED;
	P1OUT &= ~GREEN;
}

//Iinit our UART for BAUD 9600
void initUart(){
	P1SEL = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
	P1SEL2 = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
	UCA0CTL1 |= UCSSEL_2;                     // SMCLK
	UCA0BR0 = 104;                            // 1MHz 9600
	UCA0BR1 = 0;                              // 1MHz 9600
	UCA0MCTL = UCBRS0;                        // Modulation UCBRSx = 1
	UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
	//IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt
}

//Start the watchdog timer.
void initWDT(){
	// setup the watchdog timer as an interval timer
  	WDTCTL =(WDTPW + WDTTMSEL + WDTCNTCL);      // (bit 4) select interval timer mode, clear the counter. Clock source = smclk/32.
   	IE1 |= WDTIE;								// enable the WDT interrupt (in the system interrupt register IE1)
}//end initWDT

//Initialize ADC, uses clock div4, start on "Start conversion", internal temp ADC measurement.
void initADC(){
	ADC10CTL1= INCH_10+SHS_0+ADC10DIV_4+CONSEQ_0+ADC10SSEL_3;
	ADC10DTC1=1;   // two block per transfer
	ADC10CTL0= SREF_0	//reference voltages are Vss and Vcc
 	          +ADC10SHT_3 //64 ADC10 Clocks for sample and hold time (slowest)
 	          +ADC10ON	//turn on ADC10
 	          +ENC		//enable (but not yet start) conversions
 	          +REFON
 	          ;
}//end initADC

interrupt void timerOverflow_handler(){
	static int numberOfHits=0;

	//If we ahve waited long enough
	if (numberOfHits++>=60){				//arbitrary number 60 is, turns out to be ~1.5 seconds
		//Variables needed
		volatile unsigned int adcReading=0;
		char outputString[20];
		int dec1, dec2;
		volatile float tempCelcius;

		//Take an ADC
		__startADC();
		while ((ADC10CTL1 & ADC10BUSY)==ADC10BUSY);					//wait for it to be done.
		adcReading=ADC10MEM;										//Load into a variable
		tempCelcius=(((adcReading*3.5)/1024.0)-.986)/.00355;		//Doing a volt check of the board reveals 3.5V is VCC

		//Convert to correct printable form
		dec1=tempCelcius;											//Get what's before decimal
		dec2=(tempCelcius-dec1)*10000;								//Get what's after the decimal
		sprintf(outputString,"%d.%d C.",dec1,dec2);					//Print the two decimals

		//Toggle the red led and reet number of hits.
		numberOfHits=0;
		P1OUT ^= RED;

		//Send data to PC
		int i=0;
		for (i=0; i<strlen(outputString);i++){
			while (!(IFG2&UCA0TXIFG));								//Wait for tranmit to be done, then add next char in buffer.
			UCA0TXBUF=outputString[i];
		}
	}//end if numberOfHits >=20

}//end timerOverflow_handlaer

//Register the timer
ISR_VECTOR(timerOverflow_handler,".int10")

