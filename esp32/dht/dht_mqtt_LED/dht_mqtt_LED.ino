#include "DHT.h"
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "credentials.h"
#include "SevSeg.h"


/************ begin globals ******************/

// connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

TaskHandle_t Task1;

#define DHTPIN 32    // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

DHT dht(DHTPIN, DHTTYPE);
int publishInterval = 3000; 
float gHumidity = 0.0;
float gTemperature = 0.0;
bool unitsF = true;

SevSeg sevseg; //Instantiate a seven segment controller object

/********* end globals *********/


/****************************** Feeds ***************************************/

// Setup a feed called 'humidity' 
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");

// Setup a feed called 'temperature' 
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");

/*************************** Sketch Code ************************************/
void connectToWIFI()
{
  // Connect to WiFi access point.
  Serial.println(); 
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
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

void codeForTask1(void* param)
{ 
  while(true)
  {  
    sevseg.setNumber(gTemperature, 1);
    sevseg.refreshDisplay(); // Must run repeatedly
    delay(5);
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("this sketch reads data from a dht22 sensor and publishes it to adafruit mqtt server. Also displays temp data to an attached 7 segment display");

  configure_SevenSeg();
  connectToWIFI();
  dht.begin();

  xTaskCreatePinnedToCore(
    codeForTask1,   // code to run
    "displayOnLED",       // name of task
    2048,           // stack size
    NULL,           // parameters to pass to task
    1,              // priority
    &Task1,         // code to run
    0);             // Core to run on : choose between 0 and 1 (1 is default for ESP32 IDE)

}

void loop() {

   // connect to mqtt server
   MQTT_connect();
   delay(publishInterval);
    
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  gHumidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  gTemperature = dht.readTemperature(unitsF);
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(gHumidity) || isnan(gTemperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  
  printDataToSerial(gHumidity, gTemperature);
  publishData(gHumidity, gTemperature);
 
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
}

void printDataToSerial(float h, float t)
{
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.println(t);
}

void publishData(float h, float t)
{
  humidity.publish(h) ? Serial.println(F("humidity publish OK!")) : Serial.println(F("humidity publish Failed"));
  temperature.publish(t) ? Serial.println(F("temp publish OK!")) : Serial.println(F("temp publish Failed"));
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
