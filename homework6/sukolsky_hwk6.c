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
|	4/12: Initial build
|================================================================================
| *NOTES:
\*******************************************************************************/

#include "msp430g2553.h"


void initUart();
void initWDT();
void initADC();

void main()
{
	//Set watchdog off and make correct smclk
	WDTCTL = WDTPW + WDTHOLD; 		// Halt the watchdog timer
	BCSCTL1 = CALBC1_1MHZ;   		// 1MHz SMCLK bits.
	DCOCTL  = CALDCO_1MHZ;
	
	initUart();
	initADC();
	initWDT();
  	
	__bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, interrupts enabled
}


void initUart(){
	P1SEL = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
	P1SEL2 = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
	UCA0CTL1 |= UCSSEL_2;                     // SMCLK
	UCA0BR0 = 104;                            // 1MHz 9600
	UCA0BR1 = 0;                              // 1MHz 9600
	UCA0MCTL = UCBRS0;                        // Modulation UCBRSx = 1
	UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
	IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt
}

void initWDT(){
	// setup the watchdog timer as an interval timer
  	WDTCTL =(WDTPW +		// (bits 15-8) password
     	                   	// bit 7=0 => watchdog timer on
       	                 	// bit 6=0 => NMI on rising edge (not used here)
                        	// bit 5=0 => RST/NMI pin does a reset (not used here)
           	 WDTTMSEL +     // (bit 4) select interval timer mode
  		 WDTCNTCL  		// (bit 3) clear watchdog timer counter
  		                	// bit 2=0 => SMCLK is the source
  		                	// bits 1-0 = 00 => source/32K
 			 );
     	IE1 |= WDTIE;			// enable the WDT interrupt (in the system interrupt register IE1)
}//end initWDT

void initADC(){
	ADC10CTL1= ADC_INCH	//input channel 4
 			  +SHS_0 //use ADC10SC bit to trigger sampling
 			  +ADC10DIV_4 // ADC10 clock/5
 			  +ADC10SSEL_0 // Clock Source=ADC10OSC
 			  +CONSEQ_0; // single channel, single conversion
 			  ;
	ADC10AE0=ADC_INPUT_BIT_MASK; // enable A4 analog input
	ADC10DTC1=1;   // one block per transfer
	ADC10CTL0= SREF_0	//reference voltages are Vss and Vcc
 	          +ADC10SHT_3 //64 ADC10 Clocks for sample and hold time (slowest)
 	          +ADC10ON	//turn on ADC10
 	          +ENC		//enable (but not yet start) conversions
 	          ;		  
}//end initADC

interrupt void timerOverflow_handler(){
	unsigned int adcReading;
	char outputString[10];	
	//Take an ADC

	//Change the data to something readable
	double actualADC=(3.3/1024)*adcReading;
	dtostrf(actualADC,5,2,outputString);	//converts float/double to a string
	//Send data to PC
	int i=0;
	for (i=0; i<strlen(outputString);i++){
		while (!(IFG2&UCA0TXIFG));
		UCA0TXBUF=outputString[i];
	}
}

