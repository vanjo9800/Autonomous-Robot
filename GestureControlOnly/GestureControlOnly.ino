#include<Wire.h>
#include<ZumoMotors.h>

//Open-source code provided from Pololu for controlling a Servo with their Zumo Shield: https://www.pololu.com/docs/0J57/8.a
#define SERVO_PIN 6
 
// This is the time since the last rising edge in units of 0.5us.
uint16_t volatile servoTime = 0;
 
// This is the pulse width we want in units of 0.5us.
uint16_t volatile servoHighTime = 3000;
 
// This is true if the servo pin is currently high.
boolean volatile servoHigh = false;
 
// This ISR runs after Timer 2 reaches OCR2A and resets.
// In this ISR, we set OCR2A in order to schedule when the next
// interrupt will happen.
// Generally we will set OCR2A to 255 so that we have an
// interrupt every 128 us, but the first two interrupt intervals
// after the rising edge will be smaller so we can achieve
// the desired pulse width.
ISR(TIMER2_COMPA_vect)
{
  // The time that passed since the last interrupt is OCR2A + 1
  // because the timer value will equal OCR2A before going to 0.
  servoTime += OCR2A + 1;
   
  static uint16_t highTimeCopy = 3000;
  static uint8_t interruptCount = 0;
   
  if(servoHigh)
  {
    if(++interruptCount == 2)
    {
      OCR2A = 255;
    }
 
    // The servo pin is currently high.
    // Check to see if is time for a falling edge.
    // Note: We could == instead of >=.
    if(servoTime >= highTimeCopy)
    {
      // The pin has been high enough, so do a falling edge.
      digitalWrite(SERVO_PIN, LOW);
      servoHigh = false;
      interruptCount = 0;
    }
  } 
  else
  {
    // The servo pin is currently low.
     
    if(servoTime >= 40000)
    {
      // We've hit the end of the period (20 ms),
      // so do a rising edge.
      highTimeCopy = servoHighTime;
      digitalWrite(SERVO_PIN, HIGH);
      servoHigh = true;
      servoTime = 0;
      interruptCount = 0;
      OCR2A = ((highTimeCopy % 256) + 256)/2 - 1;
    }
  }
}
 
void servoInit()
{
  digitalWrite(SERVO_PIN, LOW);
  pinMode(SERVO_PIN, OUTPUT);
   
  // Turn on CTC mode.  Timer 2 will count up to OCR2A, then
  // reset to 0 and cause an interrupt.
  TCCR2A = (1 << WGM21);
  // Set a 1:8 prescaler.  This gives us 0.5us resolution.
  TCCR2B = (1 << CS21);
   
  // Put the timer in a good default state.
  TCNT2 = 0;
  OCR2A = 255;
   
  TIMSK2 |= (1 << OCIE2A);  // Enable timer compare interrupt.
  sei();   // Enable interrupts.
}
 
void servoSetPosition(uint16_t highTimeMicroseconds)
{
  TIMSK2 &= ~(1 << OCIE2A); // disable timer compare interrupt
  servoHighTime = highTimeMicroseconds * 2;
  TIMSK2 |= (1 << OCIE2A); // enable timer compare interrupt
}

const int fullCounter=40;
const int PhoneNegligiblePitch=2,PhoneNegligibleRoll=2;
const int PhonePitchMultiplier=19,PhoneRollMultiplier=19;
const int PhoneServoMultiplier=50;
const int GlovePitchMultiplier=4,GloveRollMultiplier=1;
const int GloveServoMultiplier=8;
const int EyesInit=1500;
const int WireNumber=7;

ZumoMotors motors;

void stop(){
	motors.setSpeeds(0,0);
}

void setup() {
  Serial.begin(9600);
  Wire.begin(WireNumber);
  Wire.onReceive(receiveEvent);
  servoInit();
  servoSetPosition(EyesInit);
  delay(1000);//time for eyes to turn and the gesture connection to be established
}

uint16_t counter=0;
int16_t roll,pitch,mode,sign;

void loop() {
  if(counter==0)
  {
	  stop();
  }
  else
  {
    counter--;
  }
  delay(20);
}

void GloveControl(int16_t pitch,int16_t roll){
	counter=fullCounter;
	motors.setSpeeds(GlovePitchMultiplier*pitch+GloveRollMultiplier*roll,GlovePitchMultiplier*pitch-GloveRollMultiplier*roll);
}

void PhoneControl(int16_t pitch,int16_t roll){
      if(roll>=-PhoneNegligibleRoll&&roll<=PhoneNegligibleRoll) roll=0;
      if(pitch>=-PhoneNegligiblePitch&&pitch<=PhoneNegligiblePitch) pitch=0;
      counter=fullCounter;
      motors.setSpeeds((-PhonePitchMultiplier)*pitch+(-PhoneRollMultiplier)*roll,(-PhonePitchMultiplier)*pitch+PhoneRollMultiplier*roll);
}

void receiveEvent(int info) {
  while (0 < Wire.available()) {
    mode=Wire.read();
    sign=Wire.read();
    roll=Wire.read();
    if(sign==1){
      roll=-roll;
    }
    sign=Wire.read();
    pitch=Wire.read();
    if(sign==1){
      pitch=-pitch;
    }
    if(mode==1)
    {
      servoSetPosition(ServoInit);
      GloveControl();
    }
    if(mode==2)
    {
      servoSetPosition(ServoInit+roll*GloveServoMultiplier;
    }
    if(mode==3)
    {
      servoSetPosition(ServoInit);
      PhoneControl();
    }
    if(mode==4)
    {
      servoSetPosition(ServoInit-roll*PhoneServoMultiplier);
    }
  }
}
