#include <egoShieldS.h>

#define ACCELERATION 4000 //Acceleration used during PLAY
#define VELOCITY 4000 //Velocity used during PLAY, Manual and Homing
#define P 10.0 //P-term of the PID
#define I 0.1 //I-term of the PID
#define D 0.05 //D-term of the PID
#define RESOLUTION 5 // Degree per mm of travel
#define STALLSENSITIVITY 2 //sensitivity of the stall detection, between -64 and 63 - higher number is less sensitive - TO DISABLE HOMING SET TO 100!

egoShieldTeach ego;

void setup() {
  // put your setup code here, to run once:
  ego.setup(ACCELERATION,VELOCITY,P,I,D,RESOLUTION,STALLSENSITIVITY,250);//the final value is shutter delay for ego-shield timelapse - TO DISABLE HOMING SET TO 100!
}

void loop() {
  // put your main code here, to run repeatedly:
  ego.loop();
}