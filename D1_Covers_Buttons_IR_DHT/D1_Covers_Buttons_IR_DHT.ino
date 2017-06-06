#include <Servo.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>
#include "DHT.h"

  // Servo
  Servo servo1;
  int servoPin = 14;

  // DHT
  #define DHTPIN 16     // what pin DHT is connected to
  #define DHTTYPE DHT22   // DHT 22  (AM2302)
  DHT dht(DHTPIN, DHTTYPE);
  String temp_str;
  char temp_char[50];

  String hum_str;
  char hum_char[50];

  // Buttons
  //int goodNight = 4;
  //int goodMorning = 5;
  int movementSensor = 12; 
  int buttonUp= 5 ; // define the button pin
  int buttonDown = 4;
  int pressUp = 0;
  int pressDown = 0;
  boolean coverStatus = false;
  boolean justStarted = true;

  // Networks & MQTT
  const char* mqtt_server = "192.168.31.68"; 
  // This is my MQTT server, your IP will most likely be different
  WiFiClient client;
  PubSubClient mqttclient(client);

  char msg[50];
  unsigned long startMillis = 0;
  const long upInterval = 13000;
  const long downInterval = 11210;
  const long changeInterval = 10000;   

  int previousDetectorValue = 0;
  int currentDetectorValue = 1;
  String detector_str;
  char detector_char[50];
 


  // Long and short press detection
  boolean upButtonActive = false;
  boolean upLongPressActive = false;
  long upButtonTimer = 0;
  long upLongPressTime = 250;

  boolean downButtonActive = false;
  boolean downLongPressActive = false;
  long downButtonTimer = 0;
  long downLongPressTime = 250;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFiManager wifiManager;  
  wifiManager.autoConnect("AutoConnectAP");   // And if that doesn't work, set up AutoConnectAP for config
  mqttclient.setServer(mqtt_server, 1883);    // Connect to MQTT Server
  mqttclient.setCallback(callback);           // And set the callback
  dht.begin();
  
  pinMode(buttonUp, INPUT_PULLUP);
  pinMode(buttonDown, INPUT_PULLUP);
  pinMode(movementSensor, INPUT);
  
  servo1.attach(servoPin);
  coverStatus = false;
  Serial.print("Setup Done");
  
 
  
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic); // For troubleshooting
  Serial.print("] "); 
  for (int i = 0; i < length; i++) {
    Serial.println((char)payload[i]);
  }
  Serial.print("Cover status  ");
  Serial.println(coverStatus);
  // If topic is 1, close fully
  if (((char)payload[0] == '1')  && coverStatus == false && justStarted == false) { 
    Serial.println("Closing cover");
    startMillis = millis();
    Serial.print("Start millis");
    Serial.println(startMillis);
    Serial.println("Closing");
    while(millis() - startMillis < downInterval) {
      servo1.attach(servoPin);
      servo1.write(180);
      yield();
      
    }
    mqttclient.publish("/inside/bedroom/cover/", "CLOSED");  // Notify MQTT    
    servo1.detach();  // Stop the rotation
    Serial.println("Fully Closed");
    coverStatus = true;
    Serial.print("Cover status  ");
    Serial.println(coverStatus);
    // justStarted is set at system reboot. If this is not here, it's only possible to close after restart.
    justStarted = false;
  }
  // If payload is 0 open fully
  else if (((char)payload[0] == '0') && (coverStatus == true || justStarted == true)) {
    Serial.println("Opening cover");
    startMillis = millis();
    Serial.print("Start millis");
    Serial.println(startMillis);
    Serial.println("Opening");
    while(millis() - startMillis < upInterval) {
      servo1.attach(servoPin);
      servo1.write(0);
      // If we don't use yield, the Watchdog trips for some reason.
      yield();
      
    }
    mqttclient.publish("/inside/bedroom/cover/", "OPEN");  // Notify MQTT    
    servo1.detach();  // Stop the rotation
    Serial.println("Fully Open");
    coverStatus = false;
    Serial.print("Cover status  ");
    Serial.println(coverStatus);
  }
  else {
    Serial.print("Payload ");
    Serial.print((char)payload[0]);
    Serial.print(", cover status  ");
    Serial.print(coverStatus);
    justStarted = false;
  }

}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // The client name nas to be changed for each new node!
    if (mqttclient.connect("Bedroom_Sensor_1")) {
      Serial.println("connected");
      mqttclient.subscribe("/inside/bedroom/cover/set");
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
  pressUp = digitalRead(buttonUp);
  pressDown = digitalRead(buttonDown);

  // Humidity and temperature init
  //float h = dht.readHumidity();
  //float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  /*
  if (isnan(h) || isnan(t)) 
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
    delay(100);
  }
    */
  // Serial print for debugging
  /*
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
*/
  if (!client.connected()) {
    reconnect();
  }

  //temp_str = String(t); //converting to string
  //temp_str.toCharArray(temp_char, temp_str.length() + 1); //packaging up the data in order to publish to MQTT
  //mqttclient.publish("/inside/bedroom/temperature/", temp_char);
  //delay(2000);
  //hum_str = String(h); //converting to string
  //hum_str.toCharArray(hum_char, hum_str.length() + 1); //packaging up the data in order to publish to MQTT
  //mqttclient.publish("/inside/bedroom/humidity/", hum_char);
  //Serial.print("Humidity ");
  //Serial.println(hum_char);

  if (digitalRead(buttonUp) == 0) {
    if (upButtonActive == false) {
      upButtonActive = true;
      upButtonTimer = millis();
    }
    if ((millis() - upButtonTimer > upLongPressTime) && (upLongPressActive == false)) {
      upLongPressActive = true;
      Serial.println("Up Long Press");
    }
  } else {

    if (upButtonActive == true) {

      if (upLongPressActive == true) {

        upLongPressActive = false;

      } else {
        mqttclient.publish("/inside/bedroom/cover/set", "0");
        Serial.println("Up Short Press");

      }

      upButtonActive = false;

    }

  }
  servo1.detach();  // Stop the rotation
  while (upLongPressActive == true) {
    servo1.attach(servoPin);
    //Serial.println("Up pressed");
    servo1.write(0);  // Values below 90 rotate one way, values above 90 rotate the other
    yield();
    if(digitalRead(buttonUp) == 1) {
      upLongPressActive = false;
      upButtonActive = false;
    }
  }
  servo1.detach();  // Stop the rotation



// Down
if (digitalRead(buttonDown) == 0) {
    if (downButtonActive == false) {
      downButtonActive = true;
      downButtonTimer = millis();
    }
    if ((millis() - downButtonTimer > downLongPressTime) && (downLongPressActive == false)) {
      downLongPressActive = true;
      //Serial.println("down Long Press");
    }
  } else {

    if (downButtonActive == true) {

      if (downLongPressActive == true) {

        downLongPressActive = false;

      } else {
        mqttclient.publish("/inside/bedroom/cover/set", "1");
        Serial.println("down Short Press");

      }

      downButtonActive = false;

    }

  }
  servo1.detach();  // Stop the rotation
  while (downLongPressActive == true) {
    servo1.attach(servoPin);
    //Serial.println("down pressed");
    servo1.write(180);  // Values below 90 rotate one way, values above 90 rotate the other
    yield();
    if(digitalRead(buttonDown) == 1) {
      downLongPressActive = false;
      downButtonActive = false;
    }
  }
  servo1.detach();  // Stop the rotation


  // Time to report movement data to MQTT server
  //if (millis() - previousDetectorMillis < DetectorchangeInterval) {
  currentDetectorValue = digitalRead(movementSensor);
    if (currentDetectorValue != previousDetectorValue) {
      detector_str = String(currentDetectorValue); //converting to string
      detector_str.toCharArray(detector_char, detector_str.length() + 1); //packaging up the data in order to publish to MQTT
      mqttclient.publish("/inside/bedroom/movement/", detector_char);
      Serial.print("Sensor value: ");
      Serial.println(currentDetectorValue);
      previousDetectorValue = currentDetectorValue;
    }

    
  //}
  
  //val = digitalRead(button);
  //Serial.println(val);

  //delay(100);

}
