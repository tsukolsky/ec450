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
|			 3/18(2)- Finished module. now it has the full beginning melody of "Imperial March"
|					  with almost all the right notes. There is an error in a flat or sharp,
|					  should be G major but something is off, I think C# ending? Can't figure it out.
|					  Added "Special Feature". User can press button longer or shorter to speed up
|					  tones. Longer it's left wihtout second press, quicker it goes. Three stages.
|
|================================================================================
| *NOTES: Tone Frequencies: C=261.63, D=293.66, E=329.63, F=349.23, G=392.00, A=440
|							B=493.88...That C is the middle C
|		  Imperial March starts with: C#-C#-C#-A-E-C#-A-E-C#, G#-G#-G#-B#-G#-C_low-A-E-C,
\*******************************************************************************/

#include "msp430g2553.h"

//Define Button, Pin, LEDs
#define RED_LED (1 << 0) 		//0x01
#define GREEN_LED (1 << 6)		//0x40
#define BUTTON_BIT (1 << 3) 		//0x08
#define TA0_BIT (1 << 1)		//0x02 => Bit mask corresponding to TA0

//Define Tone Half Frequencies
#define C_tone_low 261
#define Csh_tone_low 277
#define D_tone 293
#define E_tone 330
#define F_tone 350
#define G_tone 392
#define Gsh_tone 415
#define A_tone 220
#define B_tone 494
#define C_tone_high 523//same as B#
#

//Define Oscillator frequency
#define FOSC 1000000

//Define wait times
#define STATE_1_TIME	.5
#define STATE_2_TIME	.6
#define STATE_3_TIME	.3
#define STATE_4_TIME	.4
#define STATE_5_TIME	.175
#define PAUSE_TIME		.15

//Define bools
#define fTrue	 1
#define fFalse	 0

//Define MACROS
#define __resetTimeWaited() timeWaited=0;

/*==============================================*/
/*				Global Variables				*/
/*==============================================*/
unsigned int flagPlaySong=fFalse,flagWaitingForPress=fFalse;
float timeWaited=0,timeSincePress=0;
unsigned int repeatedToneReps=0, state=0, lastTone=0, speedDivisor=1;
/*==============================================*/
/*			Forward Function Declarations		*/
/*==============================================*/
void initTimerA();
void initButtonAndLEDS();
void playSong();

/*==============================================*/
/*				   Main Program					*/
/*==============================================*/

void main(void){
	//Set watchdog and enable it's interrupt
	WDTCTL = (WDTPW + WDTTMSEL + WDTCNTCL + 1);	//Boots off 1MHz clock, div 9192 gives interrupt every ~8.2 ms
    IE1 |= WDTIE;

    //Set timing
	BCSCTL1 = CALBC1_1MHZ;		//1MHz calibration for clock
	DCOCTL = CALDCO_1MHZ;		//"

	//Initialize timers, buttons, LEDs
	initButtonAndLEDS();
	initTimerA();

	//Enable global interrupts
	_bis_SR_register(GIE);

	//Processing Loop
	while(fTrue){
		if (flagPlaySong){; playSong(); flagPlaySong=fFalse;}	//Play song then reset flag.
		else;
	}
}


/*==============================================*/
/*				Functions						*/
/*==============================================*/

void initTimerA(){
	TA0CTL |= TACLR; 	//reset clock
	TA0CTL = TASSEL1+ID_0+MC_1;					//clock source is SMCLK, clk divider =1, up mode
	TA0CCTL0 = CCIE;							//turn on output compare interrupt, initially OUTMOD
	TA0CCR0 = FOSC/C_tone_low-1;					//when output is triggered. TO make C_tone, eq => 261Hz= 1MHz/(x) ==> x=1MHz/(2*C_tone)
	
	//Connect Timer output to pin TA0
	P1SEL |= TA0_BIT;
	P1DIR |= TA0_BIT;
}

/*------------------------------------------*/

void playSong(){
	unsigned int whichRun=0;
	unsigned int playingSong=fTrue;
	state=0;
	lastTone=0;

	while (playingSong){
	//State machine
	switch (state){
	//Initialization state
	case 0:{
		TACCTL0 ^= OUTMOD_4;		//turn sound on, move to state 1
		timeWaited=0;
		repeatedToneReps=0;
		state=1;
		break;
	}
	//Triple tone to play, beginning
	case 1: {
		//If we haven't played the beginning tone three times, play the tone for the correct amount of time
		//Haven't played three times, if timeWaited < time to play then keep playing the tone, stay in same state.
		TA0CCR0=FOSC/Csh_tone_low-1;
		while (timeWaited < STATE_1_TIME/speedDivisor);
		timeWaited=0; 				//reset time waited
		lastTone=state; 			//set where we were to this state
		state=15;					//go to pause state
		//We played the tone enough times, Set the reps to 0, timeWaited reset, then go to pause state.
		if (repeatedToneReps < 4){
			repeatedToneReps++;
		}
		break;
	}

	case 2: {
		//Playing the middle C, if the timeWaited so far is less than tone time, play the tone
		TACCR0=FOSC/A_tone-1;
		while (timeWaited < STATE_2_TIME/speedDivisor);
		//We played the tone, go to pause state while resetting timeWaited.
		timeWaited=0; lastTone=state; state=15;		//Go to pause
		if (whichRun==1){whichRun++;}
		break;
	}
	case 3:{
		//Playing the 5th note or 8th. If timeWaited is less than tone time, keep playhing the tone
		TACCR0=FOSC/E_tone-1;
		while (timeWaited < STATE_3_TIME/speedDivisor);
		//Done playing tone? Reset time waited and go to puase state.
		timeWaited=0;lastTone=state; state=15;
		break;
	}
	case 4:{
		//Playing the 6th note or 9th. If timeWaited is less than tone time, keep playhing the tone
		TACCR0=FOSC/Csh_tone_low-1;
		while (timeWaited < STATE_4_TIME/speedDivisor);
		//Done playing tone? Reset time waited and go to pause state
		timeWaited=0;lastTone=state; state=15; whichRun++;
		break;
	}
	case 5:{
		TACCR0=FOSC/Gsh_tone-1;
		while (timeWaited < STATE_1_TIME/speedDivisor);
		timeWaited=0; lastTone=state; state=15;
		break;
	}
	case 6:{
		TACCR0=FOSC/Gsh_tone-1;
		while (timeWaited < STATE_1_TIME/speedDivisor);
		timeWaited=0; lastTone=state; state=15;
		break;
	}
	case 7:{
		TACCR0=FOSC/Gsh_tone-1;
		while (timeWaited < STATE_1_TIME/speedDivisor);
		timeWaited=0; lastTone=state; state=15;
		break;
	}
	case 8:{
		TACCR0=FOSC/C_tone_high-1;
		while (timeWaited < STATE_2_TIME/speedDivisor);
		timeWaited=0; lastTone=state; state=15;
		break;
	}
	case 9:{
		TACCR0=FOSC/Gsh_tone-1;
		while (timeWaited < STATE_3_TIME/speedDivisor);
		timeWaited=0; lastTone=state; state=15;
		break;
	}
	case 10:{
		TACCR0=FOSC/C_tone_low-1;
		while (timeWaited < STATE_4_TIME/speedDivisor);
		timeWaited=0; lastTone=state; state=15;
		break;
	}
	case 11:{
		TACCR0=FOSC/A_tone-1;
		while (timeWaited < STATE_2_TIME/speedDivisor);
		timeWaited=0; lastTone=state; state=15;
		break;
	}
	case 12:{
		TACCR0=FOSC/E_tone-1;
		while (timeWaited < STATE_3_TIME/speedDivisor);
		timeWaited=0; lastTone=state; state=15;
		break;
	}
	case 13:{
		TACCR0=FOSC/C_tone_low-1;
		while (timeWaited < STATE_4_TIME/speedDivisor);
		timeWaited=0; lastTone=state; state=15;
		break;
	}
	case 14:{
		//Get out of this sucker.
		TACCTL0=CCIE;	//no more sound.
		playingSong=fFalse;		//break out of the while loop, done playing the song.
		break;
	}
	case 15:{
		//IF timeWaited is less than pause time, keep pausing without sound
		if (timeWaited < PAUSE_TIME/speedDivisor){
			//Don't output sound.
			TACCTL0 = CCIE;
			while (timeWaited <= PAUSE_TIME/speedDivisor);	//Do nothing
		} else;

		//Going back into playing tones
		//We are going to go back to a tone, figure out whcih one then reset timeWaited
		if (lastTone < 14){
			if (repeatedToneReps >= 3 && whichRun<1){
				state=lastTone+1;									//should move to state 2
			}else if(repeatedToneReps >= 3 && whichRun == 1){
				state=2;										//repeat the C-F-D phrase
			}else if (repeatedToneReps < 3){state=lastTone;}		//repeat the first tone three times
			else if (repeatedToneReps >= 3 && whichRun >=1){state=lastTone+1;}
			else;
			TACCTL0 = CCIE + OUTMOD_4;	//turn sound back on
		}else {state=0;}
		timeWaited=0;
		break;
	}//end else timeWaited < PAUSE_TIME
	default:{state=0; whichRun=0;break;}
	}//end switch
	}//end while
}//end function Play song
/*------------------------------------------*/

void initButtonAndLEDS(){
    //Initialize LEDs port
    P1DIR |= (RED_LED + GREEN_LED);
    P1OUT |= GREEN_LED;					//set green high initially
    P1OUT &= ~RED_LED;					//set red low initially

    //Initialize Button
    P1OUT |= BUTTON_BIT;				//Declare a pull up
    P1REN |= BUTTON_BIT;				//Enable PUll up
    P1IES |= BUTTON_BIT;				//set to trigger on falling edge
    P1IFG &= ~BUTTON_BIT;				//Make sure flag is cleared initially
    P1IE |= BUTTON_BIT;					//Enable interrupt for button
}

/*==========================================*/
/*				  ISR's						*/
/*==========================================*/

void interrupt buttonISR(){
	if (P1IFG&BUTTON_BIT){
		P1IFG &= ~BUTTON_BIT;	//lower flag
		if (flagWaitingForPress){
			flagWaitingForPress=fFalse;
			if (timeSincePress < 1){speedDivisor=1;}
			else if (timeSincePress >=1 && timeSincePress <=3){speedDivisor=3;}
			else {speedDivisor=5;}
			flagPlaySong=fTrue;
			timeSincePress=0;
			P1OUT &= ~RED_LED;
		} else {
			flagWaitingForPress=fTrue;
			P1OUT |= RED_LED;
		}
	} else; //do nothing, not correct occasion

}//end buttonISR handler.

/*------------------------------------------*/

//Timer compare match. Just toggle LEDS.
void interrupt timerAISR(){
	P1OUT ^= GREEN_LED;			//Toggle for our own benefit
}

/*------------------------------------------*/

void interrupt WDT_interval_handler(){
	if (flagPlaySong){timeWaited+=.008;}		//If overflow and we are playing the song, add amount of time that has gone by.
	else if (flagWaitingForPress){timeSincePress+=.008;}
	else;
}

/*------------------------------------------*/
//Declare interrupt vectors.
ISR_VECTOR(buttonISR,".int02")
ISR_VECTOR(timerAISR,".int09")
ISR_VECTOR(WDT_interval_handler,".int10")
