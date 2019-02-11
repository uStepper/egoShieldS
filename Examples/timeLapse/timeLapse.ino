#include <egoShieldS.h>

#define ACCELERATION 4000 //Acceleration used during PLAY
#define VELOCITY 100 //Velocity used during PLAY
#define P 1.0 //P-term of the PID
#define I 0.1 //I-term of the PID
#define D 0.05 //D-term of the PID
#define RESOLUTION 5 // Degree per mm of travel

egoShieldTimeLapse ego;

void setup() {
  // put your setup code here, to run once:
  ego.setup(ACCELERATION,VELOCITY,P,I,D,RESOLUTION,250);
}

void loop() {
  // put your main code here, to run repeatedly:
  ego.loop();
}