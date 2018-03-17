/********************************************************************************************
* 	 	File: 		egoShieldTimeLapse.h													*
*		Version:    1.0.0                                           						*
*      	Date: 		January 10th, 2018	                                    				*
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
*	\version 1.0.1:
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

#include "uStepper.h"
#include "U8g2lib.h"
#include "SPI.h"
#include "Arduino.h"
#include "SoftwareSerial.h"

/** Forward button pin definition*/
#define FWBT A3
 /** Play button pin definition*/
#define PLBT A1
 /** Record button pin definition*/
#define RECBT A2
 /** Backward button pin definition*/
#define BWBT A0
 /** Max record count definition*/
#define CNT 50
 /** Optocoupler pin definition*/
#define OPTO 3

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

#define en_width 11
#define en_height 9
static unsigned char fw_bits[] = {
   0x41, 0x00, 0xc3, 0x00, 0xc7, 0x01, 0xcf, 0x03, 0xdf, 0x07, 0xcf, 0x03,
   0xc7, 0x01, 0xc3, 0x00, 0x41, 0x00 };
static unsigned char bw_bits[] = {
   0x10, 0x04, 0x18, 0x06, 0x1c, 0x07, 0x9e, 0x07, 0xdf, 0x07, 0x9e, 0x07,
   0x1c, 0x07, 0x18, 0x06, 0x10, 0x04 };
#define tt_width 10
#define tt_height 10
static unsigned char rec_bits[] = {
   0xfc, 0x00, 0xfe, 0x01, 0xff, 0x03, 0xff, 0x03, 0xff, 0x03, 0xff, 0x03,
   0xff, 0x03, 0xff, 0x03, 0xfe, 0x01, 0xfc, 0x00 };
static unsigned char stop_bits[] = {
   0xff, 0x03, 0xff, 0x03, 0xff, 0x03, 0xff, 0x03, 0xff, 0x03, 0xff, 0x03,
   0xff, 0x03, 0xff, 0x03, 0xff, 0x03, 0xff, 0x03 };
static unsigned char pse_bits[] = {
   0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00,
   0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00 };
#define play_width 6
#define play_height 11
static unsigned char play_bits[] = {
   0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01 };
#define loop_width 11
#define loop_height 10
static unsigned char loop_bits[] = {
   0x04, 0x00, 0x08, 0x00, 0x9e, 0x03, 0x09, 0x04, 0x05, 0x04, 0x01, 0x05,
   0x81, 0x04, 0xce, 0x03, 0x80, 0x00, 0x00, 0x01 };
#define logo_width 90
#define logo_height 16
static unsigned char logo_bits[] = {
   0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x80, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xc0, 0xff, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xc0, 0xc7, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x3e, 0xcf, 0x07, 0xfc, 0xe3, 0xc3, 0x7b, 0x78, 0x0f, 0x3e, 0x9c, 0x01,
   0xdf, 0xcf, 0x3f, 0xfe, 0xf3, 0xf7, 0xff, 0xfe, 0x1f, 0x7f, 0xff, 0x03,
   0x3e, 0xcf, 0xff, 0x78, 0x38, 0xc7, 0xf3, 0x78, 0x9e, 0x73, 0xfe, 0x03,
   0x3c, 0x8f, 0xff, 0x79, 0xfc, 0xcf, 0xf3, 0x79, 0xde, 0xff, 0xbc, 0x01,
   0x1c, 0x47, 0xfe, 0x78, 0xfc, 0xcf, 0xf3, 0x78, 0xbe, 0x7f, 0x1e, 0x00,
   0x3c, 0xef, 0xf0, 0x79, 0x38, 0xc0, 0xf3, 0x79, 0xde, 0x03, 0x1e, 0x00,
   0xbe, 0xcf, 0xff, 0x78, 0x78, 0xc4, 0xf3, 0x78, 0x9e, 0x47, 0x3e, 0x00,
   0xfc, 0xdf, 0x7f, 0xf8, 0xf3, 0xc7, 0x7f, 0xf8, 0x0f, 0x7f, 0x7e, 0x00,
   0x38, 0x07, 0x3f, 0xf0, 0xe1, 0xc3, 0x3f, 0xf8, 0x07, 0x3e, 0x7f, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x7c, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x07, 0xfc, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0xf8, 0x00, 0x00, 0x00, 0x00 };

typedef struct {
	uint8_t debounce,
	state,
	holdCnt,
	btn;
	uint16_t time;
} buttons;

#define DEPRESSED 0
#define PRESSED 1
#define HOLD 2

#define DBSAMPLEPERIOD 1UL
#define HOLDTIME 75
#define HOLDTICK 5

/** Macro for resetting watchdog timer */
#define RESETWDT asm volatile("WDR \n\t")


extern "C" void WDT_vect(void) __attribute__ ((signal,used));

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
	* @param[in]  	Delay in milliseconds between motor stopped and shutter fires.
	*/
	void egoShield::setup(uint16_t acc = 1500, 
						  uint16_t vel = 1000, 
						  uint8_t uStep = SIXTEEN, 
						  uint16_t fTol = 10,
						  uint16_t fHys = 5, 
						  float P = 1.0, 
						  float I = 0.02, 
						  float D = 0.006,
						  float res = 1,
						  uint16_t shutterDelay = 250);
	/**
	* @brief      	Contains the main logic of the shield functionality, e.g. transition between states (idle, play, record and pause).
	*/	
	void loop(void);
	/**
	* @brief      	Creates an uStepper instance.
	*/
	uStepper stepper;
private:
	/**Creates an SoftwareSerial instance for BT-module */
	SoftwareSerial *BTSerial;
	/**Creates an u8g2 (OLED) instance */
	U8G2_SSD1306_128X64_NONAME_1_4W_SW_SPI *u8g2;
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
	/** This variable holds the brake flag */
	bool brakeFlag;
	/* Declaration of structs for each button*/
	volatile buttons   	forwardBtn    = {0x1F, DEPRESSED, 0, 0, 0},
          				playBtn       = {0x1F, DEPRESSED, 0, 0, 0},
          				recordBtn     = {0x1F, DEPRESSED, 0, 0, 0},
          				backwardsBtn  = {0x1F, DEPRESSED, 0, 0, 0};
	/** This variable holds the value of the delay between motor stops and the shutter fires */
    uint16_t shutterDelay;

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
	* @param[in]  	index tells which step we are at in the timelapse mode.
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
	void debounce(buttons *btn, uint8_t sample);
	friend void WDT_vect(void) __attribute__ ((signal,used));
};
#endif