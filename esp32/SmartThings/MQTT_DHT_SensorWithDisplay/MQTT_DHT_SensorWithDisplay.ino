//******************************************************************************************

//  Summary:  This Arduino Sketch, along with the ST_Anything library and the revised SmartThings 
//            library, demonstrates the ability of one ESP32 to implement 
//            a multi input/output custom device for integration into SmartThings.
//            
//******************************************************************************************
//******************************************************************************************
// SmartThings Library for ESP32WiFi
//******************************************************************************************
#include <SmartThingsESP32WiFi.h>
#include "credentials.h"
#include "SevSeg.h"

//******************************************************************************************
// SmartThings Library for ESP32WiFi
//******************************************************************************************
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

//******************************************************************************************
// ST_Anything Library 
//******************************************************************************************
#include <Constants.h>       //Constants.h is designed to be modified by the end user to adjust behavior of the ST_Anything library
#include <Device.h>          //Generic Device Class, inherited by Sensor and Executor classes
#include <Sensor.h>          //Generic Sensor Class, typically provides data to ST Cloud (e.g. Temperature, Motion, etc...)
#include <Executor.h>        //Generic Executor Class, typically receives data from ST Cloud (e.g. Switch)
#include <InterruptSensor.h> //Generic Interrupt "Sensor" Class, waits for change of state on digital input 
#include <PollingSensor.h>   //Generic Polling "Sensor" Class, polls Arduino pins periodically
#include <Everything.h>      //Master Brain of ST_Anything library that ties everything together and performs ST Shield communications
#include <PS_TemperatureHumidity.h>  //Implements a Polling Sensor (PS) to measure Temperature and Humidity via DHT library
#include <IS_Motion.h>       //Implements an Interrupt Sensor (IS) to detect motion via a PIR sensor on a digital input pin

//****************************************************************************************************************************
//NodeMCU-32s ESP32 Pin Definitions (just for reference from ..hardware\espressif\esp32\variants\nodemcu-32s\pins_arduino.h)
//****************************************************************************************************************************
//#define LED_BUILTIN 2
//#define BUILTIN_LED 2
//
//#define A0 = 36;
//#define A3 = 39;
//#define A4 = 32;
//#define A5 = 33;
//#define A6 = 34;
//#define A7 = 35;
//#define A10 = 4;
//#define A11 = 0;
//#define A12 = 2;
//#define A13 = 15;
//#define A14 = 13;
//#define A15 = 12;
//#define A16 = 14;
//#define A17 = 27;
//#define A18 = 25;
//#define A19 = 26;

//******************************************************************************************
//Define which Arduino Pins will be used for each device
//******************************************************************************************
//"RESERVED" pins for ESP32 - best to avoid
#define PIN_0_RESERVED             0  //reserved ESP32 boot/program upload
#define PIN_1_RESERVED             1  //reserved ESP32 for TX0
#define PIN_3_RESERVED             3  //reserved ESP32 for RX0
#define PIN_6_RESERVED             6  //reserved ESP32 for flash
#define PIN_7_RESERVED             7  //reserved ESP32 for flash
#define PIN_8_RESERVED             8  //reserved ESP32 for flash
#define PIN_9_RESERVED             9  //reserved ESP32 for flash
#define PIN_10_RESERVED           10  //reserved ESP32 for flash
#define PIN_11_RESERVED           11  //reserved ESP32 for flash

//Digital Pins
#define PIN_TEMPERATUREHUMIDITY_1 32  //SmartThings Capabilities "Temperature Measurement" and "Relative Humidity Measurement"
#define PIN_MOTION_1              22  //SmartThings Capability "Motion Sensor"

float gTemperature = 0.0;
float gHumidity = 0.0;
TaskHandle_t Task1;
SevSeg sevseg; 
int gPublishIntervalInMilliSec = 1000*60*5; // 5 minute interval
//******************************************************************************************
//ESP832 WiFi Information
//******************************************************************************************
const unsigned int serverPort = 8090; // port to run the http server on

// Smartthings / Hubitat Hub TCP/IP Address
IPAddress hubIp(192, 168, 1, 175);    // smartthings/hubitat hub ip //  <---You must edit this line!

// SmartThings / Hubitat Hub TCP/IP Address: UNCOMMENT line that corresponds to your hub, COMMENT the other
const unsigned int hubPort = 39500;   // smartthings hub port

//DHCP IP Assigment - Must set your router's DHCP server to provice a static IP address for this device's MAC address
st::SmartThingsESP32WiFi stWifi = st::SmartThingsESP32WiFi(WLAN_SSID, WLAN_PASS, serverPort, hubIp, hubPort, st::receiveSmartString);

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&(stWifi.st_client), AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Setup a feed called 'humidity' 
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");

// Setup a feed called 'temperature' 
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");


//******************************************************************************************
//st::Everything::callOnMsgSend() optional callback routine.  This is a sniffer to monitor 
//    data being sent to ST.  This allows a user to act on data changes locally within the 
//    Arduino sktech.
//******************************************************************************************
void callback(const String &msg)
{
  String strTemp = msg;
  Serial.print(F("ST_Anything Callback: Sniffed data = "));
  Serial.println(msg);
  
  //TODO:  Add local logic here to take action when a device's value/state is changed
  
  //Masquerade as the ThingShield to send data to the Arduino, as if from the ST Cloud (uncomment and edit following line)
  //st::receiveSmartString("Put your command here!");  //use same strings that the Device Handler would send
  if (strTemp.startsWith("temperature1"))
  {
      strTemp.remove(0,13);
      Serial.println(strTemp);
      gTemperature = strTemp.toFloat();
  }
//  if (strTemp.startsWith("humidity1"))
//  {
//    strTemp.remove(0,10);
//    Serial.println(strTemp);
//  }
}

void configure_SevenSeg()
{
  byte numDigits = 4;
  byte digitPins[] = {13, 27, 26, 19};
  byte segmentPins[] = {12, 25, 18, 16, 4, 14, 23, 17};
  bool resistorsOnSegments = true; // 'false' means resistors are on digit pins
  byte hardwareConfig = COMMON_ANODE; // See README.md for options
  bool updateWithDelays = false; // Default. Recommended
  bool leadingZeros = false; // Use 'true' if you'd like to keep the leading zeros
  
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments, updateWithDelays, leadingZeros);
  sevseg.setBrightness(10);
}


void displayAndPublish(void* param)
{ 
  int count = 0;
  while(true)
  {  
    //Serial.println("display LED Task");
    sevseg.setNumber(gTemperature, 1); // 1 digit after decimal precision
    sevseg.refreshDisplay(); // Must run repeatedly
    delay(5);
    count += 5;
    
    //publish data every 10 sec to MQTT server
    if(count%gPublishIntervalInMilliSec == 0) {
      publishDataToMQTTServer(gHumidity, gTemperature);
      count = 0; // reset counter to 0 to avoid any overflow   
    }
  }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  
  Serial.println("MQTT Connected!");
}

void publishDataToMQTTServer(float h, float t)
{
  MQTT_connect();
  humidity.publish(h) ? Serial.println(F("humidity publish OK!")) : Serial.println(F("humidity publish Failed"));
  temperature.publish(t) ? Serial.println(F("temp publish OK!")) : Serial.println(F("temp publish Failed"));

  if(! mqtt.ping()) {
        mqtt.disconnect();
  }
}

//******************************************************************************************
//Arduino Setup() routine
//******************************************************************************************
void setup()
{
  //******************************************************************************************
  // Setup the default values for the ADC.  Used for analog voltage reads.
  //  Notes:  analogReadResolution(12) sets the resolution for all pins. 12 = 0-4095, 11 = 0-2047, 10 = 0-1024, 9 = 0-512 
  //          analogSetAttenuation(ADC_11db) sets the attenuation for all pins.  11db = 0-3.3v, 6dB range = 0-2.2v, 2.5db = 0-1.5v, 0db = 0-1v
  //          analogSetPinAttenuation(A7, ADC_11db) sets the attenuation for individual pins.
  //******************************************************************************************
  
  analogReadResolution(11);        // Default of 12 is not very linear.  Recommended to use 10 or 11 depending on needed resolution.
  analogSetAttenuation(ADC_6db);   // Default is 11db which is very noisy.  Recommended to use 2.5 or 6.
  
  //******************************************************************************************
  //Declare each Device that is attached to the Arduino
  //  Notes: - For each device, there is typically a corresponding "tile" defined in your 
  //           SmartThings Device Hanlder Groovy code, except when using new COMPOSITE Device Handler
  //         - For details on each device's constructor arguments below, please refer to the 
  //           corresponding header (.h) and program (.cpp) files.
  //         - The name assigned to each device (1st argument below) must match the Groovy
  //           Device Handler names.  (Note: "temphumid" below is the exception to this rule
  //           as the DHT sensors produce both "temperature" and "humidity".  Data from that
  //           particular sensor is sent to the ST Hub in two separate updates, one for 
  //           "temperature" and one for "humidity")
  //         - The new Composite Device Handler is comprised of a Parent DH and various Child
  //           DH's.  The names used below MUST not be changed for the Automatic Creation of
  //           child devices to work properly.  Simply increment the number by +1 for each duplicate
  //           device (e.g. contact1, contact2, contact3, etc...)  You can rename the Child Devices
  //           to match your specific use case in the ST Phone Application.
  //******************************************************************************************
  //Polling Sensors
  static st::PS_TemperatureHumidity sensor7(F("temphumid1"), 15, 5, PIN_TEMPERATUREHUMIDITY_1, st::PS_TemperatureHumidity::DHT22,"temperature1","humidity1");

  // Interrupt sensors
  static st::IS_Motion              sensor9(F("motion1"), PIN_MOTION_1, HIGH, false, 500);
  
  //*****************************************************************************
  //  Configure debug print output from each main class 
  //  -Note: Set these to "false" if using Hardware Serial on pins 0 & 1
  //         to prevent communication conflicts with the ST Shield communications
  //*****************************************************************************
  st::Everything::debug=true;
  st::Executor::debug=true;
  st::Device::debug=true;
  st::PollingSensor::debug=true;
  st::InterruptSensor::debug=true;

  //*****************************************************************************
  //Initialize the "Everything" Class
  //*****************************************************************************

  //Initialize the optional local callback routine (safe to comment out if not desired)
  st::Everything::callOnMsgSend = callback;
  
  st::Everything::SmartThing = &stWifi;

  //Run the Everything class' init() routine which establishes WiFi communications with SmartThings Hub
  st::Everything::init();

  //*****************************************************************************
  //Add each sensor to the "Everything" Class
  //*****************************************************************************
  st::Everything::addSensor(&sensor7);
  st::Everything::addSensor(&sensor9);
   
  //*****************************************************************************
  //Initialize each of the devices which were added to the Everything Class
  //*****************************************************************************
  st::Everything::initDevices();

  configure_SevenSeg();

  //setup task on core 1
  xTaskCreatePinnedToCore(
    displayAndPublish,   // code to run
    "displayOnLED",       // name of task
    2048,           // stack size
    NULL,           // parameters to pass to task
    1,              // priority
    &Task1,         // code to run
    0);             // Core to run on : choose between 0 and 1 (1 is default for ESP32 IDE)
  
}

//******************************************************************************************
//Arduino Loop() routine
//******************************************************************************************
void loop()
{
  //*****************************************************************************
  //Execute the Everything run method which takes care of "Everything"
  //*****************************************************************************
  st::Everything::run();
}
