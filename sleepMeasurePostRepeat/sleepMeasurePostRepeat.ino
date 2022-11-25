/*
 * This sketch measures values from the connected sensors, prints them to serial output and sends them to a server.
 * Both ESP8266 and ESP32 microcontrollers are supported. Other microcontrollers may be compatible too.
 * Printing and posting can be enabled/disabled by uncommenting/commenting the relative options in config.h.
 * To enable/disable a sensor, uncomment/comment its name in config.h.
 * All the available options can be set in config.h.
 * Do not edit this file. Edit config.h instead.
 * Repository: https://gitlab.cern.ch/guescini/canary-software
 * Wiki: https://gitlab.cern.ch/guescini/canary/wikis/home
 * Author: Francesco Guescini.
*/

//******************************************
//definitions
#define VERSION "v. 1.3.0"

//******************************************
//libraries
#include "config.h"
#include "sensor.h"
#include "vector"
#include "memory"
#include "AsyncDelay.h"
#include "ArduinoJson.h"

//network and MQTT
#if POST or VERBOSE
#include "NetworkHandler.h"
#ifdef ETHERNET //ethernet
#include "EthernetHandler.h"
#else //wifi
#include "WiFiHandler.h"
#endif //ETHERNET
#include "MQTTHandler.h"
#endif //POST or VERBOSE

//SHTxx
#if defined(SHT35A) or defined(SHT35B) or defined(SHT85)
#include "SHTxxSensor.h"
#endif //SHTxx

//MAX31865
#ifdef MAX31865
#include "MAX31865Sensor.h"
#endif //MAX31865

//BME280
#ifdef BME280
#include "BME280Sensor.h"
#endif //BME280

//BMP3xx
#ifdef BMP3XX
#include "BMP3xxSensor.h"
#endif //BMP3xx

//SPS30
#ifdef SPS30
#include "SPS30Sensor.h"
#endif //SPS30

//ADS1x15
#if defined(ADS1115) or defined(ADS1015)
#include "ADS1x15Sensor.h"
#endif //ADS1015 or ADS1115

//ESPx ADC
#ifdef ESPxADC
#include "ESPxADCSensor.h"
#endif //ESPx ADC

//******************************************
//network and MQTT setup
#if POST or VERBOSE
#ifdef ETHERNET //ethernet
EthernetHandler networkhandler;
#else //wifi
WiFiHandler networkhandler(WIFISSID,
			   WIFIPASSWORD);
#endif //ETHERNET

PubSubClient mqttclient;
MQTTHandler mqtthandler(&mqttclient,
			MQTTSERVER,
			TLS? MQTTTLSPORT:MQTTPORT,
			TLS,
			MQTTUSERNAME,
			MQTTPASSWORD,
			MQTTTOPIC,
			MQTTMESSAGESIZE,
			TLS? CACERT:"");
#endif //POST or VERBOSE

//******************************************
//vector of sensors
std::vector<std::unique_ptr<Sensor>> sensors;

//******************************************
//JSON documents
StaticJsonDocument<MQTTMESSAGESIZE> masterdoc;
StaticJsonDocument<160> sensordoc;

//******************************************
//loop time intervals
AsyncDelay sleepTime; //data measurement and posting
AsyncDelay integrationTime; //measurement average
AsyncDelay MQTTTime; //MQTT broker check-in

//******************************************
//setup
void setup() {

  //------------------------------------------
  //serial begin
  Serial.begin(115200);
  while (!Serial) {}
  delay(500);//ms
  Serial.println("\nsleep, measure, post, repeat");
  Serial.println(VERSION);
  Serial.println();

  //------------------------------------------
  //set I2C GPIO pins
#if defined(I2CSDA) and defined(I2CSCL)
  Wire.begin(I2CSDA, I2CSCL); //SDA, SCL
#endif //I2CSDA and I2CSCL

  //------------------------------------------
  //network connection
#if POST
  networkhandler.connect(VERBOSE);
#endif //POST

  //------------------------------------------
  //add sensors to the vector

  //------------------------------------------
  //SHT35A
#ifdef SHT35A
  SHTxxSensor* sht35a = new SHTxxSensor(SHTxxSensor::sht35a);
  sensors.emplace_back(sht35a);
#endif //SHT35A

  //------------------------------------------
  //SHT35B
#ifdef SHT35B
  SHTxxSensor* sht35b = new SHTxxSensor(SHTxxSensor::sht35b);
  sensors.emplace_back(sht35b);
#endif //SHT35B

  //------------------------------------------
  //SHT85
#ifdef SHT85
  SHTxxSensor* sht85 = new SHTxxSensor(SHTxxSensor::sht85);
  sensors.emplace_back(sht85);
#endif //SHT85

  //------------------------------------------
  //MAX31865
#ifdef MAX31865
  MAX31865Sensor* max31865 = new MAX31865Sensor(MAX31865RNOM, MAX31865RREF, MAX31865CS);
  sensors.emplace_back(max31865);
#ifdef MAX31865RHSOURCE
  max31865->setRHSource(MAX31865RHSOURCE);
#endif //MAX31865RHSOURCE
#endif //MAX31865

  //------------------------------------------
  //BME280
#ifdef BME280
  BME280Sensor* bme280 = new BME280Sensor(BME280ADDRESS);
  sensors.emplace_back(bme280);
#endif //BME280

  //------------------------------------------
  //BMP3xx
#ifdef BMP3XX
  BMP3xxSensor* bmp3xx = new BMP3xxSensor(BMP3XXTYPE, BMP3XXADDRESS);
  sensors.emplace_back(bmp3xx);
#endif //BMP3xx
    
  //------------------------------------------
  //SPS30
#ifdef SPS30
  SPS30Sensor* sps30 = new SPS30Sensor(SPS30AVERAGE, SPS30VERBOSE);
  sensors.emplace_back(sps30);
#endif //SPS30

  //------------------------------------------
  //ADS1015 or ADS1115
#if defined(ADS1015)
  ADS1x15Sensor<Adafruit_ADS1015, 12>* ads1x15 = new ADS1x15Sensor<Adafruit_ADS1015, 12>("ADS1015", ADS1x15VDD, ADS1x15VREF, ADS1x15GAIN);
#elif defined(ADS1115)
  ADS1x15Sensor<Adafruit_ADS1115, 16>* ads1x15 = new ADS1x15Sensor<Adafruit_ADS1115, 16>("ADS1115", ADS1x15VDD, ADS1x15VREF, ADS1x15GAIN);
#endif //ADS1015 or ADS1115

#if defined(ADS1115) or defined(ADS1015)
  sensors.emplace_back(ads1x15);
  
#if ADS1x150ADC or ADS1x150NTC
  ads1x15->setADCChannel(0, ADS1x150ADC, ADS1x150NTC, ADS1x150RDIV, ADS1x150T0, ADS1x150B, ADS1x150R0);
#ifdef ADS1x150NTCTHRESHOLD
  ads1x15->setNTCThreshold(0, ADS1x150NTCTHRESHOLD);
#endif //ADS1x150NTCTHRESHOLD
#endif //ADS1x150ADC or ADS1x150NTC

#if ADS1x151ADC or ADS1x151NTC
  ads1x15->setADCChannel(1, ADS1x151ADC, ADS1x151NTC, ADS1x151RDIV, ADS1x151T0, ADS1x151B, ADS1x151R0);
#ifdef ADS1x151NTCTHRESHOLD
  ads1x15->setNTCThreshold(1, ADS1x151NTCTHRESHOLD);
#endif //ADS1x151NTCTHRESHOLD
#endif //ADS1x151ADC or ADS1x151NTC

#if ADS1x152ADC or ADS1x152NTC
  ads1x15->setADCChannel(2, ADS1x152ADC, ADS1x152NTC, ADS1x152RDIV, ADS1x152T0, ADS1x152B, ADS1x152R0);
#ifdef ADS1x152NTCTHRESHOLD
  ads1x15->setNTCThreshold(2, ADS1x152NTCTHRESHOLD);
#endif //ADS1x152NTCTHRESHOLD
#endif //ADS1x152ADC or ADS1x152NTC

#if ADS1x153ADC or ADS1x153NTC
  ads1x15->setADCChannel(3, ADS1x153ADC, ADS1x153NTC, ADS1x153RDIV, ADS1x153T0, ADS1x153B, ADS1x153R0);
#ifdef ADS1x153NTCTHRESHOLD
  ads1x15->setNTCThreshold(3, ADS1x153NTCTHRESHOLD);
#endif //ADS1x153NTCTHRESHOLD
#endif //ADS1x153ADC or ADS1x153NTC

#endif //ADS1015 or ADS1115
  
  //------------------------------------------
  //ESP8266 or ESP32 ADC
#ifdef ESPxADC

#ifdef ESP8266
  ESPxADCSensor<10, 1>* espxadc = new ESPxADCSensor<10, 1>("ESP8266ADC", ESPxADCVDD, ESPxADCVREF, ESPxADCNREADINGS);
#else //ESP32
  ESPxADCSensor<12, 1>* espxadc = new ESPxADCSensor<12, 1>("ESP32ADC", ESPxADCVDD, ESPxADCVREF, ESPxADCNREADINGS);
  espxadc->setAttenuation(ESPxADCATTENUATION);
#endif //ESP8266 or ESP32

  sensors.emplace_back(espxadc);

#if ESPxADC0ADC or ESPxADC0NTC
  espxadc->setADCChannel(0, ESPxADC0PIN, ESPxADC0ADC, ESPxADC0NTC, ESPxADC0RDIV, ESPxADC0T0, ESPxADC0B, ESPxADC0R0);
#ifdef ESPxADC0NTCTHRESHOLD
  espxadc->setNTCThreshold(0, ESPxADC0NTCTHRESHOLD);
#endif //ESPxADC0NTCTHRESHOLD
#endif //ESPxADC0ADC or ESPxADC0NTC

#endif //ESPxADC

  //------------------------------------------
  //initialize sensors
  for (auto&& sensor : sensors) {
      sensor->init();
  }

  //------------------------------------------
  //print list of sensors
#if PRINTSERIAL or VERBOSE
  Serial.println("available sensors:");
  for (auto&& sensor : sensors) {
      Serial.print("\t");
      Serial.println(sensor->getName());
  }
  Serial.println();
#endif //PRINTSERIAL or VERBOSE

  //------------------------------------------
  //print measurement names and units
#if PRINTSERIAL
  for (auto&& sensor : sensors) {
      Serial.print(sensor->getSensorString());
  }
  Serial.println();
#endif //PRINTSERIAL

  //------------------------------------------
  //read, print and post values before going to sleep
  //NOTE this is meant for battery operation (no caffeine)
#if not CAFFEINE
  for (auto&& sensor : sensors) {
    sensor->integrate();
  }
  readPrintPost();
  Serial.println("now going to sleep zzz...");
  ESP.deepSleep(SLEEPTIME * 1e6); //µs
#endif //not CAFFEINE
  
  //------------------------------------------
  //start asynchronous timing
#if CAFFEINE
  MQTTTime.start(MQTTTIME*1e3, AsyncDelay::MILLIS);
  integrationTime.start(INTEGRATIONTIME*1e3, AsyncDelay::MILLIS);
  sleepTime.start(SLEEPTIME*1e3, AsyncDelay::MILLIS);
#endif //CAFFEINE
  
} //setup()

//******************************************
//loop
void loop() {

  //------------------------------------------
  //integrate sensor measurements (if averaging is enabled)
  if (integrationTime.isExpired()) {
    for (auto&& sensor : sensors) {
      sensor->integrate();
    }
    integrationTime.repeat();
  }

  //------------------------------------------
  //read, print and post measurements
  if (sleepTime.isExpired()) {
    readPrintPost();
    sleepTime.repeat();
  }

  //------------------------------------------
  //MQTT check-in loop
#if POST
  if (MQTTTime.isExpired()) {
    mqtthandler.loop();
    MQTTTime.repeat();
  }
#endif //POST

} //loop()

//******************************************
//read, print and post measurements
//NOTE integration and averaging is done separately
void readPrintPost() {

    //------------------------------------------
    //read measurements
    for (auto&& sensor : sensors) {
      sensor->readData();
    }
    
    //------------------------------------------
    //print measurements
#if PRINTSERIAL
    for (auto&& sensor : sensors) {
      Serial.print(sensor->getMeasurementsString());
    }
    Serial.println();
#endif //PRINTSERIAL

#if POST or VERBOSE
    //------------------------------------------
    //post measurements
    masterdoc.clear();
    for (auto&& sensor : sensors) {
      sensordoc.clear();
      sensor->getJSONDoc(sensordoc);
      addMetaData(sensordoc);
      masterdoc.add(sensordoc);
    }
#if POST
    mqtthandler.post(masterdoc, networkhandler.connect(VERBOSE), VERBOSE);
#else
    mqtthandler.post(masterdoc, POST, VERBOSE);
#endif //POST

    //------------------------------------------
    //disconnect before leaving
#if not CAFFEINE
    networkhandler.disconnect();
#endif //not CAFFEINE
#endif //POST or VERBOSE


} //readPrintPost()

//******************************************
//add metadata to JSON document
void addMetaData(JsonDocument &doc) {
#ifdef INSTITUTE
  doc["institute"] = INSTITUTE;
#endif //INSTITUTE
#ifdef ROOM
  doc["room"] = ROOM;
#endif //ROOM
#ifdef LOCATION
  doc["location"] = LOCATION;
#endif //LOCATION
#ifdef NAME
  doc["name"] = NAME;
#elif defined(MACASNAME)
  doc["name"] = networkhandler.getMACAddress();
#endif //NAME or MACASNAME
  return;
}
