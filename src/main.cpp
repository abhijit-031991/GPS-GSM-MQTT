// #define TINY_GSM_MODEM_SIM800
// #define ARDUINOJSON_USE_LONG_LONG 1

// // Libraries //
// #include <Arduino.h>
// #include <ArduinoJson.h>
// #include <TinyGPS++.h>
// #include <elapsedMillis.h>
// #include <extEEPROM.h>
// #include <SoftwareSerial.h>
// #include <Wire.h>
// #include <TinyGsmClient.h>
// #include <ArduinoHttpClient.h>
// #include <Sleep_n0m1.h>
// #include <TimeLib.h>
// #include <PubSubClient.h>


// // Definittions //

// #define SerialMon Serial

// #define SerialAT Serial1


// // Library Definitions //

// elapsedMillis Mtime;
// elapsedMillis Btime;
// TinyGPSPlus GPS;
// extEEPROM eeprom(kbits_1024, 2, 256);

// // ExternalEEPROM mem;

// SoftwareSerial gpss(13, 12);
// Sleep slp;

// // Pin Definitions //

// #define GPIN A1
// #define IPIN 4
// #define FPIN A0

// // Device Details // 

// const char ID[] = "TIG101";
// const char Key[] = "KoiXXymDq01C0hcDeHHd";

// // GPS Control Variables //

// int gpsTimeout = 10;    // GPS Timeout in seconds  *** USER CONFIG ***
// int gpsFrequency = 1;  // GPS Frequency in Minutes *** USER CONFIG ***
// int gpshdop = 5;           // GPS HDOP *** USER CONFIG ***

// // GPS Storage Variables // 
// double lat;
// double lng;
// unsigned int count; 

// // Memory Variables //
// unsigned long wAdd = 1;
// unsigned long rAdd = 1;

// // Transmission Control Variables //
// int transInterval = 3; // Transmission Interval *** USER CONFIG ***

// //GPRS credentials //
// const char apn[]      = "airtelgprs.com";
// const char gprsUser[] = "";
// const char gprsPass[] = "";

// // MQTT details ///
// const char* broker = "13.233.227.254";

// const char* telemetryTopic = "telemetry";
// const char* statusTopic = "status";
// const char* settingsSupTopic = "settings";

// /// Extra ///

// TinyGsm modem(SerialAT);
// TinyGsmClient client(modem);
// PubSubClient  mqtt(client);
// uint32_t lastReconnectAttempt = 0;

// /// Cyclic Variables ///

// int cycle = 1;
// int targetCycle;

// //// activation Switch ////

// bool activate = false;
// bool ret = false;
// bool activity = false;

// //////////////////////////////
// /////     FUNCTIONS      /////
// //////////////////////////////

// void mqttCallback(char* topic, byte* payload, unsigned int len) {
//   SerialMon.print("Message arrived [");
//   SerialMon.print(topic);
//   SerialMon.print("]: ");
//   // SerialMon.write(payload, len);
//   SerialMon.println();

//   char x[len];
//   strncpy(x, (char*)payload, len);
//   Serial.println(x);
//   Serial.println("DONE");

//   StaticJsonDocument<192> doc;

//   DeserializationError error = deserializeJson(doc, x);

//   if (error) {
//     Serial.print(F("deserializeJson() failed: "));
//     Serial.println(error.f_str());
//     return;
//   }

//   JsonObject shared = doc["shared"];
//   gpshdop = shared["gpsHdop"]; // 5
//   gpsFrequency = shared["gpsFrequency"]; // 1
//   transInterval = shared["transmissionInterval"]; // 9
//   gpsTimeout = shared["gpsTimeout"]; // 10
//   activate = shared["activate"]; // false

//   Serial.println(gpshdop);
//   Serial.println(gpsFrequency);
//   Serial.println(gpsTimeout);
//   Serial.println(transInterval);
//   Serial.println(activate);

//   if (activate == true)
//   {
//     activity = true;
//   }else{
//     activity = false;
//   }
  
//   targetCycle = transInterval/gpsFrequency;
//   ret = true;  

// }

// boolean mqttConnect() {
//   SerialMon.print("Connecting to ");
//   SerialMon.print(broker);

//   // Connect to MQTT Broker
//   // boolean status = mqtt.connect("GsmClientTest");

//   // Or, if you want to authenticate MQTT:
//   boolean status = mqtt.connect(ID, Key, "");
//   mqtt.setKeepAlive(10000);

//   if (status == false) {
//     SerialMon.println(" fail");
//     return false;
//   }
//   SerialMon.println(" success");
//   mqtt.subscribe(settingsSupTopic);
//   return mqtt.connected();

// }

// void getSettings(){

//   Serial.println(F("Requesting Attributes"));
//   if (mqtt.connected()){
//     mqtt.subscribe(settingsSupTopic);
//   }
//   Btime = 0;
//   while (Btime < 30000)
//   {
//     if (!mqtt.connected())
//     {
//       mqttConnect();
//     }    
//     mqtt.loop();
//     delay(1000);
//     if (ret == true)
//     {
//       ret = false;
//       return;
//     }    
//   }    
// }

// // void postSetAttributes(){
// //   Serial.println(F("Posting Set Attributes"));
// //   char a[250];
// //   StaticJsonDocument<250> doc;
// //   doc[F("gpsFrequency")] = gpsFrequency;
// //   doc[F("gpsTimeout")] = gpsTimeout;
// //   doc[F("gpsHdop")] = gpshdop;
// //   doc[F("transmissionInterval")] = transInterval;
// //   doc[F("activity")] = activity;
// //   serializeJson(doc, a);  
// //   Serial.println(a);
// //   if (mqtt.connected())
// //   {
// //     if(mqtt.publish(attributeUpdateTopic, (char*)a)){
// //       Serial.println(F("Posted Set Attributes"));
// //     }
// //   }  
// // }

// void postMetaAttributes(){
//   Serial.println(F("Posting Meta Attributes"));
//   char a[250];
//   StaticJsonDocument<250> doc;
//   doc[F("Battery")] = modem.getBattPercent();
//   doc[F("Signal")] = modem.getSignalQuality();
//   doc[F("Data")] = count;
//   serializeJson(doc, a);  
//   Serial.println(a);
//   if (mqtt.connected())
//   {
//     if(mqtt.publish(attributeUpdateTopic, (char*)a)){
//       Serial.println(F("Posted Meta Attributes"));
//     }
//   }  
// }

// void networkInit(){
//   modem.init();
//   String modemInfo = modem.getModemInfo();
//   SerialMon.print(F("Modem Info: "));
//   SerialMon.println(modemInfo);

//   SerialMon.print(F("Waiting for network..."));
//   if (!modem.waitForNetwork()) {
//     SerialMon.println(F("fail"));
//     // delay(10000);
//     return;
//   }
//   SerialMon.println(F("success"));

//   if (modem.isNetworkConnected()) { 
//     SerialMon.println(F("Network connected")); 
//   }

//    // MQTT Broker setup
//   mqtt.setServer(broker, 1883);
//   mqtt.setCallback(mqttCallback);

//   if (!modem.isGprsConnected()) {
//       SerialMon.println(F("GPRS disconnected!"));
//       SerialMon.print(F("Connecting to "));
//       SerialMon.print(apn);
//       if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
//         SerialMon.println(F(" fail"));
//         delay(10000);
//         return;
//       }
//     }

//   if (modem.isGprsConnected()){ 
//     SerialMon.println(F("GPRS reconnected")); 
//   }
//   if (!mqtt.connected()) {
//     SerialMon.println(F("=== MQTT NOT CONNECTED ==="));
//     // Reconnect every 10 seconds
//     uint32_t t = millis();
//     if (t - lastReconnectAttempt > 10000L) {
//       lastReconnectAttempt = t;
//       if (mqttConnect()) { lastReconnectAttempt = 0; }
//     }
//     delay(100);
//     return;
//   }
// }

// void RGPS() {
//   struct main
//   {
//     double lat;
//     double lng;
//     time_t ts;
//     int lt;
//     int hdop;
//     int spd;
//     unsigned int cnt;
//   }data;
//   count = count + 1;
//   int js = sizeof(data);
//   Btime = 0;
//   digitalWrite(GPIN, HIGH);
//   while (Btime <= (unsigned)gpsTimeout*1000) {
//     while (gpss.available()) {
//       if (GPS.encode(gpss.read())) {
//         if (!GPS.location.isValid()) {
//           Serial.println(F("Not Valid"));          
//         }else
//         {   
//           Serial.println(GPS.location.isUpdated());
//           Serial.print(F("Location Age:"));
//           Serial.println(GPS.location.age());
//           Serial.print(F("Time Age:"));
//           Serial.println(GPS.time.age());
//           Serial.print(F("Date Age:"));
//           Serial.println(GPS.date.age());
//           Serial.print(F("Satellites:"));
//           Serial.println(GPS.satellites.value());
//           Serial.print(F("HDOP:"));
//           Serial.println(GPS.hdop.hdop());
//         }
//       }
//     }
//     if (GPS.hdop.hdop() > 0.0 && GPS.hdop.hdop() < gpshdop && GPS.location.age() < 3000)
//     { 
//       Serial.println(F("breaking"));
//       break;
//     }
    
//   }
//   digitalWrite(GPIN, LOW);
  
//   if (GPS.location.age() < 60000)
//   {
//     lat = GPS.location.lat();
//     // values[F("Lat")] = GPS.location.lat();
//     lng = GPS.location.lng();
//     // values[F("Lng")] = GPS.location.lng();
//   } else{
//     lat = 502;
//     lng = 502;
//   }  
//   setTime(GPS.time.hour(),GPS.time.minute(),GPS.time.second(),GPS.date.day (), GPS.date.month (),GPS.date.year());
//   time_t t = now();
//   Serial.println(t);
//   int h = GPS.hdop.hdop();
//   int lt = Btime/1000;
//   int spd = GPS.speed.kmph();

//   data.lat = lat;
//   data.lng = lng;
//   data.hdop = h;
//   data.lt = lt;
//   data.spd = spd;
//   data.ts = t;
//   data.cnt = count;

//   ////////////// Save To Memory ///////////////
//   digitalWrite(FPIN, HIGH);
//   byte i2cStat = eeprom.begin(extEEPROM::twiClock100kHz);
//   if ( i2cStat != 0 ) {
//     Serial.println(F("ER1"));
//   }
//   if (wAdd > 124500) {
//     wAdd = 1;
//   }
//   i2cStat = eeprom.write(wAdd, (byte*)&data, js);
//   if (i2cStat != 0) {
//     if (i2cStat == EEPROM_ADDR_ERR) {
//       Serial.println(F("ER2"));
//     } else {
//       Serial.println(F("ER3"));
//     }
//   } else {
//     Serial.println(F("WS"));
//     delay(1000);
//   }
//   digitalWrite(FPIN, LOW);
//   delay(10);

//   wAdd = wAdd + js + 1;
//   Serial.print(F("Write address : ")); Serial.println(wAdd);
//   Serial.println(wAdd);
  
//   delay(50);
// }

// void ERE(){

//   digitalWrite(FPIN, HIGH);
//   delay(100);
//   struct main
//   {
//     double lat;
//     double lng;
//     time_t ts;
//     int lt;
//     int hdop;
//     int spd;
//     unsigned int cnt;
//   }data;

//   int js = sizeof(data);
  
//   if (rAdd > 124500) {
//     rAdd = 1;
//   }
//   byte i2cStat = eeprom.read(rAdd, (byte*)&data, js);
//   delay(100);
//   if (i2cStat != 0) {
//     if (i2cStat == EEPROM_ADDR_ERR) {
//       Serial.println(F("RF1"));
//     } else {
//       Serial.println(F("RF2"));
//     }
//   } else {
//     Serial.println(F("RS"));
//   }
//   digitalWrite(FPIN, LOW);
//   delay(10);
//   rAdd = rAdd + js + 1;
//   Serial.print(F("Read address : ")); Serial.println(rAdd);
//   char a[150];
//   StaticJsonDocument<150> doc;
//   doc[F("ts")] = (unsigned long long)data.ts*1000;
//   delay(100);
//   JsonObject val = doc.createNestedObject("values");
//   val[F("Lat")] = data.lat;
//   val[F("Lng")] = data.lng;
//   val[F("hdop")] = data.hdop;
//   val[F("LT")] = data.lt;
//   val[F("Speed")] = data.spd; 
//   val[F("Count")] = data.cnt;
//   serializeJson(doc, a);

//   if (!mqtt.connected())
//       {
//         mqttConnect();  
//         Serial.println(F("Transmitting"));
//         if(mqtt.publish(telemetryTopic, (char*)a)){
//           Serial.println(F("MQTT SUCCESS"));
//         }
//         mqtt.loop();
//       }else{
//         Serial.println(F("Transmitting"));
//         if(mqtt.publish(telemetryTopic, (char*)a)){
//           Serial.println(F("MQTT SUCCESS"));
//         }
//       }
//       delay(50);
// }

// void setup() {

//   // put your setup code here, to run once:
//   SerialMon.begin(9600);
//   SerialAT.begin(9600);
//   gpss.begin(9600);
//   pinMode(GPIN, OUTPUT);
//   pinMode(FPIN, OUTPUT);
//   pinMode(IPIN, OUTPUT);

//   Serial.println(F("INITIALIZING"));
//   digitalWrite(IPIN, HIGH);
//   networkInit();
//   // postSetAttributes();
//   getSettings();
//   mqtt.disconnect();
//   digitalWrite(IPIN, LOW);

//   if (activate == true)
//   {
//     digitalWrite(GPIN, HIGH);
//     do{
//       while (gpss.available()) {
//         if (GPS.encode(gpss.read())) {
//           if (!GPS.location.isValid()) {
//             Serial.print(F("Not Valid    "));
//             Serial.print(GPS.location.lat());
//             Serial.println(GPS.date.year());
            
//           }
//         }
//       }
//     }while (!GPS.location.isValid());

//     lat = GPS.location.lat();
//     lng = GPS.location.lng();
//     setTime(GPS.time.hour(),GPS.time.minute(),GPS.time.second(),GPS.date.day (), GPS.date.month (),GPS.date.year());
//     digitalWrite(GPIN, LOW);

//     digitalWrite(IPIN, HIGH);
//     networkInit();
//     mqttConnect();
//     // postSetAttributes();
//     // postMetaAttributes();
//     mqtt.disconnect();
//     digitalWrite(IPIN, LOW);
//     delay(1000);

//     targetCycle = transInterval/gpsFrequency;
//     Serial.println(targetCycle);

//     slp.pwrDownMode();
//     slp.sleepDelay(1000);
//     Serial.println(F("SYSTEM READY"));  
//     delay(500);
  
//   }else{
//     sleepMode(SLEEP_POWER_DOWN);
//     sleep();
//   }  
// }

// void loop(){
//   Mtime = 0;
//   enablePower(POWER_ADC);
//   enablePower(POWER_SERIAL0);
//   enablePower(POWER_SERIAL1);
//   enablePower(POWER_SPI);
//   enablePower(POWER_WIRE);
//   Serial.println(F("Waking"));
//   Serial.print(F("Traget Cycle :")); Serial.println(targetCycle);

//   RGPS();
//   cycle = cycle + 1;
//   Serial.print(F("CYCLE : ")); Serial.println(cycle);
//     if (cycle >= targetCycle)
//     {
//     Serial.println(F("Starting Transmission"));
//     digitalWrite(IPIN, HIGH);
//     networkInit();
    
//     while (rAdd != wAdd)
//     {
//       ERE();
//       if (!mqtt.connected())
//       {
//         mqttConnect();
//         mqtt.setKeepAlive(10000);
//       }
//       Btime = 0;
//       delay(100);
//     }
//     // postMetaAttributes();
//     getSettings();
//     mqtt.disconnect();
//     digitalWrite(IPIN, LOW);
//     cycle = 0;       
//   }

//   delay(1000);
  
//   Serial.println(F("Sleeping for "));
//   Serial.println(gpsFrequency*60000 - Mtime);
//   delay(100);
//   disablePower(POWER_ADC);
//   disablePower(POWER_SERIAL0);
//   disablePower(POWER_SERIAL1);
//   disablePower(POWER_SPI);
//   disablePower(POWER_WIRE);
//   slp.pwrDownMode();
//   slp.sleepDelay(gpsFrequency*60000 - Mtime);
// }
