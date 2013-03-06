/*******************************************************************************\
| main.c
| Author: Todd Sukolsky
| ID: U50387016
| Initial Build: 2/19/2013
| Last Revised: 2/19/2013
| Copyright of Todd Sukolsky
|================================================================================
| Description: This is the main.c file for homework 3 of EC450, uC. The module
|			   	waits for the user to press the button on pin 1.3 and then begins
|				a reaction timer. It will flash the green light twice, then the red
|				light once. From there it will time how long it takes the user to press
|				the button again and store it in a global variable.
|================================================================================
| Revisions: 2/19-Initial Build. Finished, See Note 1 for information on possible
|				  revision.
|
|================================================================================
| *NOTES: (1)This module/implentation does not protect against LONG BUTTON PRESSES.
|			A more refined version will require the user to RELEASE THE BUTTON
|			before the state is reset from the waiting state (state=4) as well
|			as error checking protocol in state 0,1 and 2 to make sure the timer
|			is not started in state 2 until the button is released.
\*******************************************************************************/
#include "msp430g2553.h"

#define RED (1 << 0) 		//0x01
#define GREEN (1 << 6)		//0x40
#define BUTTON (1 << 3) 	//0x08

#define BLINK_TIME 1000		//should make something around 2Hz

/************************************/
/*			Global Variables        */
/************************************/
float reactionTime=0;

/*==============================================Begin Main Program=======================================================*/

void main(void) {
    WDTCTL = (WDTPW + WDTTMSEL + WDTCNTCL + 2);	//Boots off 1MHz clock, div 512 gives interrupt every .512 ms
    //Enable interrupt for watchdog
    IE1 |= WDTIE;

    //Initialize I/0 port
    P1DIR = (RED + GREEN);
    P1REN = BUTTON;
    P1OUT = BUTTON;

    _bis_SR_register(GIE+LPM0_bits);  //Enable global interrupts; LPM0_bits is low power bit, sleep cycle.
}


interrupt void WDT_interval_handler(){
	//Initialize variables
	static int state=0;									//what state the machine is in
	static long int timeBlinked=BLINK_TIME;				//blink interval, should be about 1/2 hz
	static int lastButtonState=0;						//last button state
	static int repetitions=0;							//way to count how many times an LED has been toggled or waiting period has gone by

	int currentButton=(P1IN & BUTTON); //read the current button state

	//Moore Machine
	switch(state){
		//Case 0. Turn on green led, move to state 1
		case 0: {P1OUT |= GREEN; state=1;break;}		//just turn the RED LED on
		//Case 1. Blink the green led twice, then turn it off and turn red led on moving to state 2
		case 1:	{
				if (--timeBlinked <= 0){ 				//Done with the blink, need to toggle this sucker.
					P1OUT ^= GREEN;						//toggle LED, then wait. Does this four times
					timeBlinked = BLINK_TIME;
					repetitions++;
				}
				if (repetitions >= 4){state=2; repetitions=0; P1OUT &= ~GREEN; P1OUT |= RED;}	//done with blinking, turn on the red LED which is next.
				break;
		}
		//Case 1. green LED blinks are done. Blink the red LED and when done, start timer.
		case 2:{
				if (--timeBlinked <= 0){
					P1OUT &= ~RED;						//turn green LED off
					state=3;
					reactionTime=0;
				}
				break;
		}
		//Case 2. Waiting for button press. If pressed, go to state 3?
		case 3: {if (!currentButton && lastButtonState){	//button was just pressed
					//Do something with reactionTime.
					reactionTime+=.512;
					state=4;	//waiting state
				 } else {	//doesn't matter whats going on, increment the counter of how long we have waited.
					 reactionTime+=.512;	//adds .512ms to the timer
				 }
				 break;
		}
		//Random wait case. allows time to view reaction time in debugger, about two seconds
		case 4: {if (--timeBlinked <= 0){repetitions++; timeBlinked=BLINK_TIME;}
				 if (repetitions >= 4){state=0; repetitions=0;}
				 break;	//just wait with everything off for what should be 2 second.
		}
		default: {state=0; break;}
	}//end switch

	//Remember current button state
   	lastButtonState = currentButton;
}

//Declare interrupt vector for watchdog.
ISR_VECTOR(WDT_interval_handler,".int10")
