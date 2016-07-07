#include <ESP8266WiFi.h>
#include <Wire.h>

extern "C" {
  #include <espnow.h>
  #include <user_interface.h>
}

const uint8_t WIFI_ESPNOW_DEFAULT_CHANNEL=;
uint8_t mac[] = {};
uint8_t keysESPNOW[] = {};
uint8_t keysEncrypt[] = {};

const uint8_t MPU_addr=0x68;

const uint8_t buttonGroundPin=D0;
uint8_t lastButtonState=LOW;

const uint8_t greenLightPin=D6;
const uint8_t blueLightPin=D5;
bool gestureControl=false;

uint8_t statusCommand;

uint8_t mode=0;

void blink()
{
    digitalWrite(BUILTIN_LED,HIGH);
    delay(1000);
    digitalWrite(BUILTIN_LED,LOW);
    delay(200);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init()==0) {
    Serial.println("direct link  init ok");
  } else {
    Serial.println("dl init failed");
    ESP.restart();
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_recv_cb([](uint8_t *macaddr, uint8_t *data, uint8_t len) {
    Serial.println("Receiving...");
  });
  esp_now_register_send_cb([](uint8_t *macaddr, uint8_t status) {
    Serial.println("Sending: ");
    Serial.print("status = "); Serial.println(status);
    statusCommand=status;
  });

  pinMode(buttonGroundPin,INPUT);
  pinMode(greenLightPin,OUTPUT);
  pinMode(blueLightPin,OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED,LOW);

  while(esp_now_add_peer(mac,(uint8_t)ESP_NOW_ROLE_SLAVE,WIFI_ESPNOW_DEFAULT_CHANNEL,keysESPNOW, 16)){
    blink();
  }
  while(esp_now_set_kok(keysEncrypt,16)){
    blink();
  }

  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  delay(10);
}

void getValues(int16_t &pitch,int16_t &roll){
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true);
  int16_t AcX=Wire.read()<<8|Wire.read();
  int16_t AcY=Wire.read()<<8|Wire.read();
  int16_t AcZ=Wire.read()<<8|Wire.read();
  int16_t Tmp=Wire.read()<<8|Wire.read();
  int16_t GyX=Wire.read()<<8|Wire.read();
  int16_t GyY=Wire.read()<<8|Wire.read();
  int16_t GyZ=Wire.read()<<8|Wire.read();
  roll=atan2(AcY,AcZ)*180/PI;
  pitch=atan2(AcX,sqrt(AcY*AcY+AcZ*AcZ))*180/PI;
}

uint8_t sign(int16_t number){
  return number<0;
}

uint16_t counter=0;

void loop() {
  uint8_t currentButtonState=digitalRead(buttonGroundPin);
  //Serial.println(mode);
  //delay(100);
  if(currentButtonState!=lastButtonState)
  {
    if(currentButtonState==LOW)
    {
      mode++;
      mode%=3;
      if(mode==1)
      {
        digitalWrite(greenLightPin,HIGH);
      }
      else
      {
        digitalWrite(greenLightPin,LOW);
      }
      if(mode==2)
      {
        digitalWrite(blueLightPin,HIGH);
      }
      else
      {
        digitalWrite(blueLightPin,LOW);
      }
    }
    lastButtonState=currentButtonState;
  }
  counter++;
  if(counter==1000){
    counter=0;
    int16_t pitch,roll;
    getValues(pitch,roll);
    Serial.println("Pitch:"+String(pitch)+" Roll:"+String(roll));
    uint8_t message[] = {mode,sign(pitch),abs(pitch),sign(roll),abs(roll)};
    statusCommand=1;
    for(int i=0;i<10&&mode!=0&&statusCommand;i++){
      esp_now_send(mac, message, sizeof(message));
      delay(100);
    }
  }
}

