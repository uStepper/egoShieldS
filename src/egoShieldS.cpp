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

egoShield *egoPointer;

extern "C" {
  void WDT_vect(void)
  {
    sei();
    egoPointer->inputs();
    WDTCSR |= (1<<WDIE);
  }
}

egoShield::egoShield(void)
{
  u8g2 = new U8G2_SSD1306_128X64_NONAME_1_HW_I2C(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
}

void egoShield::setup(uint16_t acc, uint16_t vel, float P, float I, float D, float res, uint16_t shutterDelay)//brake mode?
{
  egoPointer = this;
  //cli();
  RESETWDT;
  WDTCSR = (1 << WDCE) | (1 << WDE);
  WDTCSR |= (1 << WDIE) | (1 << WDE);

 
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
  u8g2->begin();//start display
  
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
  stepper.setMaxVelocity(this->velocity);
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
  
  this->startPage();//show startpage
  delay(2000);//for 2 seconds
  this->resetAllButton(); 
  state = 'a';//start in idle
  stepper.moveToEnd(CCW);
  stepper.encoder.setHome();
  stepper.moveToAngle(30);
  while(stepper.getMotorState());
  stepper.encoder.setHome();
  pidFlag = 1;//enable PID
  setPoint = stepper.encoder.getAngleMoved();//set manual move setpoint to current position

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
  
  this->recordPage(pidFlag,0,place,setPoint);

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
      //stepper.encoder.setHome();//Set current position as home
      //setPoint = 0;
      place = 0;//Reset the array counter
      record = 1;//Record flag
    }
    this->recordPage(pidFlag,1,place,setPoint);
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
    this->idlePage(pidFlag,setPoint);
    while(this->playBtn.state == HOLD);
    this->resetAllButton(); 
  }  
}

void egoShield::idleMode(void)
{
  static bool continousForward = 0;
  static bool continousBackwards = 0;

  this->idlePage(pidFlag,setPoint);

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
      this->idlePage(pidFlag,setPoint);
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

  this->playPage(loopMode,pidFlag,place,0);
  
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
      started = 0;
      state = 'a';//idle
      this->idlePage(pidFlag,setPoint);
      while(this->playBtn.state == HOLD);
    }
    this->resetAllButton(); 
    return;
  }
  else if(started || loopMode)
  {
    if(!stepper.driver.readRegister(VACTUAL))
    {
      place++;//increment array counter
      if(loopMode && place > endmove)
      {
        place = 0;
      }
      else if(place > endmove)//If we are at the end move
      {
        place = 0;//reset array counter
        started = 0;
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
    this->playPage(loopMode,pidFlag,place,1);
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
      stepper.setMaxVelocity(this->velocity);
      stepper.setMaxAcceleration(this->acceleration);
      this->resetAllButton(); 
      return;
    }
  }
}

void egoShield::pauseMode(void)
{
  this->pausePage(loopMode,pidFlag,place);
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
      this->idlePage(pidFlag,setPoint);
      while(this->playBtn.state == HOLD);
      this->resetAllButton();
    }
    this->resetButton(&playBtn);
  }
}

void egoShield::timeMode(void)
{  
  static uint8_t step = 0;  
  static uint32_t i = 0, j = 0;
  static uint8_t runState = 0;

  stepper.getMotorState();

  this->timePage(step,pidFlag);
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
      delay(200);
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
        this->timePage(step,pidFlag);
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

unsigned char* egoShield::loadVideoBuffer(unsigned char *data, unsigned char length)
{
  int i = 0;
  for(i = 0; i < length; i++)
  {
    this->videoBuffer[i] = pgm_read_byte(&(data[i]));
  }

  return this->videoBuffer;
}

void egoShield::startPage(void)
{
  u8g2->firstPage();
  do {
    u8g2->drawXBM(19, 20, logo_width, logo_height, this->loadVideoBuffer(logo_bits, sizeof(logo_bits)/sizeof(logo_bits[0])));
  } while ( u8g2->nextPage() );
}

void egoShield::idlePage(bool pidMode, float pos)
{
  char buf[20];
  String sBuf;

  sBuf = "Position: ";
  sBuf += (int32_t)(pos/this->resolution);
  sBuf += " mm";
  sBuf.toCharArray(buf, 20);

  u8g2->firstPage();
  do {
    u8g2->drawBox(1, 1, 128, 12);
    u8g2->drawBox(1, 48, 128, 68);
    u8g2->setFontMode(0);
    u8g2->setDrawColor(0);
    u8g2->setFontDirection(0);
    u8g2->setFont(u8g2_font_6x10_tf);
    
    //Bottom bar
    u8g2->drawXBM(5, 51, en_width, en_height, this->loadVideoBuffer(bw_bits, sizeof(bw_bits)/sizeof(bw_bits[0]))); 
    u8g2->drawXBM(112, 51, en_width, en_height, this->loadVideoBuffer(fw_bits, sizeof(fw_bits)/sizeof(fw_bits[0]))); 
    u8g2->drawXBM(32, 50, play_width, play_height, this->loadVideoBuffer(play_bits, sizeof(play_bits)/sizeof(play_bits[0])));  
    u8g2->drawXBM(43, 51, tt_width, tt_height, this->loadVideoBuffer(stop_bits, sizeof(stop_bits)/sizeof(stop_bits[0])));  
    u8g2->drawXBM(71, 51, tt_width, tt_height, this->loadVideoBuffer(rec_bits, sizeof(rec_bits)/sizeof(rec_bits[0]))); 
    u8g2->drawXBM(85, 51, tt_width, tt_height, this->loadVideoBuffer(pse_bits, sizeof(pse_bits)/sizeof(pse_bits[0]))); 

    //Mode
    u8g2->drawStr(2,10,"Idle");
    if(pidMode)
    {
      u8g2->drawStr(45,10,"PID ON");
    }
    else
    {
      u8g2->drawStr(45,10,"PID OFF");
    }
    u8g2->setFontMode(1);
    u8g2->setDrawColor(1);
    u8g2->drawStr(2,35,buf);
  } while ( u8g2->nextPage() );  
}

void egoShield::recordPage(bool pidMode, bool recorded, uint8_t index, float pos)
{
  char buf[22];//char array buffer
  String sBuf;
    
  u8g2->firstPage();
  do 
  {
    u8g2->drawBox(1, 1, 128, 12);
    u8g2->drawBox(1, 48, 128, 68);
    u8g2->setFontMode(0);
    u8g2->setDrawColor(0);
    u8g2->setFontDirection(0);
    u8g2->setFont(u8g2_font_6x10_tf);
    
    u8g2->drawXBM(5, 51, en_width, en_height, this->loadVideoBuffer(bw_bits, sizeof(bw_bits)/sizeof(bw_bits[0])));
    u8g2->drawXBM(112, 51, en_width, en_height, this->loadVideoBuffer(fw_bits, sizeof(fw_bits)/sizeof(fw_bits[0])));
    u8g2->drawXBM(38, 51, tt_width, tt_height, this->loadVideoBuffer(stop_bits, sizeof(stop_bits)/sizeof(stop_bits[0])));
    u8g2->drawXBM(76, 51, tt_width, tt_height, this->loadVideoBuffer(rec_bits, sizeof(rec_bits)/sizeof(rec_bits[0])));

    //Mode
    u8g2->drawStr(2,10,"Record");
    if(pidMode)
    {
      u8g2->drawStr(45,10,"PID ON");
    }
    else
    {
      u8g2->drawStr(45,10,"PID OFF");
    }
    u8g2->setFontMode(1);
    u8g2->setDrawColor(1);
    if(recorded)
    {
      sBuf = "Position ";
      sBuf += index;
      sBuf += " recorded";
      sBuf.toCharArray(buf, 22);
      u8g2->drawStr(2,35,buf);
    }
    else
    {
    sBuf = "Position: ";
    sBuf += (int32_t)(pos/this->resolution);
    sBuf += " mm";
    sBuf.toCharArray(buf, 22);
    u8g2->drawStr(2,35,buf);
    }
  } while ( u8g2->nextPage() );  
}

void egoShield::playPage(bool loopMode, bool pidMode, uint8_t index, bool mode)
{
  char buf[5];//char array buffer

  u8g2->firstPage();
  do 
  {
    u8g2->drawBox(1, 1, 128, 12);
    u8g2->drawBox(1, 48, 128, 68);
    u8g2->setFontMode(0);
    u8g2->setDrawColor(0);
    u8g2->setFontDirection(0);
    u8g2->setFont(u8g2_font_6x10_tf);

    if(loopMode)
    {
      u8g2->drawXBM(110, 2, loop_width, loop_height, this->loadVideoBuffer(loop_bits, sizeof(loop_bits)/sizeof(loop_bits[0])));
    }
    
    //Bottom bar
    u8g2->drawXBM(5, 51, en_width, en_height, this->loadVideoBuffer(bw_bits, sizeof(bw_bits)/sizeof(bw_bits[0]))); 
    u8g2->drawXBM(112, 51, en_width, en_height, this->loadVideoBuffer(fw_bits, sizeof(fw_bits)/sizeof(fw_bits[0]))); 
    u8g2->drawXBM(32, 50, play_width, play_height, this->loadVideoBuffer(play_bits, sizeof(play_bits)/sizeof(play_bits[0])));  
    u8g2->drawXBM(43, 51, tt_width, tt_height, this->loadVideoBuffer(stop_bits, sizeof(stop_bits)/sizeof(stop_bits[0])));  
    u8g2->drawXBM(77, 51, tt_width, tt_height, this->loadVideoBuffer(pse_bits, sizeof(pse_bits)/sizeof(pse_bits[0]))); 

    //Mode
    u8g2->drawStr(2,10,"Play");
    if(pidMode)
    {
      u8g2->drawStr(45,10,"PID ON");
    }
    else
    {
      u8g2->drawStr(45,10,"PID OFF");
    }
    u8g2->setFontMode(1);
    u8g2->setDrawColor(1);
    if(mode)
    {
      u8g2->drawStr(2,25,"Adjust velocity");
    }
    else
    {
      u8g2->drawStr(2,25,"Moving to pos");
      String(index).toCharArray(buf, 5);
      u8g2->drawStr(90,25,buf);
    }
    u8g2->drawStr(2,40,"Speed:");
    String(this->velocity).toCharArray(buf, 5);
    u8g2->drawStr(60,40,buf);
  } while ( u8g2->nextPage() );
}

void egoShield::pausePage(bool loopMode, bool pidMode, uint8_t index)
{
  char buf[3];//char array buffer

  u8g2->firstPage();
  do 
  {
    u8g2->drawBox(1, 1, 128, 12);
    u8g2->drawBox(1, 48, 128, 68);
    u8g2->setFontMode(0);
    u8g2->setDrawColor(0);
    u8g2->setFontDirection(0);
    u8g2->setFont(u8g2_font_6x10_tf);
    
    if(loopMode)
    {
      u8g2->drawXBM(110, 2, loop_width, loop_height, this->loadVideoBuffer(loop_bits, sizeof(loop_bits)/sizeof(loop_bits[0])));
    }
    
    //Bottom bar
    u8g2->drawXBM(32, 50, play_width, play_height, this->loadVideoBuffer(play_bits, sizeof(play_bits)/sizeof(play_bits[0])));
    u8g2->drawXBM(43, 51, tt_width, tt_height, this->loadVideoBuffer(stop_bits, sizeof(stop_bits)/sizeof(stop_bits[0])));

    //Mode
    u8g2->drawStr(2,10,"Pause");
    if(pidMode)
    {
      u8g2->drawStr(45,10,"PID ON");
    }
    else
    {
      u8g2->drawStr(45,10,"PID OFF");
    }
    u8g2->setFontMode(1);
    u8g2->setDrawColor(1);
    u8g2->drawStr(2,35,"Paused at pos");
    String(index).toCharArray(buf, 3);
    u8g2->drawStr(90,35,buf);
  } while ( u8g2->nextPage() );  
}


void egoShield::timePage(uint8_t step, bool pidMode)
{
  char buf[22];//char array buffer
  String sBuf;

  u8g2->firstPage();
  do 
  {
    u8g2->drawBox(1, 1, 128, 12);
    u8g2->drawBox(1, 48, 128, 68);
    u8g2->setFontMode(0);
    u8g2->setDrawColor(0);
    u8g2->setFontDirection(0);
    u8g2->setFont(u8g2_font_6x10_tf);

    if(step == 0)//we are waiting for the distance interval to be put in
    {
      u8g2->drawXBM(5, 51, en_width, en_height, this->loadVideoBuffer(bw_bits, sizeof(bw_bits)/sizeof(bw_bits[0])));
      u8g2->drawXBM(112, 51, en_width, en_height, this->loadVideoBuffer(fw_bits, sizeof(fw_bits)/sizeof(fw_bits[0])));
      u8g2->drawXBM(76, 51, tt_width, tt_height, this->loadVideoBuffer(rec_bits, sizeof(rec_bits)/sizeof(rec_bits[0])));
      u8g2->setFontMode(1);
      u8g2->setDrawColor(1);
      u8g2->drawStr(115,24,"<-");
      u8g2->setFontMode(0);
      u8g2->setDrawColor(0);
    }
    else if(step == 1)//we are waiting for the time interval to be put in
    {
      u8g2->drawXBM(5, 51, en_width, en_height, this->loadVideoBuffer(bw_bits, sizeof(bw_bits)/sizeof(bw_bits[0])));
      u8g2->drawXBM(112, 51, en_width, en_height, this->loadVideoBuffer(fw_bits, sizeof(fw_bits)/sizeof(fw_bits[0])));
      u8g2->drawXBM(76, 51, tt_width, tt_height, this->loadVideoBuffer(rec_bits, sizeof(rec_bits)/sizeof(rec_bits[0])));
      u8g2->setFontMode(1);
      u8g2->setDrawColor(1);
      u8g2->drawStr(115,34,"<-");
      u8g2->setFontMode(0);
      u8g2->setDrawColor(0);
    }
    else if(step == 2)//we are waiting for play to be issued
    {
      u8g2->drawXBM(32, 50, play_width, play_height, this->loadVideoBuffer(play_bits, sizeof(play_bits)/sizeof(play_bits[0])));
      u8g2->drawXBM(38, 51, tt_width, tt_height, this->loadVideoBuffer(stop_bits, sizeof(stop_bits)/sizeof(stop_bits[0])));
    }
    else if(step == 3)//we are playing sequence until end of rail
    {
      u8g2->drawXBM(38, 51, tt_width, tt_height, this->loadVideoBuffer(stop_bits, sizeof(stop_bits)/sizeof(stop_bits[0])));
    }

    u8g2->drawStr(2,10,"Time");
    if(pidMode)
    {
      u8g2->drawStr(45,10,"PID ON");
    }
    else
    {
      u8g2->drawStr(45,10,"PID OFF");
    }
    u8g2->setFontMode(1);
    u8g2->setDrawColor(1);
    sBuf = "Stepsize:  ";
    sBuf += stepSize;
    sBuf += " mm";
    sBuf.toCharArray(buf, 22);
    u8g2->drawStr(2,24,buf);
    sBuf = "Interval:  ";
    sBuf += interval*0.001;
    sBuf += " s";
    sBuf.toCharArray(buf, 22);
    u8g2->drawStr(2,34,buf); 
    sBuf = "Encoder:   ";
    sBuf += (int32_t)(stepper.encoder.getAngleMoved()/resolution);
    sBuf += " mm";
    sBuf.toCharArray(buf, 22);
    u8g2->drawStr(2,44,buf);
  } while ( u8g2->nextPage() );  
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