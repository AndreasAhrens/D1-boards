#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>

#include <SPI.h>
#include <Wire.h>



const char* mqtt_server = "192.168.31.68"; 
const char* mqtt_name = "Plant watering";
// This is my MQTT server, your IP will most likely be different
WiFiClient client;
PubSubClient mqttclient(client);

char status;
String temp_str;
char temp_char[50];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFiManager wifiManager;  
  wifiManager.autoConnect("AutoConnectAP");   // And if that doesn't work, set up AutoConnectAP for config
  mqttclient.setServer(mqtt_server, 1883);    // Connect to MQTT Server
  mqttclient.setCallback(callback);           // And set the callback
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic); // For troubleshooting
  Serial.print("] "); 
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
}


void packDataForMQTT (char inputValue) {
  
  temp_str = String(inputValue); //converting to string
  temp_str.toCharArray(temp_char, temp_str.length() + 1); //packaging up the data in order to publish to MQTT
}

void sendMQTT () {
  Serial.println("Here we print the MQTT message");
  packDataForMQTT(status);
  Serial.print("Status: ");
  Serial.println(status);
  mqttclient.publish("/inside/livingroom/plantwatering/status", temp_char);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // The client name nas to be changed for each new node!
    if (mqttclient.connect(mqtt_name)) {
      Serial.println("connected");
      mqttclient.subscribe("/inside/livingroom/plantwatering/set");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void loop() {
  mqttclient.loop(); // Do the MQTT loop
  unsigned long currentMillis = millis(); // Get current time to compare to last time we checked.

  if (!client.connected()) {
    reconnect();
  }
}
