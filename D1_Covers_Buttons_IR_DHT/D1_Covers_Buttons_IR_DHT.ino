#include <eBtn.h>
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

  // Networks & MQTT
  const char* mqtt_server = "192.168.31.68"; 
  // This is my MQTT server, your IP will most likely be different
  // Set separate MQTT client name for each node
  const char* mqtt_client = "Bedroom_Sensor_1";
  
  // Buttons
  //int goodNight = 4;
  //int goodMorning = 5;
  int movementSensor = 12; 
  int buttonUp= 5 ; // define the button pin
  int buttonDown = 4;

  long longPressTime = 250;
  const long upInterval = 13000;    // Time to roll up curtain, 13 seconds
  const long downInterval = 11210;  // Time to roll down curtain, lower since down uses less power

  eBtn btnUp = eBtn(buttonUp);
  eBtn btnDown = eBtn(buttonDown);
  float n;
  float sinceN;
  
  //int pressUp = 0;
  //int pressDown = 0;
  boolean coverStatus = false;
  boolean justStarted = true;

  
  WiFiClient client;
  PubSubClient mqttclient(client);

   
  unsigned long startMillis = 0;
  

  // Motion detector 
  int previousDetectorValue = 0; // Initial status for previous value
  int currentDetectorValue = 1;  // Initial status for current value 
  
  int direction = 0;
 
 // Long and short press detection with timings

  boolean buttonActive = false;
  boolean longPressActive = false;
  long buttonTimer = 0;

  int servoValue = 0;

  String temp_str;
  char temp_char[50];
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFiManager wifiManager;  
  wifiManager.autoConnect("AutoConnectAP");   // And if that doesn't work, set up AutoConnectAP for config
  mqttclient.setServer(mqtt_server, 1883);    // Connect to MQTT Server
  mqttclient.setCallback(callback);           // And set the callback
  dht.begin();

  // Set pinmodes for the buttons and the sensor
  pinMode(buttonUp, INPUT_PULLUP);
  pinMode(buttonDown, INPUT_PULLUP);
  pinMode(movementSensor, INPUT);
  btnUp.on("press",pressUpFunc);
  btnDown.on("press",pressDownFunc);
  btnUp.on("release",releaseUpFunc);
  btnDown.on("release",releaseDownFunc);
  btnUp.on("hold",holdUpFunc);
  btnUp.on("long",longUpPressFunc);
  btnDown.on("hold",holdDownFunc);
  btnDown.on("long",longDownPressFunc);
  
  attachInterrupt(buttonUp, pinUp_ISR, CHANGE);
  attachInterrupt(buttonDown, pinDown_ISR, CHANGE);

  // Attach the servo and set initial status
  servo1.attach(servoPin);
  coverStatus = false;
  Serial.print("Setup Done");
  
 
  
}

void pinUp_ISR(){
  btnUp.handle();
}
void pinDown_ISR(){
  btnDown.handle();
}

void longUpPressFunc(){
  Serial.println("Btn released after a long press of " + String((millis()-n) /1000) + " seconds");  
  servo1.detach();
  longPressActive = false;
  n = 0;
}

void holdUpFunc(){
  Serial.println("Btn hold for: " + String((millis()-n) /1000) + " seconds");
  //servo1.detach();
  //n = 0;
}

void longDownPressFunc(){
  Serial.println("Btn released after a long press of " + String((millis()-n) /1000) + " seconds");  
  servo1.detach();
  longPressActive = false;
  n = 0;
}

void holdDownFunc(){
  Serial.println("Btn hold for: " + String((millis()-n) /1000) + " seconds");
  //servo1.detach();
  //n = 0;
}

void pressUpFunc(){
  n = millis();
  direction = 0;
  Serial.println("Up button pressed");
}

void releaseUpFunc(){ 
  Serial.println("Btn released after " + String((millis()-n) /1000) + " seconds");
  longPressActive = false;
  buttonActive = false;
  servo1.detach();
  n = 0;
}

void pressDownFunc(){
  n = millis();
  direction = 1;
  Serial.println("Down button pressed");
}

void releaseDownFunc(){ 
  Serial.println("Btn released after " + String((millis()-n) /1000) + " seconds");
  longPressActive = false;
  buttonActive = false;
  servo1.detach();
  n = 0;
}


void packDataForMQTT (int inputValue) {
  
  temp_str = String(inputValue); //converting to string
  temp_str.toCharArray(temp_char, temp_str.length() + 1); //packaging up the data in order to publish to MQTT
}

void sustainedMotion () {
  servo1.attach(servoPin);
  servoValue = direction * 180;
  servo1.write(servoValue);  // Values below 90 rotate one way, values above 90 rotate the other  
}

void sendMQTT () {
  Serial.println("Here we print the MQTT message");
  buttonActive = true;
  packDataForMQTT(direction);
  Serial.print("Direction: ");
  Serial.println(direction);
  mqttclient.publish("/inside/bedroom/cover/set", temp_char);
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
  
  // If topic is 1, close fully. We also check if the coverStatus (previous status) was up or down.
  // This way, we can never send an Open command if it's already open and can never send an 
  // Close command if it's already closed. If just started, don't roll down.
  if (((char)payload[0] == '1')  && coverStatus == false && justStarted == false) { 
    Serial.println("Closing cover");
    startMillis = millis(); // Set start time
    // Run as long as it's not fully Closed
    while(millis() - startMillis < downInterval) {
      servo1.attach(servoPin);
      servo1.write(180);
      yield(); // needed to make sure the Watchdog doesn't time out. Doesn't do anything else.
      
    }
    mqttclient.publish("/inside/bedroom/cover/", "CLOSED");  // Notify MQTT    
    servo1.detach();  // Stop the rotation when done
    Serial.println("Fully Closed");
    coverStatus = true;
    // justStarted is set at system reboot. If this is not here, it's only possible to close after restart.
    justStarted = false;
  }
  
  // If payload is 0 open fully. The rest, same as above but inverted.
  else if (((char)payload[0] == '0') && (coverStatus == true || justStarted == true)) {
    Serial.println("Opening cover");
    startMillis = millis();
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
    if (mqttclient.connect(mqtt_client)) {
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
  if (!client.connected()) {
    reconnect();
  }
  
  sinceN = millis() - n;
  if (n != 0 && sinceN > longPressTime && longPressActive == false) {
    if (sinceN > longPressTime) {
      //Serial.print("Milliseconds since N: ");
      //Serial.println(sinceN);
      longPressActive = true;
      yield();
      sustainedMotion();
    }
  } 
  else if (buttonActive == false) {
      sendMQTT();
  }


  // Time to report movement data to MQTT server
  // I'm getting detections even when nobody is in the room, but only for very short times. 
  // TODO: reliably report motion. Possibly set minimum time for positive detection to a second or so? Maybe 100sm?
  //if (millis() - previousDetectorMillis < DetectorchangeInterval) {
  currentDetectorValue = digitalRead(movementSensor);
    if (currentDetectorValue != previousDetectorValue) {
      packDataForMQTT(currentDetectorValue);
      mqttclient.publish("/inside/bedroom/movement/", temp_char);
      Serial.print("Sensor value: ");
      Serial.println(currentDetectorValue);
      previousDetectorValue = currentDetectorValue;
    }


  // Humidity and temperature init
  // The damn DHT doesn't work for some reason, so comment out until I have time to fix.
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
  
  /*
  temp_str = String(t); //converting to string
  temp_str.toCharArray(temp_char, temp_str.length() + 1); //packaging up the data in order to publish to MQTT
  mqttclient.publish("/inside/bedroom/temperature/", temp_char);
  //delay(2000);
  hum_str = String(h); //converting to string
  hum_str.toCharArray(hum_char, hum_str.length() + 1); //packaging up the data in order to publish to MQTT
  mqttclient.publish("/inside/bedroom/humidity/", hum_char);
  Serial.print("Humidity ");
  Serial.println(hum_char);
  */
  // Let's try to call the new function and see if it works.


}
