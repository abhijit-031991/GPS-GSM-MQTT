#define TINY_GSM_MODEM_SIM800
#define ARDUINOJSON_USE_LONG_LONG 1

// Libraries //
#include <Arduino.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include <elapsedMillis.h>
#include <extEEPROM.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <Sleep_n0m1.h>
#include <TimeLib.h>
#include <PubSubClient.h>


// Definittions //

#define SerialMon Serial

#define SerialAT Serial1


// Library Definitions //

elapsedMillis Mtime;
elapsedMillis Btime;
TinyGPSPlus GPS;
extEEPROM eeprom(kbits_1024, 2, 256);

// ExternalEEPROM mem;

SoftwareSerial gpss(13, 12);
Sleep slp;

// Pin Definitions //

#define GPIN A1
#define IPIN 4
#define FPIN A0

// Device Details // 

const char ID[] = "ST101";
const char statReq[] = "{\"id\":\"ST101\", \"req\":1}";
const char setReq[] = "{\"id\":\"ST101\", \"req\":2}";

// GPS Control Variables //

int gpsTimeout = 60;    // GPS Timeout in seconds  *** USER CONFIG ***
int gpsFrequency = 1;  // GPS Frequency in Minutes *** USER CONFIG ***
int gpshdop = 5;           // GPS HDOP *** USER CONFIG ***

// GPS Storage Variables // 
double lat;
double lng;
unsigned int count; 

// Memory Variables //
unsigned long wAdd = 1;
unsigned long rAdd = 1;

// Transmission Control Variables //
int transInterval = 5; // Transmission Interval *** USER CONFIG ***

//GPRS credentials //
const char apn[]      = "airtelgprs.com";
const char gprsUser[] = "";
const char gprsPass[] = "";

// MQTT details ///
const char* broker = "13.233.227.254";

const char* telemetryTopic = "telemetry";
const char* statusSubTopic = "status";
const char* settingsSupTopic = "settings";



/// Extra ///

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient  mqtt(client);
uint32_t lastReconnectAttempt = 0;

/// Cyclic Variables ///

int cycle = 1;
int targetCycle;

//// activation Switch ////

bool activate = false;
bool ret = false;
bool activity = false;


//////////////////////////////
/////     FUNCTIONS      /////
//////////////////////////////

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  SerialMon.print("Message arrived [");
  SerialMon.print(topic);
  SerialMon.print("]: ");
  SerialMon.println();

  Serial.println(len);
  char x[len];
  strncpy(x, (char*)payload, len);
  String top = (char*)topic;
  Serial.println(x);
  Serial.println("DONE");

  if (top == "status")
  {
    Serial.println(F("Starting system"));
    StaticJsonDocument<85> doc;

    DeserializationError error = deserializeJson(doc, x);

    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }
    Serial.println((String)doc["active"]);

    if (doc["active"] == true)
    {
        Serial.println("Activating system");
        activate = true;
    }else
    {
        activate = false;
    }
  }

  if (top == "settings")
  {
        StaticJsonDocument<100> doc;

        DeserializationError error = deserializeJson(doc, x);

        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        }else{
          bool x = doc["NEW"];
          if (x == true)
          {
            gpshdop = doc["HDOP"]; // 5
            gpsFrequency = doc["GFRQ"]; // 1
            transInterval = doc["TFRQ"]; // 9
            gpsTimeout = doc["GTO"]; // 10

            Serial.println(gpshdop);
            Serial.println(gpsFrequency);
            Serial.println(gpsTimeout);
            Serial.println(transInterval);
          }else{
            Serial.println(F("No New Settings"));
          }
          
          
        }      
     
  }
  
  Serial.println(activate);
  targetCycle = transInterval/gpsFrequency;
  ret = true;  

}

boolean mqttConnect() {
  SerialMon.print("Connecting to ");
  SerialMon.print(broker);

  // Connect to MQTT Broker
  // boolean status = mqtt.connect("GsmClientTest");

  // Or, if you want to authenticate MQTT:
  boolean status = mqtt.connect(ID, "", "");
  mqtt.setKeepAlive(10000);

  if (status == false) {
    SerialMon.println(" fail");
    return false;
  }
  SerialMon.println(" success");
  mqtt.subscribe(settingsSupTopic);
  return mqtt.connected();

}

void networkInit(){
  modem.init();
  String modemInfo = modem.getModemInfo();
  SerialMon.print(F("Modem Info: "));
  SerialMon.println(modemInfo);

  SerialMon.print(F("Waiting for network..."));
  if (!modem.waitForNetwork()) {
    SerialMon.println(F("fail"));
    // delay(10000);
    return;
  }
  SerialMon.println(F("success"));

  if (modem.isNetworkConnected()) { 
    SerialMon.println(F("Network connected")); 

     // MQTT Broker setup
      mqtt.setServer(broker, 1883);
      mqtt.setCallback(mqttCallback);

      if (!modem.isGprsConnected()) {
          SerialMon.println(F("GPRS disconnected!"));
          SerialMon.print(F("Connecting to "));
          SerialMon.print(apn);
          if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
            SerialMon.println(F(" fail"));
            delay(10000);
            return;
          }
        }

      if (modem.isGprsConnected()){ 
        SerialMon.println(F("GPRS reconnected")); 
      }
      if (!mqtt.connected()) {
        SerialMon.println(F("=== MQTT NOT CONNECTED ==="));
        // Reconnect every 10 seconds
        uint32_t t = millis();
        if (t - lastReconnectAttempt > 10000L) {
          lastReconnectAttempt = t;
          if (mqttConnect()) { lastReconnectAttempt = 0; }
        }
        delay(100);
        return;
      }
  }else{
    SerialMon.println(F("No Network"));
  }

  
}

void RGPS() {
  struct main
  {
    double lat;
    double lng;
    time_t ts;
    int lt;
    int hdop;
    int spd;
    unsigned int cnt;
  }data;
  count = count + 1;
  int js = sizeof(data);
  Btime = 0;
  digitalWrite(GPIN, HIGH);
  while (Btime <= (unsigned)gpsTimeout*1000) {
    while (gpss.available()) {
      if (GPS.encode(gpss.read())) {
        if (!GPS.location.isValid()) {
          Serial.println(F("Not Valid"));          
        }else
        {   
          Serial.println(GPS.location.isUpdated());
          Serial.print(F("Location Age:"));
          Serial.println(GPS.location.age());
          Serial.print(F("Time Age:"));
          Serial.println(GPS.time.age());
          Serial.print(F("Date Age:"));
          Serial.println(GPS.date.age());
          Serial.print(F("Satellites:"));
          Serial.println(GPS.satellites.value());
          Serial.print(F("HDOP:"));
          Serial.println(GPS.hdop.hdop());
        }
      }
    }
    if (GPS.hdop.hdop() > 0.0 && GPS.hdop.hdop() < gpshdop && GPS.location.age() < 3000)
    { 
      Serial.println(F("breaking"));
      break;
    }
    
  }
  digitalWrite(GPIN, LOW);
  
  if (GPS.location.age() < 60000)
  {
    lat = GPS.location.lat();
    // values[F("Lat")] = GPS.location.lat();
    lng = GPS.location.lng();
    // values[F("Lng")] = GPS.location.lng();
  } else{
    lat = 502;
    lng = 502;
  }  
  setTime(GPS.time.hour(),GPS.time.minute(),GPS.time.second(),GPS.date.day (), GPS.date.month (),GPS.date.year());
  time_t t = now();
  Serial.println(t);
  int h = GPS.hdop.hdop();
  int lt = Btime/1000;
  int spd = GPS.speed.kmph();

  data.lat = lat;
  data.lng = lng;
  data.hdop = h;
  data.lt = lt;
  data.spd = spd;
  data.ts = t;
  data.cnt = count;

  ////////////// Save To Memory ///////////////
  digitalWrite(FPIN, HIGH);
  byte i2cStat = eeprom.begin(extEEPROM::twiClock100kHz);
  if ( i2cStat != 0 ) {
    Serial.println(F("ER1"));
  }
  if (wAdd > 124500) {
    wAdd = 1;
  }
  i2cStat = eeprom.write(wAdd, (byte*)&data, js);
  if (i2cStat != 0) {
    if (i2cStat == EEPROM_ADDR_ERR) {
      Serial.println(F("ER2"));
    } else {
      Serial.println(F("ER3"));
    }
  } else {
    Serial.println(F("WS"));
    delay(1000);
  }
  digitalWrite(FPIN, LOW);
  delay(10);

  wAdd = wAdd + js + 1;
  Serial.print(F("Write address : ")); Serial.println(wAdd);
  Serial.println(wAdd);
  
  delay(50);
}

void ERE(){

  digitalWrite(FPIN, HIGH);
  delay(100);
  struct main
  {
    double lat;
    double lng;
    time_t ts;
    int lt;
    int hdop;
    int spd;
    unsigned int cnt;
  }data;

  int js = sizeof(data);
  
  if (rAdd > 124500) {
    rAdd = 1;
  }
  byte i2cStat = eeprom.read(rAdd, (byte*)&data, js);
  delay(100);
  if (i2cStat != 0) {
    if (i2cStat == EEPROM_ADDR_ERR) {
      Serial.println(F("RF1"));
    } else {
      Serial.println(F("RF2"));
    }
  } else {
    Serial.println(F("RS"));
  }
  digitalWrite(FPIN, LOW);
  delay(10);
  rAdd = rAdd + js + 1;
  Serial.print(F("Read address : ")); Serial.println(rAdd);
  char a[150];
  StaticJsonDocument<150> doc;
  doc[F("ts")] = (unsigned long long)data.ts*1000;
  doc[F("Lat")] = data.lat;
  doc[F("Lng")] = data.lng;
  doc[F("hdop")] = data.hdop;
  doc[F("LT")] = data.lt;
  doc[F("Speed")] = data.spd; 
  doc[F("Count")] = data.cnt;
  doc[F("id")] = ID;
  serializeJson(doc, a);
  Serial.println(a);
  if (!mqtt.connected())
      {
        mqttConnect();  
        Serial.println(F("Transmitting"));
        if(mqtt.publish(telemetryTopic, (char*)a)){
          Serial.println(F("MQTT SUCCESS"));
        }
        mqtt.loop();
      }else{
        Serial.println(F("Transmitting"));
        if(mqtt.publish(telemetryTopic, (char*)a)){
          Serial.println(F("MQTT SUCCESS"));
        }
      }
      delay(50);
}

void postMetaAttributes(){
  Serial.println(F("Posting Meta Attributes"));
  char a[100];
  StaticJsonDocument<100> doc;
  doc[F("Battery")] = modem.getBattPercent();
  doc[F("Signal")] = modem.getSignalQuality();
  doc[F("Data")] = count;
  doc[F("sleep")] = false;
  doc[F("id")] = ID;
  serializeJson(doc, a);  
  Serial.println(a);
  if (mqtt.connected())
  {
    if(mqtt.publish(telemetryTopic, (char*)a)){
      Serial.println(F("Posted Meta Attributes"));
    }
  }  
}

void setup() {

  // put your setup code here, to run once:
  SerialMon.begin(9600);
  SerialAT.begin(9600);
  gpss.begin(9600);
  pinMode(GPIN, OUTPUT);
  pinMode(FPIN, OUTPUT);
  pinMode(IPIN, OUTPUT);

  Serial.println(F("INITIALIZING"));
  digitalWrite(IPIN, HIGH);
  delay(5000);
  // SerialAT.println(F("AT"));
  // Btime = 0;
  // while (Btime < 2000)
  // {
  //   if (SerialAT.available())
  //   {
  //     SerialMon.println(SerialAT.readString());
  //   }
    
  // }
  SerialAT.println(F("AT+CNETLIGHT=1"));
  Btime = 0;
  while (Btime < 2000)
  {
    if (SerialAT.available())
    {
      SerialMon.println(SerialAT.readString());
    }
    
  }
  
  networkInit();
  if (modem.isNetworkConnected())
  {
    SerialMon.print(F("Connecting to "));
    SerialMon.print(broker);

    // Connect to MQTT Broker
    // boolean status = mqtt.connect("GsmClientTest");

    // Or, if you want to authenticate MQTT:
    boolean status = mqtt.connect(ID, "", "");
    mqtt.setKeepAlive(10000);

    if (status == false) {
      SerialMon.println(F(" fail"));
      // return false;
    }
    SerialMon.println(F("success"));
    mqtt.subscribe(statusSubTopic);
  //   return mqtt.connected();
  }else{
    Serial.println(F("No Network"));
  }
  
  

  if (mqtt.connected())
  {
     Serial.println(F("Transmitting"));
        if(mqtt.publish(telemetryTopic, (char*)statReq)){
          Serial.println(F("MQTT SUCCESS"));
        } 
  }
  
  Btime = 0;
  while (Btime <= 5000)
  {
    mqtt.loop();
    delay(100);     
  }
  mqtt.disconnect();

  SerialAT.println(F("AT+CNETLIGHT=0"));
  Btime = 0;
  while (Btime < 2000)
  {
    if (SerialAT.available())
    {
      Serial.println(SerialAT.readString());
    }
    
  }

  SerialAT.println(F("AT&W"));
  Btime = 0;
  while (Btime < 2000)
  {
    if (SerialAT.available())
    {
      Serial.println(SerialAT.readString());
    }
    
  }
  digitalWrite(IPIN, LOW);

  if (activate == true)
  {
    Serial.println(F("STARTING GPS"));
    digitalWrite(GPIN, HIGH);
    do{
      while (gpss.available()) {
        if (GPS.encode(gpss.read())) {
          if (!GPS.location.isValid()) {
            Serial.print(F("Not Valid    "));
            Serial.print(GPS.location.lat());
            Serial.println(GPS.date.year());
            
          }else{
            Serial.println(GPS.location.isUpdated());
            Serial.print(F("Location Age:"));
            Serial.println(GPS.location.age());
            Serial.print(F("Time Age:"));
            Serial.println(GPS.time.age());
            Serial.print(F("Date Age:"));
            Serial.println(GPS.date.age());
            Serial.print(F("Satellites:"));
            Serial.println(GPS.satellites.value());
            Serial.print(F("HDOP:"));
            Serial.println(GPS.hdop.hdop());
          }
        }
      }
    }while (!GPS.location.isValid());

    lat = GPS.location.lat();
    lng = GPS.location.lng();
    setTime(GPS.time.hour(),GPS.time.minute(),GPS.time.second(),GPS.date.day (), GPS.date.month (),GPS.date.year());
    digitalWrite(GPIN, LOW);

    digitalWrite(IPIN, HIGH);
    networkInit();
    mqttConnect();
    postMetaAttributes();
    
    mqtt.disconnect();
    digitalWrite(IPIN, LOW);
    delay(1000);

    targetCycle = transInterval/gpsFrequency;
    Serial.println(targetCycle);

    slp.pwrDownMode();
    slp.sleepDelay(1000);
    Serial.println(F("SYSTEM READY"));  
    delay(500);
  
  }else{
    Serial.println(F("ETERNAL SLEEP"));  
    delay(500);
    sleepMode(SLEEP_POWER_DOWN);
    sleep();
  }  
}

void loop(){

    Mtime = 0;
    enablePower(POWER_ADC);
    enablePower(POWER_SERIAL0);
    enablePower(POWER_SERIAL1);
    enablePower(POWER_SPI);
    enablePower(POWER_WIRE);
    Serial.println(F("Waking"));
    Serial.print(F("Traget Cycle :")); Serial.println(targetCycle);

    RGPS();
    cycle = cycle + 1;
    Serial.print(F("CYCLE : ")); Serial.println(cycle);
        if (cycle >= targetCycle)
        {
        Serial.println(F("Starting Transmission"));
        digitalWrite(IPIN, HIGH);
        networkInit();
        
        while (rAdd != wAdd)
        {
        ERE();
        if (!mqtt.connected())
        {
            mqttConnect();
            mqtt.setKeepAlive(10000);
        }
        Btime = 0;
        delay(100);
        }
        postMetaAttributes();
        if (mqtt.connected())
        {
            if(mqtt.publish(telemetryTopic, setReq)){
            Serial.println(F("Posted Meta Attributes"));
            }
        }

        while (Btime <= 5000)
        {
            mqtt.loop();
            delay(100);                
        }
        mqtt.disconnect();
        digitalWrite(IPIN, LOW);
        cycle = 0;       
    }

    delay(1000);
    
    Serial.println(F("Sleeping for "));
    Serial.println(gpsFrequency*60000 - Mtime);
    delay(100);
    disablePower(POWER_ADC);
    disablePower(POWER_SERIAL0);
    disablePower(POWER_SERIAL1);
    disablePower(POWER_SPI);
    disablePower(POWER_WIRE);
    slp.pwrDownMode();
    slp.sleepDelay(gpsFrequency*60000 - Mtime);

}
