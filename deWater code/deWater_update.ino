#include "Smartconfig.h"
#define WATER_PIN 35
#define ELEC_PIN 33
#define CONTROL 27
String name_version = "8";
String ServerName = "https://iot.pronest.asia/api/v1/bzMDIpGlY1iouiZkNXIM/firmware?title=OTAHP&version=";
// updatefirmware
#define DEVICE "PRO-HP-Dev"
#define TOKEN "bzMDIpGlY1iouiZkNXIM"
#define THINGSBOARD_SERVER  "iot.pronest.asia"

WiFiClient wifiClient;
int status = WL_IDLE_STATUS;
PubSubClient client(wifiClient);
// Thingsboard MQTT
boolean system_error = false;
String wmData_previos = "";
long wData[100];
long mADC[500];
String wState;
String mState;
unsigned long mSum[5];
int mcount_j = 0; //moi jam
int mcount_f = 0; //moi off
int count;
unsigned long interval = 30000;
unsigned long previousMillis = 0;
unsigned int counter = 0;
int state = 1;
int deWater() {
  int value;
  // put your main code here, to run repeatedly:
  for (uint8_t i = 0; i < 100; i++) {
    wData[i] = analogRead(WATER_PIN);
    delay(5);
  }
  for (uint8_t i = 0; i < 100; i++) {
    if (wData[i] > 1000) count++;
  }
  if ( count > 15 ) {
    digitalWrite(CONTROL, HIGH); //TURN ON CONTROL WHEN WATER DOESNT LEAK
    value = 1;
  }
  else {
    digitalWrite(CONTROL, LOW); //TURN OFF CONTROL WHEN WATER LEAKS
    value = 0;
  }
  count = 0;

  return value;
}

int checkMoi() {
  int mvalue;
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 500; j++) {
      mADC[j] = analogRead(ELEC_PIN);
      delayMicroseconds(167);
      mSum[i] = mSum[i] + mADC[j];
    }
    mSum[i] = mSum[i] / 500;
    if (mSum[i] > 290) mcount_j++;
    else if (mSum[i] < 20) mcount_f++;
    mSum[i] = 0;
  }
  if (mcount_j > 4) mvalue = 1; //notOK
  else if (mcount_f > 4) mvalue = 2; //notReady
  else mvalue = 3; //OK
  mcount_f = mcount_j = 0;
  return mvalue;
}

void setup() {
  pinMode(ELEC_PIN, INPUT);
  pinMode(WATER_PIN, INPUT);
  Serial.begin(115200);
  pinMode(CONTROL, OUTPUT);
  digitalWrite(CONTROL, LOW);
  pinMode(SMART_CONFIG, INPUT);
  pinMode(PIN_LED, OUTPUT);
  LED_OFF();
  read_EEPROM();
  delay(3000);
  client.setServer(THINGSBOARD_SERVER, 1883);
  client.setCallback(callback);
  if (WiFi.status() == WL_CONNECTED) {
    reconnect();
  }
  delay(2000);
}
void loop() {
  if (!system_error)
  {
    if (longPress()) { // Gọi hàm longPress kiểm tra trạng thái button
      enter_smartconfig(); // Nếu button được nhấn giữ trong 3s thì vào trạng thái smartconfig
    }
    if (WiFi.status() == WL_CONNECTED && WiFi.smartConfigDone()) { //Kiểm tra trạng thái kết nối wifi,
      Serial.println("exit smart");
      exit_smart();
      if (WiFi.status() == WL_CONNECTED ) {
        reconnect();
      }
      check_config = false;
    }
    if (WiFi.status() == WL_CONNECTED && check_config == false ) {
      reconnect();
    }
    client.loop();
    if (millis() - previousMillis > interval) {
      previousMillis = millis();
      String data_power =  "{\"state\":\"true\"}";
      Serial.println(data_power);
      client.publish("v1/devices/me/telemetry", data_power.c_str());
      String data_alert =  "{\"data_alert\":\"false\"}";
      Serial.println(data_alert);
      client.publish("v1/devices/me/telemetry", data_alert.c_str());
    }
    if (deWater()) wState = "false";
    else wState = "true";
    if (checkMoi() == 3) mState = "true";
    else if (checkMoi() == 1) mState = "false";// ERROR
    else if (checkMoi() == 2) mState = "false";// Not Ready
    String wmData = "{\"data_waterLeak\":\"" + wState + "\"," + "\"data_state\":\"" + mState + "\"}";
    if (wmData != wmData_previos) {
      Serial.println(wmData);
      client.publish("v1/devices/me/telemetry", wmData.c_str());
      wmData_previos = "{\"data_waterLeak\":\"" + wState + "\"," + "\"data_state\":\"" + mState + "\"}";
      delay(100);
    }
    if (!deWater())
    { wState = "true";
    }
    else
    {
      wState = "false";
      delay(100);
      int retry = 0;
      if (checkMoi() == 1)
      {
        while (retry < 3)
        { digitalWrite(CONTROL, LOW);
          delay(2000);
          digitalWrite(CONTROL, HIGH);
          retry++;
          if (checkMoi() == 3)
          {
            break;
          }
        }
        if (checkMoi() != 3)
        {
          String state = "{\"data_alert\":\"true\"}";
          client.publish("v1/devices/me/telemetry", state.c_str());
          digitalWrite(CONTROL, LOW);
          system_error = true;
        }
      }
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
      }
    }
    if (client.connect(DEVICE, TOKEN, NULL)) {
      LED_ON();
      Serial.println("Connected Thingsboard");
      String data_power =  "{\"state\":\"true\"}";
      Serial.println(data_power);
      client.publish("v1/devices/me/telemetry", data_power.c_str());
      client.subscribe("v1/devices/me/rpc/request/+");//dang ky nhan cas lenh rpc tu cloud
      client.subscribe("v1/devices/me/attributes");
    }
    else {
      LED_OFF();
      delay( 1000 );
    }
  }
}

void callback(const char* topic, byte * payload, unsigned int length) {
  Serial.println("Received Data");
  char json[length + 1];
  strncpy (json, (char*)payload, length);
  json[length] = '\0';
  Serial.println((char *)json);
  JSONVar object = JSON.parse((char *)json);
  String fw_version = "";
  fw_version = object["fw_version"];
  Serial.println(name_version);
  Serial.println(fw_version);
  if (name_version != fw_version)
  {
    String New_ServerName = ServerName + fw_version ;
    Serial.println("update new version ");
    Serial.println(New_ServerName);
    ESPhttpUpdate.update(New_ServerName);
  }
}