#include "DHT.h"
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "credentials.h"

/************ begin globals ******************/

// connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

TaskHandle_t Task1;

#define DHTPIN 2     // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

DHT dht(DHTPIN, DHTTYPE);

float h, t;

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

// task to run on second core. this runs independently of the other core on
// which loop() is running
void readFromDHTSensor(void* param)
{

}

void setup() {
  Serial.begin(9600);
  Serial.println("this sketch reads data from a dht22 sensor and publishes it to adafruit mqtt server");

  connectToWIFI();
  dht.begin();
}

void loop() {

  // connect to mqtt server
  MQTT_connect();
  
  // Wait a few seconds between measurements.
  delay(3000);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  t = dht.readTemperature();
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  printDataToSerial(h, t);
  publishData(h, t);

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
