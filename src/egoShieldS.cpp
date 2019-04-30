/********************************************************************************************
*       File:       egoShieldTimeLapse.cpp                                                  *
*       Version:    1.1.0                                                                   *
*       Date:       March 17th, 2018                                                        *
*       Author:     Mogens Groth Nicolaisen                                                 *
*                                                                                           * 
*********************************************************************************************
*                 egoShield class                                                           *
*                                                                                           *
* This file contains the implementation of the class methods, incorporated in the           *
* egoShield Arduino library. The library is used by instantiating an egoShield object       *
* by calling of the overloaded constructor:                                                 *
*                                                                                           *
*   example:                                                                                *
*                                                                                           *
*   egoShield ego;                                                                          *
*                                                                                           *
* The instantiation above creates an egoShield object                                       *
* after instantiation of the object, the object setup function should be called within      *
* Arduino's setup function, and the object loop function should be run within the Arduino's *
* loop function:                                                                            *
*                                                                                           *
*   example:                                                                                *
*                                                                                           *
*   egoShieldTimeLapse ego;                                                                 *
*                                                                                           *
*   void setup()                                                                            *
*   {                                                                                       *
*     ego.setup();                                                                          *
*   }                                                                                       *
*                                                                                           *
*   void loop()                                                                             *
*   {                                                                                       *
*     ego.loop();                                                                           *
*   }                                                                                       *
*                                                                                           *
*                                                                                           *
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
 * @file egoShieldTeach.cpp
 * @brief      Class implementations for the egoShield Teach library
 *
 *             This file contains the implementations of the classes defined in
 *             egoShieldTeach.h
 *
 * @author     Mogens Groth Nicolaisen (mogens@ustepper.com)
 */
 
#include "egoShieldS.h"
#include "screen.h"
egoShield *egoPointer;

extern "C" {
  void TIMER4_COMPA_vect(void)
  {
    uint8_t temp = TCCR3B, temp1 = TCCR1B;
    TCCR3B &= ~(1 << CS30);
    TCCR1B &= ~(1 << CS10);
    egoPointer->inputs();
    TCCR3B = temp; 
    TCCR1B = temp1; 
  }
}

egoShield::egoShield(void)
{
}

void egoShield::setup(uint16_t acc, uint16_t vel, float P, float I, float D, float res, uint16_t shutterDelay)//brake mode?
{
  #ifdef ARDUINO_AVR_USTEPPER_S
    this->screen = new Screen(1);
  #endif

  #ifdef ARDUINO_AVR_USTEPPER_S_LITE
    this->screen = new Screen(0);
  #endif

  
  this->screen->init();
  egoPointer = this;

  
  this->acceleration = acc;
  this->velocity = vel;
  this->pTerm = P;
  this->iTerm = I;
  this->dTerm = D;
  this->resolution = res;
  this->stepSize = 2;
  this->interval = 2000;
  this->shutterDelay = shutterDelay;

  brakeFlag = 1;
  stepper.setup();
  //stepper.setup(PID,3200.0,50.0,0.0,0.0,true);
  //stepper.setup(PID,3200.0,5.5,4.1,0.0,true);
  // Check whether the uStepper is mounted on a motor with a magnet attached. If not, show an error message untill mounted correctly
  /*do
  {
    u8g2->firstPage();
    do 
    {
      u8g2->setFontMode(1);
      u8g2->setDrawColor(1);
      u8g2->setFontDirection(0);
      u8g2->setFont(u8g2_font_6x10_tf);
    
      u8g2->drawStr(2,10,"Magnet not present !");
    } while ( u8g2->nextPage() );  
  }
  while(stepper.encoder.detectMagnet() == 2 || stepper.encoder.detectMagnet() == 1);
*/

  stepper.encoder.setHome();
  DRAWPAGE(this->startPage());
  #ifdef ARDUINO_AVR_USTEPPER_S
    stepper.setMaxVelocity(this->velocity/16.0);
  #else
    stepper.setMaxVelocity(this->velocity);
  #endif
  stepper.setMaxAcceleration(this->acceleration);
  pinMode(FWBT ,INPUT);
  pinMode(PLBT ,INPUT);
  pinMode(RECBT ,INPUT);
  pinMode(BWBT ,INPUT);
  pinMode(OPTO,OUTPUT);
  digitalWrite(OPTO ,HIGH);
  digitalWrite(FWBT ,HIGH);//pull-up
  digitalWrite(PLBT ,HIGH);//pull-up
  digitalWrite(RECBT ,HIGH);//pull-up
  digitalWrite(BWBT ,HIGH);//pull-up
  
  DRAWPAGE(this->startPage());//show startpage
  delay(2000);//for 2 seconds
  this->resetAllButton(); 
  state = 'a';//start in idle
  stepper.moveToEnd(CW);
  stepper.encoder.setHome();

  stepper.moveToAngle(30);
  while(stepper.getMotorState());
  stepper.encoder.setHome();
  pidFlag = 0;//enable PID
  setPoint = stepper.encoder.getAngleMoved();//set manual move setpoint to current position
  TCNT4 = 0;
  ICR4 = 35000;
  TIFR4 = 0;
  TIMSK4 = (1 << OCIE4A);
  TCCR4A = (1 << WGM41);
  TCCR4B = (1 << WGM42) | (1 << WGM43) | ( 1 << CS41);
}

void egoShieldTimeLapse::loop(void)
{
  setPoint = stepper.encoder.getAngleMoved();

  switch (state)
  {
    case 'a'://if we are in idle
      idleMode();
    break;
    
    case 'b'://if we are in play
        state = 'a';
    break;
    
    case 'c'://if we are in record
        timeMode();
    break;

    case 'd'://in pause
      pauseMode();
    break;
  }
}

void egoShieldTeach::loop(void)
{
  setPoint = stepper.encoder.getAngleMoved();
  switch (state)
  {
    case 'a'://if we are in idle
      idleMode();
    break;
    
    case 'b'://if we are in play
        playMode();
    break;
    
    case 'c'://if we are in record
        recordMode();
    break;

    case 'd'://in pause
      pauseMode();
    break;
  }
}

void egoShield::recordMode(void)
{
  static bool continousForward = 0;
  static bool continousBackwards = 0;

  DRAWPAGE(this->recordPage(pidFlag,0,place,setPoint));
  
  if(continousForward)
  {
    if(this->forwardBtn.state != HOLD)
    {
      stepper.stop();
      this->resetAllButton(); 
      continousForward = 0;
    }
  }
  else if(continousBackwards)
  {
    if(this->backwardsBtn.state != HOLD)
    {
      stepper.stop();
      this->resetAllButton(); 
      continousBackwards = 0;
    }
  }
  if(this->forwardBtn.btn)//if manual forward signal
  {
    if(this->forwardBtn.state == HOLD)
    {
      if(!continousForward)
      {
        stepper.runContinous(CCW);
        continousForward = 1;  
      }
      this->forwardBtn.btn = 0;
    }
    else
    {
      stepper.moveAngle(-5.0);//move 5deg
      this->forwardBtn.btn = 0;
    }
  }
  else if(this->backwardsBtn.btn)//if manual backward signal
  {
    if(this->backwardsBtn.state == HOLD)
    {
      if(!continousBackwards)
      {
        stepper.runContinous(CW);
        continousBackwards = 1;  
      }
      this->backwardsBtn.btn = 0;
    }
    else
    {
      stepper.moveAngle(5.0);//move 5deg
      this->backwardsBtn.btn = 0;
    }
  }
  else if(this->recordBtn.btn == 1)//record position
  {      
    this->recordBtn.btn = 0;
    if(record == 0)//If we were not recording before
    {
      place = 0;//Reset the array counter
      record = 1;//Record flag
    }
    DRAWPAGE(this->recordPage(pidFlag,1,place,setPoint));
    delay(500);
    if(record == 1)//If we have initialized recording
    {
      pos[place] = setPoint;//Save current position
      place++;//Increment array counter
      if(place>CNT)
      {
        place=0;
      }
    }
  }
  else if(this->playBtn.btn == 1)//stop pressed
  {
    endmove = place-1;//set the endmove to the current position
    place = 0;//reset array counter
    record = 0;//reset record flag
    state = 'a';//stop state
    continousForward = 0;
    continousBackwards = 0;
    stepper.moveToAngle(pos[0]);
    do
    {
      DRAWPAGE(this->idlePage(pidFlag,setPoint));
    }
    while(this->playBtn.state == HOLD);
    this->resetAllButton(); 
  }  
}

void egoShield::idleMode(void)
{
  static bool continousForward = 0;
  static bool continousBackwards = 0;

  DRAWPAGE(this->idlePage(pidFlag,setPoint));
  if(continousForward)
  {
    if(this->forwardBtn.state != HOLD)
    {
      stepper.stop();
      this->resetAllButton(); 
      continousForward = 0;
    }
  }
  else if(continousBackwards)
  {
    if(this->backwardsBtn.state != HOLD)
    {
      stepper.stop();
      this->resetAllButton(); 
      continousBackwards = 0;
    }
  }
  if(this->playBtn.btn)//if play/stop/pause is pressed for long time, invert the pid mode
  {
    while(this->playBtn.state == PRESSED);
    if(this->playBtn.state == DEPRESSED)//we want to play sequence when doing a short press
    {
      state = 'b';
      continousForward = 0;
      continousBackwards = 0;
      this->resetAllButton(); 
    }
    else
    {
      if(pidFlag == 0)
      {
        pidFlag = 1;
        //stepper.enablePid();
      }
      else
      {
        pidFlag = 0;
        //stepper.disablePid();
        stepper.stop();
      }
      DRAWPAGE(this->idlePage(pidFlag,setPoint));
      while(this->playBtn.state == HOLD);
    }
    this->resetButton(&playBtn);
  }
  else if(this->forwardBtn.btn)//if manual forward signal
  {
    if(this->forwardBtn.state == HOLD)
    {
      if(!continousForward)
      {
        stepper.runContinous(CCW);
        continousForward = 1;  
      }
      this->forwardBtn.btn = 0;
    }
    else
    {
      stepper.moveAngle(-5.0);//move 5deg
      this->forwardBtn.btn = 0;
    }
  }
  else if(this->backwardsBtn.btn)//if manual backward signal
  {
    if(this->backwardsBtn.state == HOLD)
    {
      if(!continousBackwards)
      {
        stepper.runContinous(CW);
        continousBackwards = 1;  
      }
      this->backwardsBtn.btn = 0;
    }
    else
    {
      stepper.moveAngle(5.0);//move 5deg
      this->backwardsBtn.btn = 0;
    }
  }
  else if(this->recordBtn.btn == 1)
  {
    this->resetAllButton(); 
    continousForward = 0;
    continousBackwards = 0;
    state = 'c';
  }
}

void egoShield::playMode(void)
{
  static uint8_t started = 0;
  static bool lastMove = 0;

  DRAWPAGE(this->playPage(loopMode,pidFlag,place,0));
  
  if(this->recordBtn.btn)//play/stop/pause
  {
    while(this->recordBtn.state == PRESSED);
    if(this->recordBtn.state == DEPRESSED)//we want to play sequence when doing a short press
    {
        state = 'd';
        this->resetAllButton(); 
        return;
    }
    else //Long press = stop
    {
      this->resetAllButton();    
      this->changeVelocity();
    }
  }
  else if(this->playBtn.btn)//play/stop/pause
  {
    while(this->playBtn.state == PRESSED);
    if(this->playBtn.state == DEPRESSED)//we want to play sequence when doing a short press
    {
      started = 1;
    }
    else //Long press = stop
    {
      place = 0;//reset array counter
      loopMode = 0;
      lastMove = 0;
      started = 0;
      state = 'a';//idle
      stepper.moveToAngle(pos[0]);
      do
      {
        DRAWPAGE(this->idlePage(pidFlag,setPoint));
      }
      while(this->playBtn.state == HOLD);
    }
    this->resetAllButton(); 
    return;
  }
  else if(started || loopMode)
  {
    #ifdef ARDUINO_AVR_USTEPPER_S
    if(!stepper.driver.readRegister(VACTUAL))
    #else
    if(!stepper.getMotorState())
    #endif
    {
      place++;//increment array counter
      if(lastMove && loopMode)
      {
        lastMove = 0;
      }
      if(loopMode && place > endmove)
      {
        place = 0;
        lastMove = 1;
      }
      else if(place > endmove)//If we are at the end move
      {
        place = 0;//reset array counter
        lastMove = 1;
      }
      else if(lastMove)//If we are at the end move
      {
        place = 0;//reset array counter
        started = 0;
        lastMove = 0;
        state = 'a';
        this->resetAllButton(); 
        return;
      }
      stepper.moveToAngle(pos[place]);   
    }
  }
  
  if(this->forwardBtn.state == HOLD)//loop mode start
  {
    this->resetButton(&forwardBtn);
    loopMode = 1;
  }
  else if(this->backwardsBtn.state == HOLD)//loop mode stop
  {
    this->resetButton(&backwardsBtn);
    loopMode = 0;
  }
}

void egoShield::changeVelocity(void)
{
  for(;;)
  {
    DRAWPAGE(this->playPage(loopMode,pidFlag,place,1));
    if(this->forwardBtn.btn == 1 && this->velocity <= 9900 && this->acceleration <= 19900)//increase speed
    {
      this->forwardBtn.btn = 0;
      this->velocity+=10;
      this->acceleration+=10;
    }
    else if(this->backwardsBtn.btn == 1 && this->velocity >= 10 && this->acceleration >= 10)//decrease speed
    {
      this->backwardsBtn.btn = 0;
      this->velocity-=10;
      this->acceleration-=10;
    }
    else if(this->playBtn.btn == 1)
    {
      #ifdef ARDUINO_AVR_USTEPPER_S
        stepper.setMaxVelocity(this->velocity/16.0);
      #else
        stepper.setMaxVelocity(this->velocity);
      #endif
      stepper.setMaxAcceleration(this->acceleration);
      this->resetAllButton(); 
      return;
    }
  }
}

void egoShield::pauseMode(void)
{
  DRAWPAGE(this->pausePage(loopMode,pidFlag,place));
  if(this->playBtn.btn)//play/stop/pause
  {
    while(this->playBtn.state == PRESSED);
    if(this->playBtn.state == DEPRESSED) //Short press = unpause
    {
      state = 'b';
    }
    else      //Long press = stop
    {
      state = 'a';
      loopMode = 0;
      DRAWPAGE(this->idlePage(pidFlag,setPoint));
      while(this->playBtn.state == HOLD);
      this->resetAllButton();
    }
    this->resetButton(&playBtn);
  }
  if(this->forwardBtn.state == HOLD)//loop mode start
  {
    this->resetButton(&forwardBtn);
    loopMode = 1;
  }
  else if(this->backwardsBtn.state == HOLD)//loop mode stop
  {
    this->resetButton(&backwardsBtn);
    loopMode = 0;
  }
}

void egoShield::timeMode(void)
{  
  static uint8_t step = 0;  
  static uint32_t i = 0, j = 0;
  static uint8_t runState = 0;

  stepper.getMotorState();

  DRAWPAGE(this->timePage(step,pidFlag));
  if(step == 0)//first put in how long to move at every step in mm
  {
    digitalWrite(OPTO, HIGH);
    if(this->forwardBtn.btn == 1 && stepSize < 100.0)
    {
      this->forwardBtn.btn = 0;
      stepSize=stepSize+0.25;
    }
    else if(this->backwardsBtn.btn == 1 && stepSize >= 0.5)
    {
      this->backwardsBtn.btn = 0;
      stepSize=stepSize-0.25;
    }
    else if(this->recordBtn.btn == 1)
    {
        this->recordBtn.btn = 0;
        step = 1;
    }
  }
  else if(step == 1)//next put in how long the intervals between moves are in milliseconds
  {
    if(this->forwardBtn.btn == 1 && interval < 65000)
    {
      this->forwardBtn.btn = 0;
      interval=interval+250;
    }
    else if(this->backwardsBtn.btn == 1 && interval >= (500 + this->shutterDelay))
    {
      this->backwardsBtn.btn = 0;
      interval=interval-250;
    }
    else if(this->recordBtn.btn == 1)
    {
      delay(200);
      this->recordBtn.btn = 0;
      step = 2;
    }
  }
  else if(step == 2)//ready to play
  {
    if(this->playBtn.btn)//play/stop/pause
    {
      while(this->playBtn.state == PRESSED);
      if(this->playBtn.state == DEPRESSED) //Short press = unpause
      {
        step = 3;
        i = millis();
      }
      else      //Long press = stop
      {
        state = 'a';
        step = 0;
        this->resetAllButton(); 
      }
      this->resetButton(&playBtn);
    }
  }
  else if(step == 3)//playing until the end
  {  
    if(runState == 0)   //start new movement
    {
        setPoint += (stepSize*resolution);
        stepper.moveAngle((stepSize*resolution));
        DRAWPAGE(this->timePage(step,pidFlag));
        i = millis();
        runState = 1;
        return;
    }
    else if(runState == 1) //Waiting for movement to finish
    {
      if(!stepper.getMotorState())
      {
        j = millis();
        runState = 2;
        return;
      }
    }
    else if(runState == 2) //Waiting 250ms before firing trigger. (to stabilize rail and avoid vibrations)
    {
      if(((millis() - j) > this->shutterDelay))
      {
        digitalWrite(OPTO, LOW);   // sets the LED in the opto on triggering the camera
        runState = 3;
        j = millis();
        return;
      }
    }
    else if(runState == 3) //Waiting to release trigger
    {
      if(((millis() - j) > 200))
      {
        digitalWrite(OPTO, HIGH);  // sets the LED in the opto off releases the camera trigger
        runState = 4;
        return;
      }
    }
    else if(runState == 4) //Waiting The remaining period
    {
      if((millis() - i) > interval)
      {
        runState = 0;
        return;
      }
    }
    if(this->playBtn.btn == 1)
    {
      state = 'a';
      step = 0;
      this->resetAllButton(); 
      return;
    }
    if(stepper.isStalled())
    {
      stepper.moveToEnd(CCW);
      stepper.encoder.setHome();
      stepper.moveToAngle(30);
      while(stepper.getMotorState());
      stepper.encoder.setHome();
      state = 'a';//idle state 
      step = 0;
      this->resetAllButton();
      return;
    }
  }
}

void egoShield::inputs(void)
{
    this->debounce(&forwardBtn,(PINB >> 5) & 0x01);
    this->debounce(&playBtn,(PINB >> 3) & 0x01);
    this->debounce(&recordBtn,(PIND >> 2) & 0x01);
    this->debounce(&backwardsBtn,(PINB >> 4) & 0x01);
}

void egoShield::startPage(void)
{
  this->screen->clrScreen();
  this->screen->drawImage(logoBmp, 10, 0, 112, 48);
}

void egoShield::idlePage(bool pidMode, float pos)
{
  static int32_t position; 
  static bool lastPidMode;
  char buf[20];
  String sBuf;

  if(this->lastPage != IDLEPAGE)
  {
    this->screen->clrScreen();
    lastPidMode = pidMode;
    this->lastPage = IDLEPAGE;
    position = (int32_t)(stepper.encoder.getAngleMoved()/this->resolution);
    sBuf = "Position: ";
    sBuf += position;
    sBuf += " mm";
    sBuf.toCharArray(buf, 20);
    this->screen->drawRect(0,0,127,7,1);
    this->screen->drawRect(0,48,127,63,1);
    this->screen->printString("Idle",2,0,1);
    if(pidMode)
    {
      this->screen->printString("PID ON ",45,0,1);
    }
    else
    {
      this->screen->printString("PID OFF",45,0,1);
    }
    this->screen->drawImage(fastRewindBmp, 0, 48, 16, 16, 1);
    this->screen->drawImage(playBmp, 28, 48, 16, 16, 1);
    this->screen->drawImage(stopBmp, 39, 48, 16, 16, 1);
    this->screen->drawImage(recordBmp, 67, 48, 16, 16, 1);
    this->screen->drawImage(pauseBmp, 81, 48, 16, 16, 1);
    this->screen->drawImage(fastForwardBmp, 112, 48, 16, 16, 1);
    sBuf.toCharArray(buf, 20);
    this->screen->printString((const uint8_t*)buf,2,24,0);
  }
  else
  {
    if((int32_t)(stepper.encoder.getAngleMoved()/this->resolution) != position)
    {
      position = (int32_t)(stepper.encoder.getAngleMoved()/this->resolution);
      sBuf = position;
      sBuf += " mm";
      sBuf.toCharArray(buf, 20);

      this->screen->drawRect(62,24,100,31,0);
      this->screen->printString((const uint8_t*)buf,62,24,0);
    }

    if(pidMode != lastPidMode)
    {
      lastPidMode = pidMode;
      if(pidMode)
      {
        this->screen->printString("PID ON ",45,0,1);
      }
      else
      {
        this->screen->printString("PID OFF",45,0,1);
      }
    }
  }
}

void egoShield::recordPage(bool pidMode, bool recorded, uint8_t index, float pos)
{
  static int32_t position; 
  static bool lastPidMode, updatePosition;
  char buf[22];
  String sBuf;

  if(this->lastPage != RECORDPAGE)
  {
    this->screen->clrScreen();
    lastPidMode = pidMode;
    this->lastPage = RECORDPAGE;
    position = (int32_t)(pos/this->resolution);
    sBuf = "Position: ";
    sBuf += position;
    sBuf += " mm";
    sBuf.toCharArray(buf, 20);
    this->screen->drawRect(0,0,127,7,1);
    this->screen->drawRect(0,48,127,63,1);
    if(pidMode)
    {
      this->screen->printString("PID ON ",45,0,1);
    }
    else
    {
      this->screen->printString("PID OFF",45,0,1);
    }
    this->screen->drawImage(fastRewindBmp, 0, 48, 16, 16, 1);
    this->screen->drawImage(stopBmp, 35, 48, 16, 16, 1);
    this->screen->drawImage(recordBmp, 73, 48, 16, 16, 1);
    this->screen->drawImage(fastForwardBmp, 112, 48, 16, 16, 1);

    if(recorded)
    {
      sBuf = "Position: ";
      sBuf += index;
      sBuf += " recorded";
      sBuf.toCharArray(buf, 22);
      this->screen->printString((const uint8_t*)buf,2,24,0);
      updatePosition = 1;
    }
    else
    {
      sBuf = "Position: ";
      sBuf += (int32_t)(pos/this->resolution);
      sBuf += " mm";
      sBuf.toCharArray(buf, 22);
      this->screen->printString((const uint8_t*)buf,2,24,0);
      updatePosition = 0;
    }
  }
  else
  {
    if(recorded)
    {
      sBuf = "Position: ";
      sBuf += index;
      sBuf += " recorded";
      sBuf.toCharArray(buf, 22);
      this->screen->drawRect(0,24,127,31,0);
      this->screen->printString((const uint8_t*)buf,2,24,0);
      updatePosition = 1;
    }
    else
    {
      if((int32_t)(pos/this->resolution) != position || updatePosition)
      {
        updatePosition = 0;
        position = (int32_t)(pos/this->resolution);

        sBuf = position;
        sBuf += " mm";
        sBuf.toCharArray(buf, 20);

        this->screen->drawRect(62,24,127,31,0);
        this->screen->printString((const uint8_t*)buf,62,24,0);
      }
    }

    if(pidMode != lastPidMode)
    {
      this->screen->drawRect(0,0,127,7,1);
      lastPidMode = pidMode;
      if(pidMode)
      {
        this->screen->printString("PID ON ",45,0,1);
      }
      else
      {
        this->screen->printString("PID OFF",45,0,1);
      }
    }
  }
}

void egoShield::playPage(bool loopMode, bool pidMode, uint8_t index, bool mode)
{
  static int32_t position; 
  static bool lastPidMode, lastMode, lastLoopMode, lastIndex;
  static float lastVelocity;
  char buf[22];
  String sBuf;

  if(this->lastPage != PLAYPAGE)
  {
    this->screen->clrScreen();
    lastPidMode = pidMode;
    this->lastPage = PLAYPAGE;
    this->screen->drawRect(0,0,127,7,1);
    this->screen->drawRect(0,48,127,63,1);
    this->screen->printString("Play",2,0,1);
    if(pidMode)
    {
      this->screen->printString("PID ON ",45,0,1);
    }
    else
    {
      this->screen->printString("PID OFF",45,0,1);
    }
    this->screen->drawImage(fastRewindBmp, 0, 48, 16, 16, 1);
    this->screen->drawImage(playBmp, 34, 48, 16, 16, 1);
    this->screen->drawImage(stopBmp, 48, 48, 16, 16, 1);
    this->screen->drawImage(pauseBmp, 79, 48, 16, 16, 1);
    this->screen->drawImage(fastForwardBmp, 112, 48, 16, 16, 1);

    lastLoopMode = loopMode;

    if(loopMode)
    {
      this->screen->drawImage(repeatBmp, 112, 0, 16, 8, 1);
    }

    lastMode = mode;
    if(mode)
    {
      this->screen->printString("Adjust velocity",2,24,0);
    }
    else
    {
      this->screen->printString("Moving to pos",2,24,0);
      String(index).toCharArray(buf, 5);
      this->screen->printString((const uint8_t*)buf,90,24,0);
    }
    lastVelocity = this->velocity;
    this->screen->printString("Speed: ",2,32,0);
    String(this->velocity).toCharArray(buf, 5);
    this->screen->printString((const uint8_t*)buf,60,32,0);
  }
  else
  {
    if(lastVelocity != this->velocity)
    {
      String(this->velocity).toCharArray(buf, 5);
      this->screen->printString((const uint8_t*)buf,60,32,0);
    }

    lastVelocity = this->velocity;

    if(mode)
    {
      if(lastMode != mode)
      {
        this->screen->drawRect(0,24,127,31,0);
        this->screen->printString("Adjust velocity",2,24,0);
      }
    }
    else
    {
      if(lastMode != mode)
      {
        this->screen->drawRect(0,24,127,31,0);
        this->screen->printString("Moving to pos",2,24,0);
        String(index).toCharArray(buf, 5);
        this->screen->printString((const uint8_t*)buf,90,24,0);
      }
      else
      {
        if(lastIndex != index)
        {
          String(index).toCharArray(buf, 5);
          this->screen->printString((const uint8_t*)buf,90,24,0);
        }
      }
      lastIndex = index;
    }

    lastMode = mode;

    if(loopMode != lastLoopMode)
    {
      this->screen->drawRect(110,0,127,7,1);
      if(loopMode)
      {
        this->screen->drawImage(repeatBmp, 112, 0, 16, 8, 1);
      }
    }
    lastLoopMode = loopMode;

    if(pidMode != lastPidMode)
    {
      this->screen->drawRect(0,0,127,7,1);
      lastPidMode = pidMode;
      if(pidMode)
      {
        this->screen->printString("PID ON ",45,0,1);
      }
      else
      {
        this->screen->printString("PID OFF",45,0,1);
      }
    }
  }
}

void egoShield::pausePage(bool loopMode, bool pidMode, uint8_t index)
{
  static bool lastPidMode, lastLoopMode;
  char buf[22];
  String sBuf;

  if(this->lastPage != PAUSEPAGE)
  {
    lastPidMode = pidMode;
    this->lastPage = PAUSEPAGE;
    this->screen->clrScreen();
    this->screen->drawRect(0,0,127,7,1);
    this->screen->drawRect(0,48,127,63,1);
    this->screen->printString("Pause",2,0,1);
    if(pidMode)
    {
      this->screen->printString("PID ON ",45,0,1);
    }
    else
    {
      this->screen->printString("PID OFF",45,0,1);
    }
    this->screen->drawImage(playBmp, 22, 48, 16, 16, 1);
    this->screen->drawImage(stopBmp, 35, 48, 16, 16, 1);

    lastLoopMode = loopMode;

    if(loopMode)
    {
      this->screen->drawImage(repeatBmp, 112, 0, 16, 8, 1);
    }

    this->screen->printString("Paused at Position ",2,24,0);
    String(index).toCharArray(buf, 5);
    this->screen->printString((const uint8_t*)buf,114,24,0);
  }
  else
  {
    if(loopMode != lastLoopMode)
    {
      this->screen->drawRect(110,0,127,7,1);
      if(loopMode)
      {
        this->screen->drawImage(repeatBmp, 112, 0, 16, 8, 1);
      }
    }

    lastLoopMode = loopMode;

    if(pidMode != lastPidMode)
    {
      this->screen->drawRect(45,0,127,7,1);
      lastPidMode = pidMode;
      if(pidMode)
      {
        this->screen->printString("PID ON ",45,0,1);
      }
      else
      {
        this->screen->printString("PID OFF",45,0,1);
      }
    }
  }
}


void egoShield::timePage(uint8_t step, bool pidMode)
{

  static bool lastPidMode;
  static uint8_t lastStep;
  static float lastStepSize, lastInterval;
  int32_t angle;
  static int32_t lastAngle;
  char buf[22];
  String sBuf;

  if(this->lastPage != TIMEPAGE)
  {
    lastPidMode = pidMode;
    this->lastPage = TIMEPAGE;
    this->screen->clrScreen();
    this->screen->drawRect(0,0,127,7,1);
    this->screen->drawRect(0,48,127,63,1);
    this->screen->printString("Time",2,0,1);


    if(step == 0)//we are waiting for the distance interval to be put in
    {
      this->screen->drawImage(fastRewindBmp, 0, 48, 16, 16, 1);
      this->screen->drawImage(recordBmp, 73, 48, 16, 16, 1);
      this->screen->drawImage(fastForwardBmp, 112, 48, 16, 16, 1);
     this->screen->printString("<-",115,24,0);
    }
    else if(step == 1)//we are waiting for the time interval to be put in
    {
      this->screen->drawImage(fastRewindBmp, 0, 48, 16, 16, 1);
      this->screen->drawImage(recordBmp, 73, 48, 16, 16, 1);
      this->screen->drawImage(fastForwardBmp, 112, 48, 16, 16, 1);
      this->screen->printString("<-",115,32,0);
    }
    else if(step == 2)//we are waiting for play to be issued
    {
      this->screen->drawImage(playBmp, 28, 48, 16, 16, 1);
      this->screen->drawImage(stopBmp, 35, 48, 16, 16, 1);
    }
    else if(step == 3)//we are playing sequence until end of rail
    {
      this->screen->drawImage(stopBmp, 35, 48, 16, 16, 1);
    }

    lastStep = step;
    this->screen->drawRect(45,0,127,7,1);
    if(pidMode)
    {
      this->screen->printString("PID ON ",45,0,1);
    }
    else
    {
      this->screen->printString("PID OFF",45,0,1);
    }

    angle = (int32_t)(stepper.encoder.getAngleMoved()/resolution);

    sBuf = "Stepsize:  ";
    sBuf += stepSize;
    lastStepSize = stepSize;
    sBuf += " mm";
    sBuf.toCharArray(buf, 22);
    this->screen->printString((const uint8_t*)buf,2,24,0);
    sBuf = "Interval:  ";
    sBuf += interval*0.001;
    lastInterval = interval;
    sBuf += " s";
    sBuf.toCharArray(buf, 22);
    this->screen->printString((const uint8_t*)buf,2,32,0);
    sBuf = "Encoder:   ";
    sBuf += angle;
    lastAngle = angle;
    sBuf += " mm";
    sBuf.toCharArray(buf, 22);
    this->screen->printString((const uint8_t*)buf,2,40,0);
  }
  else
  {
    angle = (int32_t)(stepper.encoder.getAngleMoved()/resolution);

    if(lastStepSize != stepSize)
    {
      sBuf = stepSize;
      lastStepSize = stepSize;
      sBuf += " mm";
      sBuf.toCharArray(buf, 22);
      this->screen->drawRect(66,24,127,31,0);
      this->screen->printString((const uint8_t*)buf,68,24,0);
    }

    if(lastInterval != interval)
    {
      sBuf = interval*0.001;
      lastInterval = interval;
      sBuf += " s";
      sBuf.toCharArray(buf, 22);
      this->screen->drawRect(66,32,127,39,0);
      this->screen->printString((const uint8_t*)buf,68,32,0);
    }

    if(lastAngle != angle)
    {
      sBuf = angle;
      lastAngle = angle;
      sBuf += " mm";
      sBuf.toCharArray(buf, 22);
      this->screen->drawRect(68,40,114,47,0);
      this->screen->printString((const uint8_t*)buf,68,40,0);
    }

    if(step != lastStep)
    {
      this->screen->drawRect(0,48,127,63,1);
      this->screen->drawRect(115,24,127,39,0);
      if(step == 0)//we are waiting for the distance interval to be put in
      {
        this->screen->drawImage(fastRewindBmp, 0, 48, 16, 16, 1);
        this->screen->drawImage(recordBmp, 73, 48, 16, 16, 1);
        this->screen->drawImage(fastForwardBmp, 112, 48, 16, 16, 1);
        this->screen->printString("<-",115,24,0);
      }
      else if(step == 1)//we are waiting for the time interval to be put in
      {
        this->screen->drawImage(fastRewindBmp, 0, 48, 16, 16, 1);
        this->screen->drawImage(recordBmp, 73, 48, 16, 16, 1);
        this->screen->drawImage(fastForwardBmp, 112, 48, 16, 16, 1);
        this->screen->printString("<-",115,32,0);
      }
      else if(step == 2)//we are waiting for play to be issued
      {
        this->screen->drawImage(playBmp, 35, 48, 16, 16, 1);
      }
      else if(step == 3)//we are playing sequence until end of rail
      {
        this->screen->drawImage(stopBmp, 35, 48, 16, 16, 1);
      }

      lastStep = step;
    }

    if(pidMode != lastPidMode)
    {
      this->screen->drawRect(45,0,127,7,1);
      lastPidMode = pidMode;
      if(pidMode)
      {
        this->screen->printString("PID ON ",45,0,1);
      }
      else
      {
        this->screen->printString("PID OFF",45,0,1);
      }
    }
  }
}

void egoShield::debounce(buttons *btn, uint8_t sample)
{
  if(btn->state == DEPRESSED)
    {
    btn->debounce &= (0xFE + sample);

        if( (btn->debounce & 0x1F) == 0x00)
        {
            btn->state = PRESSED;
            btn->btn = 1;
            return;
        }

        btn->debounce <<= 1;
        btn->debounce |= 0x01;
    }

    else if((btn->state == PRESSED) || (btn->state == HOLD))
    {
        btn->debounce |= sample;

        if(btn->state != HOLD)
        {
          if((btn->debounce & 0x1F) == 0x00)
          {
            if(btn->holdCnt >= HOLDTIME)
            {
              btn->state = HOLD;
            }
            btn->holdCnt++;
          }
        }

        if( (btn->debounce & 0x1F) == 0x1F)
        {
            btn->state = DEPRESSED;
            btn->holdCnt = 0;
            return;
        }

        btn->debounce <<= 1;
        btn->debounce &= 0xFE;
    }

    if(btn->state == HOLD)
    {
      if(btn->time == HOLDTICK)
      {
        btn->btn = 1;
        btn->time = 0;
      }

      else
      {
        btn->time++;
      }
    }
}

void egoShield::resetButton(buttons *btn)
{
  btn->time = 0;
  btn->state = DEPRESSED;
  btn->debounce = 0x1F;
  btn->holdCnt = 0;
  btn->btn = 0;
}
void egoShield::resetAllButton()
{
    this->resetButton(&playBtn);
    this->resetButton(&forwardBtn);
    this->resetButton(&backwardsBtn);
    this->resetButton(&recordBtn);
}