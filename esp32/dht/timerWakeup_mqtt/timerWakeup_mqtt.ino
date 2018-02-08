#include "DHT.h"
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "credentials.h"

/************ begin globals ******************/
#define DHTPIN 13       // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

WiFiClient client;
uint64_t uS_TO_S_FACTOR = 1000000;    /* Conversion factor for micro seconds to seconds */
uint64_t TIME_TO_SLEEP =  5*60;        /* sleep time in seconds */
float gHumidity = 0.0;
float gTemperature = 0.0;
DHT dht(DHTPIN, DHTTYPE);
/********* end globals *********/

/****************************** ADAFRUIT MQTT setup ***************************************/
// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
// Setup a feed called 'humidity' 
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");

// Setup a feed called 'temperature' 
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");

/*************************** Sketch Code ************************************/

//attempts a wifi connection. if connection fails, then wifi will reset
void connectToWIFI()
{
  // Connect to WiFi access point.
  Serial.println(); 
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  int8_t retries = 5;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
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

// setup code
void setup() {
  Serial.begin(9600);
  Serial.println("this sketch reads data from a dht22 sensor and publishes it to adafruit mqtt server");

  connectToWIFI();
  dht.begin();

  // connect to mqtt server
   MQTT_connect();
 
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  gHumidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  gTemperature = dht.readTemperature();
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(gHumidity) || isnan(gTemperature)) {
    Serial.println("Failed to read from DHT sensor!");
  }
  else
  {
    printDataToSerial(gHumidity, gTemperature);
    publishData(gHumidity, gTemperature);
  }
 
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }

  /*
  First we configure the wake up source
  We set our ESP32 to wake up every 5 minutes
  */
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep");

  /*
  Next we decide what all peripherals to shut down/keep on
  By default, ESP32 will automatically power down the peripherals
  not needed by the wakeup source, but if you want to be a poweruser
  this is for you. Read in detail at the API docs
  http://esp-idf.readthedocs.io/en/latest/api-reference/system/deep_sleep.html
  Left the line commented as an example of how to configure peripherals.
  The line below turns off all RTC peripherals in deep sleep.
  */
  //esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  //Serial.println("Configured all RTC Peripherals to be powered down in sleep");

  /*
  Now that we have setup a wake cause and if needed setup the
  peripherals state in deep sleep, we can now start going to
  deep sleep.
  In the case that no wake up sources were provided but deep
  sleep was started, it will sleep forever unless hardware
  reset occurs.
  */
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
  Serial.println("This will never be printed");

}

void loop() {
  // this loop is never called
}


