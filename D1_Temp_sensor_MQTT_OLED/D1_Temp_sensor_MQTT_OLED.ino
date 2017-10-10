#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>
#include "DHT.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_SSD1306.h"

#define DHTPIN D4     // what pin DHT is connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
 
DHT dht(DHTPIN, DHTTYPE);

//SCL GPIO5
//SDA GPIO4
#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);

 /*
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
 
 
#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16
*/

const char* mqtt_server = "192.168.1.68"; 
// This is my MQTT server, your IP will most likely be different
WiFiClient client;
PubSubClient mqttclient(client);

String temp_str;
char temp_char[50];

String hum_str;
char hum_char[50];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFiManager wifiManager;  
  wifiManager.autoConnect("AutoConnectAP");   // And if that doesn't work, set up AutoConnectAP for config
  mqttclient.setServer(mqtt_server, 1883);    // Connect to MQTT Server
  //mqttclient.setCallback(callback);           // And set the callback
  dht.begin();
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)
  // init done

 display.display();
}

/*
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic); // For troubleshooting
  Serial.print("] "); 
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
}
*/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // The client name nas to be changed for each new node!
    if (mqttclient.connect("Livingroom_sensor")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  mqttclient.publish("/inside/livingroom/humidity/", hum_char);
}


void loop() {
  mqttclient.loop(); // Do the MQTT loop
  unsigned long currentMillis = millis(); // Get current time to compare to last time we checked.

  // Humidity and temperature init
  float h = dht.readHumidity();
  float t = dht.readTemperature();

   // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) 
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Serial print for debugging
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  
  if (!client.connected()) {
    reconnect();
  }

  temp_str = String(t); //converting to string
  temp_str.toCharArray(temp_char, temp_str.length() + 1); //packaging up the data in order to publish to MQTT
  mqttclient.publish("/inside/livingroom/temperature/", temp_char);
  delay(2000);
  hum_str = String(h); //converting to string
  hum_str.toCharArray(hum_char, hum_str.length() + 1); //packaging up the data in order to publish to MQTT
  mqttclient.publish("/inside/livingroom/humidity/", hum_char);
  Serial.print("Humidity ");
  Serial.println(hum_char);

  // Clear the buffer.
  display.clearDisplay();
 
  // text display tests
  display.setTextSize(2);
  display.setTextColor(WHITE);
  // For some reason, the 64*48 screen isn't detected properly, so a bit of padding is needed.
  display.setCursor(35,12);
  
  display.print(t);
  display.display();
  // Wait 5 minutes before next run
  delay(300000);
}
