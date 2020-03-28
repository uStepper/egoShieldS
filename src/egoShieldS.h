/********************************************************************************************
* 	 	File: 		egoShieldTimeLapse.h													*
*		Version:    1.1.0                                           						*
*      	Date: 		March 17th, 2018	                                    				*
*      	Author: 	Mogens Groth Nicolaisen                                					*
*                                                   										*	
*********************************************************************************************
* (C) 2018                                                                                  *
*                                                                                           *
* uStepper ApS                                                                              *
* www.ustepper.com                                                                          *
* administration@ustepper.com                                                               *
*                                                                                           *
* The code contained in this file is released under the following open source license:      *
*                                                                                           *
*     Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International               *
*                                                                                           *
*   The code in this file is provided without warranty of any kind - use at own risk!       *
*   neither uStepper ApS nor the author, can be held responsible for any damage             *
*   caused by the use of the code contained in this file !                                  *
*                                                                                           *
********************************************************************************************/
/**
*	\mainpage Arduino library for the uStepper egoShield shield
*	
*	This is the egoShield Arduino library, providing software functions for the uStepper egoShield shield.
*	
*	\par News!
*
*	\par Features
*	The egoShield library contains the following features:
*	
*	\image html functional.png
*	
*	- State is indicated at the top left corner of the OLED display
*	- PID mode is indicated in the top middle of the OLED display
*	- Loop mode is indicated in the top right corner of the OLED display
*	- The button functionalities are indicated by a bar in the bottom of the OLED display
* 
*	\par Installation
*	To install the egoShield library into the Arduino IDE, perform the following steps:
*
*	- Go to Sketch->Include Libraries->Manage Libraries... in the Arduino IDE
*	- Search for "egoShield", in the top right corner of the "Library Manager" window
*	- Install egoShield library 
*	
*	The library is tested with Arduino IDE 1.8.5
*
*	\par Prerequisites
*	The library requires the uStepper library:
*	<a rel="license" href="https://github.com/uStepper/uStepper">uStepper GitHub</a>
*	and the u8g2 library from Olikraus
*	<a rel="license" href="https://github.com/olikraus/u8g2">u8g2 GitHub</a>
*
*	\par Copyright
*
*	(C)2018 uStepper ApS
*																	
*	www.uStepper.com 																	
*
*	administration@uStepper.com 														
*																							
*	<img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by-nc-sa/4.0/88x31.png" />																
*
*	The code contained in this file is released under a <a rel="license" href="http://creativecommons.org/licenses/by-nc-sa/4.0/">Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License</a>	
*																							
*	The code in this library is provided without warranty of any kind - use at own risk!		
* 	neither uStepper ApS nor the author, can be held responsible for any damage		
* 	caused by the use of the code contained in this library ! 	
*
*	\par To do list
*	- Add comments in the .cpp
*	- clean the code in .cpp
*	- Add brakeFlag toggle (remove it from setup function)
*
*	\par Known Bugs
*	- No known bugs
*
*	\author Mogens Groth Nicolaisen (mogens@ustepper.com)
*	\par Change Log
*	\version 1.1.0:
* 	- changed order of events to move, fire shutter, delay
*	- added an adjustable delay parameter - delaying the camera shutter after end of move
*	- idle page encoder value is now displayed in mm
*
*	\version 1.0.0:
* 	- changed button debouncing algorithm
*	- fixed various minor bugs
*	- finished functionality
*
*	\version 0.1.1:	
*	- Corrected release version in library properties
*	
*	\version 0.1.0:	
*	- Initial release
*	
*
*/

/**
 * @file egoShieldTimeLapse.h
 * @brief      Function prototypes and definitions for the egoShield library
 *
 *             This file contains class and function prototypes for the library,
 *             as well as necessary constants and global variables.
 *
 * @author     Mogens Groth Nicolaisen (mogens@ustepper.com)
 */

#ifndef egoShield_h
#define egoShield_h


#ifdef ARDUINO_AVR_USTEPPER_S
	#include "uStepperS.h"
#endif

#ifdef ARDUINO_AVR_USTEPPER_S_LITE
	#include "uStepperSLite.h"
#endif

#include "Arduino.h"
#include <avr/pgmspace.h>
#include "screen.h"
#define NOPAGE 0
#define RECORDPAGE 1
#define IDLEPAGE 2
#define PLAYPAGE 3
#define PAUSEPAGE 4
#define TIMEPAGE 5

#define DRAWPAGE(x) 	do\
					    {\
					      if(this->screen->busFailure)\
					      {\
					      	cli();\
					        this->lastPage = NOPAGE;\
					        this->screen->init();\
					        x;\
					      	sei();\
					      }\
					      else\
					      {\
					      	x;\
					      }\
					    }\
					    while(this->screen->busFailure);\

/** Forward button pin definition*/
#define BWBT 5
 /** Play button pin definition*/
#define PLBT 4
 /** Record button pin definition*/
#define RECBT 3
 /** Backward button pin definition*/
#define FWBT 6
 /** Max record count definition*/
#define CNT 50
 /** Optocoupler pin definition*/
#define OPTO 2

/** Full step definition*/
#define FULL 1							
/** Half step definition*/
#define HALF 2							
/** Quarter step definition*/
#define QUARTER 4						
/** Eighth step definition*/
#define EIGHT 8							
/** Sixteenth step definition*/	
#define SIXTEEN 16			

/**
 * @brief struct to hold information required to debounce button.
 */
typedef struct {
	uint8_t debounce,			/**< Variable to hold current and past samples of the button GPIO pin*/
	state,						/**< Current debounce state of the button*/
	holdCnt,					/**< Counter to determined when the button should go from PRESSED to HOLD*/
	btn;						/**< Variable to indicate a button press or release*/
	uint16_t time;				/**< Counter to decide when a new button press should be given in the case of a HELD button*/
} buttons;

#define DEPRESSED 0				/** Definition of DEPRESSED button state */
#define PRESSED 1				/** Definition of PRESSED button state */
#define HOLD 2					/** Definition of HOLD button state */

#define HOLDTIME 50				/** Number of PRESSED samples before the button should be considered HOLD */
#define HOLDTICK 4				/** Number of HOLD samples before a new button press should be issued */

/** Macro for resetting watchdog timer */
#define RESETWDT asm volatile("WDR \n\t")

/** 
*
*	@brief Watchdog timer interrupt handler, for examining the buttons periodically
*
*	The Watchdog is configured to interrupt once every 16ms, to examine the IO state of the buttons, and performing the debouncing.
*	The debouncing algorithm looks at the last five measured IO states of each button individually, to determine whether a button has finished bouncing or not.
*	In order to see if the button is held or just pressed, a counter (seperate for each button) is incremented every time all the last five measurements are identical
*	and if this counter reaches the value "HOLDTIME", the button are considered held. If any IO measurement is different from the last one, the counter is reset.
*/
extern "C" void TIMER4_COMPA_vect(void) __attribute__ ((signal,used));

class egoShield
{
public:
	/**
	* @brief      Constructor of egoShield class
	*
	*             This is the constructor of the egoShield class. No arguments are present in the constructor.
	*/
	egoShield(void);
	/**
	* @brief      	Initializes buttons, OLED, uStepper and BT-module.
	*
	* @param[in]  	acc takes in the maximum acceleration in play mode.
	*
	* @param[in]  	vel takes in the maximum velocity in play mode.
	*
	* @param[in]  	uStep takes in the microstepping setting.
	*
	* @param[in]  	fTol takes in the fault tolerance for the PID in steps, i.e. how much error is allowed before correction.
	*
	* @param[in]  	fHys takes fault hysteresis in steps, i.e. when is the PID deactivated again.
	*
	* @param[in]  	P takes in the PID P term.
	*
	* @param[in]  	I takes in the PID I term.
	*
	* @param[in]  	D takes in the PID D term.
	*
	* @param[in]  	res takes in the resolution of the drive in deg/mm.
	*
	* @param[in]  	shutterDelay in milliseconds between motor stopped and shutter fires.
	*/
	void egoShield::setup(uint16_t acc = 1500, 
						  uint16_t vel = 100,
						  float P = 1.0, 
						  float I = 0.02, 
						  float D = 0.006,
						  float res = 1,
						  int8_t stallsens = 2,
						  uint16_t shutterDelay = 250);
	/**
	* @brief      	Contains the main logic of the shield functionality, e.g. transition between states (idle, play, record and pause).
	*/	
	//void loop(void);
	/**
	* @brief      	Creates an uStepper instance.
	*/
	#ifdef ARDUINO_AVR_USTEPPER_S
	  uStepperS stepper;
	#endif

	#ifdef ARDUINO_AVR_USTEPPER_S_LITE
	  uStepperSLite stepper;
	#endif
	
protected:
	Screen *screen;
	/**Creates an SoftwareSerial instance for BT-module */
	//SoftwareSerial *BTSerial;
	uint8_t lastPage = NOPAGE;
	/** This variable holds the step number in the recorded sequence */
	uint8_t place;
	/** This variable holds the final step number in the recorded sequence */
	uint8_t endmove;
	/** This array holds the encoder value at the recorded positions */
	float pos[CNT];
	/** This variable indicates if PID is enabled */
	bool pidFlag;
	/** This variable indicates whether we are recording */
	bool record;
	/** This variable indicates whether we are in loop mode during playback */
	bool loopMode;
	/** This array indicates whether one of the buttons have experienced a long duration push */
	bool longPushFlag[4];
	/** This variable holds the current state of the program, which tells whether the program is in idle, play, record or pause mode */
	char state;
	/** This variable holds the current state of the record button, which tells whether no, short or long push has been detected */
	uint8_t rec;
	/** This variable holds the current state of the play button, which tells whether no, short or long push has been detected */
	uint8_t play;
	/** This variable holds the current state of the forward button, which tells whether no, short or long push has been detected */
	uint8_t fw;
	/** This variable holds the current state of the backward button, which tells whether no, short or long push has been detected */
	uint8_t bw;
	/** This variable holds the current set-point to the PID, either from manual control or during playback of the sequence */
	float setPoint;
	/** This variable holds the acceleration used during playback of the sequence */
	uint16_t acceleration;
	/** This variable holds the velocity used during playback of the sequence */
	uint16_t velocity;
	/** This variable holds the microstepping setting */
	uint8_t microStepping; 
	/** This variable holds the fault tolerance setting */
	uint16_t faultTolerance;
	/** This variable holds the fault hysteresis setting */
	uint16_t faultHysteresis; 
	/** This variable holds the PID P term */
	float pTerm; 
	/** This variable holds the PID I term */
	float iTerm; 
	/** This variable holds the PID D term */
	float dTerm;
	/** This variable holds the stepSize for timelapse */
	float stepSize;
	/** This variable holds the interval for timelapse */
	uint16_t interval;
	/** This variable holds the resolution deg/mm */
	float resolution;
	/** This variable holds the stall sensitivity level */
	int8_t stallSensitivity;
	/** This variable holds the brake flag */
	bool brakeFlag;
	/* Declaration of structs for each button*/
	volatile buttons   	forwardBtn    = {0x1F, DEPRESSED, 0, 0, 0},
          				playBtn       = {0x1F, DEPRESSED, 0, 0, 0},
          				recordBtn     = {0x1F, DEPRESSED, 0, 0, 0},
          				backwardsBtn  = {0x1F, DEPRESSED, 0, 0, 0};
	/** This variable holds the value of the delay between motor stops and the shutter fires */
    uint16_t shutterDelay;

    unsigned char* loadVideoBuffer(unsigned char *data, unsigned char length);
    /**
	* @brief      	Function for resetting the state of a button seperately
	*
	* @param[in]  	btn is a Pointer to the button struct variable needed to be reset 
	*/
	void resetButton(buttons *btn);
	/**
	* @brief      	Resets the state of all 4 buttons at once
	*/
	void resetAllButton();
	/**
	* @brief      	Reads the four buttons and writes their value; no push, short push or long push, to global variables.
	*/
	void inputs(void);
	/**
	* @brief      	Returns the button state of the appropriate button.
	*
	* @param[in]  	button is set to either of the four available buttons.
	*
	* @param[in]  	nmbr is used for indexing in the longPushFlag array.
	*
	* @return     0	- no push detected.
	* @return     1	- short push detected.
	* @return     2	- long push detected.
	*
	*/
	uint8_t buttonState(uint8_t button, uint8_t nmbr);
	/**
	* @brief      	Holds the idle logic; page to show, what buttons to enable etc.
	*/
	void idleMode(void);
	/**
	* @brief      	Holds the play logic, showing play page and running the recorded sequence.
	*/
	void playMode(void);
	/**
	* @brief      	Holds the pause logic, showing the pause page and pausing the playing of a sequence.
	*/
	void pauseMode(void);
	/**
	* @brief      	Holds the timelapse logic, showing the timelapse page.
	*/
	void timeMode(void);
	/**
	* @brief      	Holds the manual forward logic for driving the stepper motor manually with the pushbuttons.
	*/
	void manForward(void);
	/**
	* @brief      	Holds the manual backward logic for driving the stepper motor manually with the pushbuttons.
	*/
	void manBackward(void);
	/**
	* @brief      	Holds the code for the start page of the OLED.
	*/
	void startPage(void);
	/**
	* @brief      	Holds the code for the idle page of the OLED.
	*
	* @param[in]  	pidMode tells if the display should show PID ON or PID OFF.
	*
	* @param[in]  	pos is the encoder position to be displayed.
	*/
	void idlePage(bool pidMode, float pos);
	/**
	* @brief      	Holds the code for the record page of the OLED.
	*
	* @param[in]  	pidMode tells if the display should show PID ON or PID OFF.
	* 
	* @param[in]  	recorded tells if a step has been recorded.
	*
	* @param[in]  	index tells which step we are at.
	*
	* @param[in]  	pos is the encoder position to be displayed.
	*/
	void recordPage(bool pidMode, bool recorded, uint8_t index, float pos);
	/**
	* @brief      	Holds the code for the play page of the OLED.
	*
	* @param[in]  	loopMode tells if the display should show loop symbol.
	*
	* @param[in]  	pidMode tells if the display should show PID ON or PID OFF.
	*
	* @param[in]  	index tells which step we are at.
	*/
	void playPage(bool loopMode, bool pidMode, uint8_t index, bool mode);
	/**
	* @brief      	Holds the code for the pause page of the OLED.
	*
	* @param[in]  	loopMode tells if the display should show loop symbol.
	*
	* @param[in]  	pidMode tells if the display should show PID ON or PID OFF.
	*
	* @param[in]  	index tells which step we are at.
	*/
	void pausePage(bool loopMode, bool pidMode, uint8_t index);
	/**
	* @brief      	Holds the code for the timelapse page of the OLED.
	*
	* @param[in]  	pidMode tells if the display should show PID ON or PID OFF.
	*
	* @param[in]  	step tells which step we are at in the timelapse mode.
	*/
	void timePage(uint8_t step, bool pidMode);
	/**
	* @brief      	Holds the code for the changing velocity during sequence play.
	*/
	void changeVelocity(void);
	/**
	* @brief 		This function handles the debouncing and tracking of whether buttons are pressed, released or held
	*
	* @param[in] 	btn is a pointer to the struct off the button currently being examined
	*
	* @param[in] 	sample is a representation of the current button IO state
	*/
	void recordMode(void);
	void debounce(buttons *btn, uint8_t sample);
	friend void TIMER4_COMPA_vect(void) __attribute__ ((signal,used));
};

class egoShieldTimeLapse : public egoShield
{
public:
	void loop(void);
};

class egoShieldTeach : public egoShield
{
public:
	void loop(void);
};
#endif