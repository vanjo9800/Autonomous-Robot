#include <ESP8266WiFi.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <stdint.h>

extern "C" {
  #include <espnow.h>
  #include <user_interface.h>
}

uint8_t WIFI_DEFAULT_CHANNEL=13;
uint8_t mac[] = {0x5C,0xCF,0x7F,0x11,0x5C,0x99};
uint8_t keysESPNOW[] = {0x13,0xAE,0x25,0x2C,0xCD,0x17,0xFB,0x7F,0x0E,0x57,0x93,0x37,0xEB,0x5C,0xF0,0xBF};
uint8_t keysEncrypt[] = {0x70,0x0A,0x3A,0xBC,0x03,0x66,0x20,0x7B,0xBA,0x68,0xB4,0x48,0x3C,0xB9,0xD4,0xC8};
long keyPhone[]={-1640232294,1108964966,1434435373,901639973};

WiFiUDP mobileApp;

const long DELTA=0x9e3779b9;
  
void xxtea(long *v, int n,long key[4]) {
  long y, z, sum;
  long p, rounds, e;
  if (n > 1) {          /* Coding Part */
    rounds = 6 + 52/n;
    sum = 0;
    z = v[n-1];
    do {
      sum += DELTA;
      e = (sum >> 2) & 3;
      for (p=0; p<n-1; p++) {
        y = v[p+1]; 
        z = v[p] += (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (key[(p&3)^e] ^ z)));
      }
      y = v[0];
      z = v[n-1] += (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (key[(p&3)^e] ^ z)));
    } while (--rounds);
  } else if (n < -1) {  /* Decoding Part */
    n = -n;
    rounds = 6 + 52/n;
    sum = rounds*DELTA;
    y = v[0];
    do {
      e = (sum >> 2) & 3;
      for (p=n-1; p>0; p--) {
        z = v[p-1];
        y = v[p] -= (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (key[(p&3)^e] ^ z)));
      }
      z = v[n-1];
      y = v[0] -= (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (key[(p&3)^e] ^ z)));
      sum -= DELTA;
    } while (--rounds);
  }
}

void blink()
{
    digitalWrite(BUILTIN_LED,HIGH);
    delay(1000);
    digitalWrite(BUILTIN_LED,LOW);
    delay(200);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP("SumoRobot", "ZumoShield", 1, 0);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED,LOW);

  Wire.begin();
  mobileApp.begin(6259);
  
  if (esp_now_init() == 0) {
    Serial.println("init");
  } else {
    Serial.println("init failed");
    ESP.restart();
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb([](uint8_t *macaddr, uint8_t *data, uint8_t len) {
    Serial.print("Message: ");
    Wire.beginTransmission(7);
    for(int i=0;i<len;i++){
      Wire.write(data[i]);
    }
    Wire.endTransmission();
  });
  esp_now_register_send_cb([](uint8_t* macaddr, uint8_t status) {
    Serial.println("Sending...");
  });

  while(esp_now_add_peer(mac, (uint8_t)ESP_NOW_ROLE_CONTROLLER,(uint8_t)WIFI_DEFAULT_CHANNEL, keysESPNOW,16)){
    blink();
  }
  while(esp_now_set_kok(keysEncrypt,16)){
    blink();
  }
}

char buff[UDP_TX_PACKET_MAX_SIZE];
uint8_t myPhoneMac[]={0xC4,0x3A,0xBE,0x9C,0xE5,0x33};

long number(String s,int &pos){
  int sign=0;
  long ans=0;
  if(s[pos]=='-'){
    sign=1;
    pos++;
  }
  while(s[pos]>='0'&&s[pos]<='9'){
    ans*=10;
    ans+=(s[pos]-'0');
    pos++;
  }
  if(sign){
    ans=-ans;
  }
  return ans;
}

void loop() {
  uint16_t length=mobileApp.parsePacket();
  if(length>0){
    mobileApp.read(buff,length);
    uint8_t myPhoneIP[4];
    uint16_t number_client= wifi_softap_get_station_num(); // Count of stations which are connected to ESP8266 soft-AP
    struct station_info *stat_info = wifi_softap_get_station_info();
    struct ip_addr *IPaddress;
    struct IPAddress address;
    
    while (stat_info != NULL) {
      IPaddress = &stat_info->ip;
      address = IPaddress->addr;
      bool equal=true;
      for(uint8_t i=0;i<6;i++){
        if((stat_info->bssid[i])!=myPhoneMac[i]){
          equal=false;
          break;
        }
      }
      if(equal){
        for(uint8_t i=0;i<4;i++){
          myPhoneIP[i]=address[i];
        }
      }
      stat_info = STAILQ_NEXT(stat_info, next);
    }

    if(mobileApp.remoteIP()==myPhoneIP){
      Serial.println("From my phone: "+String(buff)+"end of buffer");
      int pos=0;
      String data=String(buff);
      long encrypted[3];
      encrypted[0]=number(data,pos);
      pos++;
      encrypted[1]=number(data,pos);
      pos++;
      encrypted[2]=number(data,pos);
      xxtea(encrypted,-3,keyPhone);
      Wire.beginTransmission(7);
      for(int i=0;i<3;i++){
        if(i>0){
          Wire.write((encrypted[i]<0));
        }
        Wire.write(abs(encrypted[i]));
      }
      Wire.endTransmission();
      Serial.println("Decoded: "+String(encrypted[0])+" "+String(encrypted[1])+" "+String(encrypted[2]));
    }
  }
}


