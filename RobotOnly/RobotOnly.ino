#include <ZumoMotors.h>
#include <Pushbutton.h>
#include <QTRSensors.h>
#include <ZumoReflectanceSensorArray.h>
#include <Ultrasonic.h>

Ultrasonic ultrasonic(3,2);//pins to which the sonar is connected
#define LED 13

#define QTR_THRESHOLD  1500
  
#define REVERSE_SPEED     200
#define TURN_SPEED        200
#define FORWARD_SPEED     200
#define REVERSE_DURATION  200
#define TURN_DURATION     300
 
ZumoMotors motors;
Pushbutton button(ZUMO_BUTTON);
 
#define NUM_SENSORS 6
unsigned int sensor_values[NUM_SENSORS];
 
ZumoReflectanceSensorArray sensors(QTR_NO_EMITTER_PIN);

//Open-source code provided from Pololu for controlling a servo with their Zumo Shield: https://www.pololu.com/docs/0J57/8.a
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

void waitForButton()
{
  digitalWrite(LED, HIGH);
  button.waitForButton();
  digitalWrite(LED, LOW);
}

int16_t lastL,lastR;

void setup()
{
  Serial.begin(9600);
  pinMode(LED, HIGH);
  servoInit();
  servoSetPosition(1500);
  waitForButton();
  delay(1000);
  checkForOpponent();
}

bool whetherCheckForOpponent=false;

void KeepInPlayArea()
{
  whetherCheckForOpponent=false;
  sensors.read(sensor_values);
  if (sensor_values[2] < QTR_THRESHOLD)
  {
    motors.setSpeeds(200,-200);
    lastL=200;
    lastR=-200;
    delay(800);
    motors.setSpeeds(0,0);
    lastL=0;
    lastR=0;
    whetherCheckForOpponent=true;
  }
  else if (sensor_values[3] < QTR_THRESHOLD)
  {
    motors.setSpeeds(-200,200);
    lastL=-200;
    lastR=200;
    delay(800);
    motors.setSpeeds(0,0);
    lastL=0;
    lastR=0;
    whetherCheckForOpponent=true;
  }
}

bool check(){
  long minimalDist=(1<<14),place;
  for(int i=1000;i<2001;i+=100)
  {
    servoSetPosition(i);
    delay(10);
    long microsec=ultrasonic.timing();
    delay(10);
    long dist=ultrasonic.convert(microsec,Ultrasonic::CM);
    if(minimalDist>dist){
      minimalDist=dist;
      place=i;
    }
  }
  if(minimalDist<70){
    servoSetPosition(1500);
    motors.setSpeeds((place-1500)/2.5,(1500-place)/2.5);
    lastL=(place-1500)/2.5;
    lastR=(1500-place)/2.5;
    delay(400);
    motors.setSpeeds(0,0);
    lastL=0;
    lastR=0;
    delay(100);
    motors.setSpeeds(200,200);
    lastL=200;
    lastR=200;
    return true;
  }
  if(minimalDist>70&&minimalDist<100)
  {
    servoSetPosition(1500);
    delay(100);
    motors.setSpeeds(200,200);
    delay(400);
    return check();
  }
  return false;
}

void checkForOpponent()
{
  servoSetPosition(1500);
  if(check()) return;
  servoSetPosition(1500);
  delay(100);
  sensors.read(sensor_values);
  if (sensor_values[2] < QTR_THRESHOLD)
  {
    motors.setSpeeds(200,-200);
    lastL=200;
    lastR=-200;
    delay(800);
  }
  else if (sensor_values[3] < QTR_THRESHOLD)
  {
    motors.setSpeeds(-200,200);
    lastL=-200;
    lastR=200;
    delay(800);
  }
  motors.setSpeeds(0,0);
  lastL=0;
  lastR=0;
  if(check()) return;
  motors.setSpeeds(0,0);
  lastL=0;
  lastR=0;
  waitForButton();
}

int16_t counter=0,roll=0,pitch=0,mode=0,sign=0;

void loop()
{
  if (button.isPressed()){
    motors.setSpeeds(0,0);
    lastL=0;
    lastR=0;
    button.waitForRelease();
    waitForButton();
  }
  if(counter>0){
    counter--;
    if(counter==0){
      motors.setSpeeds(lastL,lastR);
    }
  }
  KeepInPlayArea();
  if(whetherCheckForOpponent)
  {
    checkForOpponent();
  }
}
