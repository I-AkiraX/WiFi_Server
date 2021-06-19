#include <DHT.h>
#include <DHT_U.h>

#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define mqS 36

#define dhtPin 23
#define DHTTYPE DHT11
int dist;

DHT dht(dhtPin, DHTTYPE);

/*** WiFi Access Point *****/

#define WLAN_SSID       ""
#define WLAN_PASS       ""

/*** Adafruit.io Setup *****/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define IO_USERNAME  "Raghib"
#define IO_KEY       "aio_VvdP02bgmFmKepz9m0o8riKMNcfQ"

/** Global State (you don't need to change this!) **/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, IO_USERNAME, IO_KEY);




/**** Feeds *****/

// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>   //
const char pollutants_FEED[] = IO_USERNAME "/feeds/pollutants";
Adafruit_MQTT_Publish pollutants = Adafruit_MQTT_Publish(&mqtt, IO_USERNAME "/feeds/mq-135.pollution");

const char humidity_FEED[] = IO_USERNAME "/feeds/pollutants";
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, IO_USERNAME "/feeds/dht11.humidity");

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish   temperature = Adafruit_MQTT_Publish(&mqtt, IO_USERNAME "/feeds/dht11.temperature");

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe Lights = Adafruit_MQTT_Subscribe(&mqtt, IO_USERNAME "/feeds/lights");
Adafruit_MQTT_Subscribe fan = Adafruit_MQTT_Subscribe(&mqtt, IO_USERNAME "/feeds/fan");

/*** Sketch Code ****/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void setup() {
  Serial.begin(115200);
  delay(10);
  dht.begin();

  Serial.println(F("Adafruit MQTT demo"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to... ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&Lights);
}

uint32_t x = 0;

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
double sV=analogRead(mqS);
   Serial.print("airQuality: ");
  Serial.print(sV ,DEC);
  Serial.println("ppm");
  delay(200);

  MQTT_connect();
  float t = dht.readTemperature();
  float h =dht.readHumidity();
  float f =dht.readTemperature(true);
  if(isnan(h) || isnan(t) || isnan(f))
 {
  Serial.println(F("Failed to read from dth"));
   return;
  }
  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &Lights) {
      Serial.print(F("Got: "));
      Serial.println((char *)Lights.lastread);
    }
  }

  // Now we can publish stuff!
  Serial.print(F("\nSending dht11.temperature val "));
  Serial.print(t);
  Serial.print("...");
  if (! temperature.publish(t)){
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }
  if (! humidity.publish(h)){
    Serial.println(F("Failed"));
  }
   else {
    Serial.println(F("OK!"));
  }
  if (! pollutants.publish(sV)){
    Serial.println(("Failed"));
  }
   else {
    Serial.println(("OK!"));
  }
  
  

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  /*
    if(! mqtt.ping()) {
    mqtt.disconnect();
    }
  */
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
}
