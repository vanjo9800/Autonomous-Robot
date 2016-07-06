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

ZumoMotors motors;

void setup() {
  Serial.begin(9600);
  Wire.begin(7);
  Wire.onReceive(receiveEvent);
  servoInit();
  servoSetPosition(1500);  // Send 1000us pulses.
  delay(1000);
}

uint16_t counter=0;
int16_t roll,pitch,mode,sign;

void loop() {
  if(counter==0)
  {
    motors.setSpeeds(0,0);
  }
  else
  {
    counter--;
  }
  delay(20);
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
      servoSetPosition(1500);
      if(pitch>roll&&pitch>0){
        roll-=(pitch-30)/1.5;
      }
      if(roll<pitch&&roll<0){
        pitch-=(-15-roll)*1.5;
      }
      if(roll<25&&roll>-15)
      {
        roll=0;
      }
      if(pitch<15&&pitch>-15)
      {
        pitch=0;
      }
      if(roll!=0||pitch!=0)
      {
        Serial.println(String(roll)+" "+String(pitch));
        Serial.println(String(4*pitch+roll*1.5)+" "+String(4*pitch-roll*1.5));
        counter=40;
        motors.setSpeeds(4*pitch+roll*1.5,4*pitch-roll*1.5);    
      }
    }
    if(mode==2)
    {
      servoSetPosition(1500+roll*8);
    }
    Serial.println(String(mode)+" "+String(pitch)+" "+String(roll));
    if(mode==3)
    {
      servoSetPosition(1500);
      if(roll>=-2&&roll<=2) roll=0;
      if(pitch>=-2&&pitch<=2) pitch=0;
      counter=40;
      motors.setSpeeds((-pitch-roll)*19,(-pitch+roll)*19);
    }
    if(mode==4)
    {
      servoSetPosition(1500-roll*50);
    }
  }
}
