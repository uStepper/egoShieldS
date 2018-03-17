#include <egoShieldTimeLapse.h>

#define ACCELERATION 4000 //Acceleration used during PLAY
#define VELOCITY 2000 //Velocity used during PLAY
#define MICROSTEPPING SIXTEEN //Microstepping according to jumpers installed
#define TOLERANCE 10 //Tolerance of PID in steps, i.e. how much error is allowed before correction
#define HYTERESIS 5 //Hysteresis of PID in steps, i.e. when is the PID deactivated again
#define P 1.0 //P-term of the PID
#define I 0.1 //I-term of the PID
#define D 0.05 //D-term of the PID
#define RESOLUTION 5 // Degree per mm of travel

egoShield ego;

void setup() {
  // put your setup code here, to run once:
  ego.setup(ACCELERATION,VELOCITY,MICROSTEPPING,TOLERANCE,HYTERESIS,P,I,D,RESOLUTION,250);
}

void loop() {
  // put your main code here, to run repeatedly:
  ego.loop();
}