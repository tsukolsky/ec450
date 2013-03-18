/*******************************************************************************\
| main.c
| Author: Todd Sukolsky
| ID: U50387016
| Initial Build: 3/18/2013
| Last Revised: 3/18/2013
| Copyright of Todd Sukolsky
|================================================================================
| Description: This program is the main program for homework 4 of BU's ECE450.
|  	The assignment require four tones to be played over a small speaker, that a button
|	can manipulate (turn on, off, raise tone) of the notes, and one advanced feature.
|	THe advanced feature, on initial build, is still TBD. See later "Revisions" and
|	"Notes" for what was actually implemented.
|--------------------------------------------------------------------------------
| Revisions: 3/18- Initial Build. Original setup is just to have it play C tone
|				   to mess around with. Next revision will have it do different things.
|
|================================================================================
| *NOTES: Tone Frequencies: C=261.63, D=293.66, E=329.63, F=349.23, G=392.00, A=440
|							B=493.88...That C is the middle C
\*******************************************************************************/

#include "msp430g2553.h"

//Define Button, Pin, LEDs
#define RED_LED (1 << 0) 		//0x01
#define GREEN_LED (1 << 6)		//0x40
#define BUTTON_BIT (1 << 3) 		//0x08
#define TA0_BIT (1 << 1)		//0x02 => Bit mask corresponding to TA0

//Define Tone Half Frequencies
#define C_tone 261/2
#define D_tone 293/2
#define E_tone 330/2
#define F_tone 350/2
#define G_tone 392/2
#define A_tone 440/2
#define B_tone 494/2

/*==============================================*/
/*				Global Variables				*/
/*==============================================*/
volatile unsigned int halfPeriod=500;	//1kHz tone.

/*==============================================*/
/*			Forward Function Declarations		*/
/*==============================================*/
void initTimerA();
void initButtonAndLEDS();

/*==============================================*/
/*				   Main Program					*/
/*==============================================*/

void main(void){
	WDTCTL = WDTPW + WDTHOLD;	//shut down watchdog timer
	BCSCTL1 = CALBC1_1MHZ;		//1MHz calibration for clock
	DCOCTL = CALDCO_1MHZ;		//"

	//Initialize timers and buttons
	initTimerA();
	initButtonAndLEDS();

	//Enable global interrupts
	_bis_SR_register(GIE+LPM0_bits);
}


/*==============================================*/
/*				Functions						*/
/*==============================================*/

void initTimerA(){
	TA0CTL |= TACLR; 	//reset clock
	TA0CTL = TASSEL1+ID_0+MC_2;		//clock source is SMCLK, clk divider =1, continuous mode
	TA0CCTL0 = CCIE;				//turn on output compare interrupt
	TA0CCR0 = C_tone-1;				//when output is triggered.
	
	//Connect Timer output to pin TA0
	P1SEL |= TA0_BIT;
	P1DIR |= TA0_BIT;
}

/*------------------------------------------*/

void initButtonAndLEDS(){
    //Initialize LEDs port
    P1DIR |= (RED_LED + GREEN_LED);
    P1OUT |= GREEN_LED;					//set green high initially
    P1OUT &= ~RED_LED;					//set red low initially

    //Initialize Button
    P1OUT |= BUTTON_BIT;			//Declare a pull up
    P1REN |= BUTTON_BIT;			//Enable PUll up
    P1IES |= BUTTON_BIT;			//set to trigger on falling edge
    P1IFG &= ~BUTTON_BIT;			//Make sure flag is cleared initially
    P1IE |= BUTTON_BIT;				//Enable interrupt for button
}

/*==========================================*/
/*				  ISR's						*/
/*==========================================*/

void interrupt buttonISR(){
	//Debounce variable
	volatile static unsigned int debounceCounter=0;

	//Make sure we should actually be doing something. ButtonBIt should be low and IFG should be set.
	if (debounceCounter < 5){debounceCounter++;}
	else {
		if (P1IFG & BUTTON_BIT){
			P1IFG &= ~BUTTON_BIT;	//lower flag
			TACCTL0 ^= OUTMOD_4;	//toggle OUTMOD between 0 and 4
			debounceCounter = 0;
		} else; //do nothing
	}//end else debounceCounter
}//end buttonISR handler.

/*------------------------------------------*/

//Timer compare match. Just toggle LEDS.
void interrupt timerAISR(){
	P1OUT ^= RED_LED + GREEN_LED;
}

//Declare interrupt vectors.
ISR_VECTOR(buttonISR,".int02")
ISR_VECTOR(timerAISR,".int09")