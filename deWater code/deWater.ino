#include <lorawan.h>
#include <string.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<Arduino_JSON.h>

#define MOI_PIN 14
#define WATER_PIN 33
#define ELEC_PIN 26
#define RELAY 13
#define RELAY2 12

JSONVar case_RL1 = "relayState_1", case_RL2 = "relayState_2";
JSONVar state_ON = "on", state_OFF = "off";
JSONVar setTime = "interval";
byte wData[100];
long mADC[1000];
unsigned long mSum[5];
int mcount_j = 0; //moi jam
int mcount_f = 0; //moi off
int count;
const char *devEui = "3d954a9518f1dbc8";
const char *appEui = "0000000000000000";
const char *appKey = "d7332718ff31e2539cb76ad07e6f8b76";

unsigned long interval = 5000;    
unsigned long previousMillis = 0;  
unsigned int counter = 0;    
int state = 1;
String wState;
String mState;
String wmData;
char outStr[255];
byte recvStatus = 0;
const sRFM_pins RFM_pins = {
  .CS = 5,
  .RST = 4,
  .DIO0 = 15,
  .DIO1 = 2,
  .DIO2 = 21,
  .DIO5 = 22,
};
int deWater() {
  int value;
  // put your main code here, to run repeatedly:
    for(uint8_t i=0; i<100;i++){
          wData[i] = analogRead(WATER_PIN);
          delay(10);
        }
     for(uint8_t i=0; i<100;i++){
            if(wData[i]>0) count++;
        }
      if( count > 30 ) {
       digitalWrite(RELAY,HIGH); //TURN ON RELAY WHEN WATER DOESNT LEAK
       value = 1;
       }
      else {
        digitalWrite(RELAY,LOW); //TURN OFF RELAY WHEN WATER LEAKS
       value = 0;  
       }
       count = 0;
       return value;
  }

int checkMoi(){
  int mvalue;
  for(int i=0;i<5;i++){
    for(int j=0; j<1000;j++){
      mADC[j] = analogRead(MOI_PIN);
      delayMicroseconds(167);
      mSum[i]= mSum[i] + mADC[j];
    }
    mSum[i] = mSum[i] / 1000;
    if(mSum[i]>502) mcount_j++; 
    else if(mSum[i]<10) mcount_f++; 
    mSum[i]=0;
   }
  if(mcount_j>4) mvalue = 1; //notOK
  else if(mcount_f>4) mvalue = 2; //notReady
  else mvalue = 3; //OK
  mcount_f=mcount_j=0;
  return mvalue;
  }

  
//void checkMoiii(){
//  for(int i=0;i<5;i++){
//    for(int j=0; j<1000;j++){
//      mADC[j] = analogRead(MOI_PIN);
//      delayMicroseconds(167);
//      mSum[i]= mSum[i] + mADC[j];
//    }
//    mSum[i] = mSum[i] / 1000;
//    Serial.println(mSum[i]);
//    mSum[i]=0;
//    }
//  }
void setup() {
  pinMode(MOI_PIN,INPUT);
  Serial.begin(115200);
  pinMode(RELAY, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  while(!Serial);
  if(!lora.init()){
    Serial.println("RFM95 not detected");
    delay(5000);
    return;
  }

  lora.setDeviceClass(CLASS_C);
  lora.setDataRate(0x02);
  lora.setChannel(MULTI);
  lora.setDevEUI(devEui);
  lora.setAppEUI(appEui);
  lora.setAppKey(appKey);
  bool isJoined;
  do {
    Serial.println("Joining...");
    isJoined = lora.join();
    delay(10000);
  }while(!isJoined);
  Serial.println("Joined to network");
}

void loop() {
  if((millis() - previousMillis) > interval) {
    previousMillis = millis(); 
    Serial.print("Sending: ");
    if(deWater()) wState = "True";
    else wState = "False";
    if(checkMoi()==3) mState = "Good";
    else if(checkMoi()==1) mState = "Error";
    else if(checkMoi()==2) mState = "Not ready";
    wmData = "{\"waterleak\":" + wState+ "," + "\"state\":" + mState + "}";
    int str_len = wmData.length() + 1; 
    char Data[str_len];
    wmData.toCharArray(Data, str_len);
    Serial.println((char *)Data); 
    lora.sendUplink(Data, strlen(Data), 0, 1);
  }
    recvStatus = lora.readData(outStr);
    if(recvStatus) {
      Serial.println(outStr);
      JSONVar object = JSON.parse(outStr);
      JSONVar key = object.keys();
        if(key[0]==case_RL1){
          if(object[key[0]] == state_ON) digitalWrite(RELAY, HIGH); //TURN ON RELAY 1
          else if (object[key[0]] == state_OFF) digitalWrite(RELAY, LOW);
          }
         else if(key[0]==case_RL2){
          if(object[key[0]] == state_ON) digitalWrite(RELAY2, HIGH); //TURN ON RELAY 2
           else if (object[key[0]] == state_OFF) digitalWrite(RELAY2, LOW);
          }
         else if(key[0]==setTime)  interval = long(object[key[0]])*1000; //SET INTERVAL TIME
    }
    lora.update();
}