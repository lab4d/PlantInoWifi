//  Plant Monitor and Watering System
//  Lab4D 2019
//
//  Written for NodeMCU E12
//  Requires ESP8266FS SPIFFS plugin for webserver


//Libraries
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "DHTesp.h"

#include <FS.h>   //Include File System Headers

const char* imagefile = "/image.png";
const char* htmlfile = "/index.html";

//ThingSpeak Connection Settings
const char* server = "api.thingspeak.com";
String apiKey = "APIKEY";
String Channel = "CHANNELNAME";


//Variable Variables™
const char* MY_SSID = "WIFI NAME";
const char* MY_PWD = "WIFI PASSWORD";


bool uploadData = true;   //UPLOAD TO THINGSPEAK?
long onlineLoop = 420000; //Send interval in millisec - 15000 minimum (def: 420000)
long sensorLoop = 4200;  //Sensor Interval
int offlineLoop = 120; // Loop time when not sending data
int flipTimer = 200;
int waterTimer = 10000;
int sensLimit = 600; // 600 default


// Pin Variables
#define voltageFlipPinA 0 //D3
#define voltageFlipPinB 2 //D4
#define sensorPin 0 // A0
int DHTPin = 16; //D0
int LDRPin = 5; //D1
int PumpPin = 4; //D2
int Device1 = 14; // D5
int LEDL = 12; // D6
int LEDA = 13; // D7
int LEDB = 15; // D8


// Internal Variables
float temperature;
float humidity;
bool light;
int moisture;
unsigned long previousMillis = 0;
unsigned long sensMillis = 0;
bool firstRun = true; 
int sent =1;
int unsent;
String webPage;
bool water = false;
bool deviceStatus;
int wattempt;

MDNSResponder mdns;
ESP8266WebServer srv(80);
DHTesp dht;

// On http request
void handleRoot() {
  srv.sendHeader("Location", "/index.html",true);   //Redirect to html web page
  srv.send(302, "text/plane","");
}
/// Web Request Handling
void handleWebRequests(){
  if(loadFromSpiffs(srv.uri())) return;
  String message = "File Not Detected\n\n";
  message += "URI: ";
  message += srv.uri();
  message += "\nMethod: ";
  message += (srv.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += srv.args();
  message += "\n";
  for (uint8_t i=0; i<srv.args(); i++){
    message += " NAME:"+srv.argName(i) + "\n VALUE:" + srv.arg(i) + "\n";
  }
  srv.send(404, "text/plain", message);
  Serial.println(message);
}

void handleWater (){
pumpWater(waterTimer);
}

/// Device State Upload Handling
void handleRDevice() {
  String d = String(deviceStatus);
  srv.send(200, "text/plane", d); //Send to web page
}
/// Moisture Upload Handling
void handleRUpload() {
  String u = String(uploadData);
  srv.send(200, "text/plane", u); //Send to web page
}
/// Moisture Handling
void handleMoisture() {
  String m = String(moisture);
  srv.send(200, "text/plane", m); //Send to web page
}
/// Pump Handling
void handlePump() {
  String s = String(water);
  srv.send(200, "text/plane", s); //Send to web page
}
/// Temperature Handling
void handleTemp() {
  String t = String(temperature);
  srv.send(200, "text/plane", t); //Send to web page
}
/// Humidity Handling
void handleHum() {
  String h = String(humidity);
  srv.send(200, "text/plane", h); //Send to web page
}
/// Light Handling
void handleLight() {
  String l = String(light);
  srv.send(200, "text/plane", l); //Send to web page
}
/// Device State Button Handling
void handleDevice() {
 String deviceState1 = "OFF";
 String t_state = srv.arg("deviceState1"); 
 if(t_state == "1")
 {
  digitalWrite(Device1, HIGH); //LED ON
  deviceState1 = "ON"; //Feedback parameter
  deviceStatus = true;
  Serial.print("D+");
 }
 else
 {
  digitalWrite(Device1, LOW); //LED OFF
  deviceState1 = "OFF"; //Feedback parameter 
  deviceStatus = false;
  Serial.print("D-"); 
 }
 srv.send(200, "text/plane", deviceState1); //Send to web page
}
/// Upload Handling
void handleUpload() {
 String uploadState = "OFF";
 String t_state = srv.arg("upload"); 
 if(t_state == "1")
 {
  digitalWrite(Device1, HIGH); //LED ON
  uploadState = "ON"; //Feedback parameter
  uploadData = true;
  Serial.print("U+");
 }
 else
 {
  digitalWrite(Device1, LOW); //LED OFF
  uploadState = "OFF"; //Feedback parameter 
  uploadData = false;
  Serial.print("U-"); 
 }
 srv.send(200, "text/plane", uploadState); //Send to web page
}

/// Upload Timer Setting
void handleUploadTime() {
 String t_state = srv.arg("uploadTime"); 
 Serial.print("U=" + t_state);
 onlineLoop = t_state.toInt();
 //srv.send(200, "text/plane", t_state); //Send to web page
}
/// Handle Upload Timer
void handleRUploadTime() {
String t = String(onlineLoop);
  srv.send(200, "text/plane", t); //Send to web page
}

/// Moisture Limit Setting
void handleMoistureLim() {
 String t_state = srv.arg("moistureLim"); 
 Serial.print("ML=" + t_state);
 sensLimit = t_state.toInt();
 //srv.send(200, "text/plane", t_state); //Send to web page
}
/// Handle Upload Timer
void handleRMoistureLim() {
String t = String(sensLimit);
  srv.send(200, "text/plane", t); //Send to web page
}

/// Pump Timer Setting
void handlePumpTime() {
 String t_state = srv.arg("pumpTime"); 
 Serial.print("P=" + t_state);
 waterTimer = t_state.toInt();
 //srv.send(200, "text/plane", t_state); //Send to web page
}
/// Handle Pump Timer
void handleRPumpTime() {
String w = String(waterTimer);
  srv.send(200, "text/plane", w); //Send to web page
}

//Print Serial Sensors
void printSerial(){
    Serial.println();
    Serial.print ("Light: ");
    Serial.println(light);
    Serial.print("Temperature: ");
    Serial.println(String(temperature));
    Serial.print("Humidity: ");
    Serial.println(String(humidity));
    Serial.print("Moisture : ");
    Serial.println(String(moisture) + " / " + sensLimit);
    Serial.print("Pump : ");
    Serial.println(String(water));
    Serial.print("Device : ");
    Serial.println(String(deviceStatus));
}

void setup() {
  pinMode(Device1, OUTPUT);
  pinMode(LEDA, OUTPUT);
  pinMode(LEDB, OUTPUT);
  pinMode(LEDL, OUTPUT);
  pinMode(PumpPin, OUTPUT);
  pinMode(sensorPin, INPUT);
  
  pinMode(voltageFlipPinA, OUTPUT);
  pinMode(voltageFlipPinB, OUTPUT);

  dht.setup(DHTPin, DHTesp::DHT22);

  Serial.begin(115200);
  SPIFFS.begin();
  connectWifi();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  srv.on("/", handleRoot);
  srv.onNotFound(handleWebRequests);      
  srv.on("/readUpload", handleRUpload);
  srv.on("/setUpload", handleUpload);
  srv.on("/readDevice", handleRDevice);
  srv.on("/setDevice", handleDevice);
  srv.on("/setUploadTime", handleUploadTime);
  srv.on("/readUploadTime", handleRUploadTime);
  srv.on("/setMoistureLim", handleMoistureLim);
  srv.on("/readMoistureLim", handleRMoistureLim);
  srv.on("/setPumpTime", handlePumpTime);
  srv.on("/readPumpTime", handleRPumpTime);
  srv.on("/readTemp", handleTemp);
  srv.on("/readHum", handleHum);
  srv.on("/readLight", handleLight);
  srv.on("/readMoisture", handleMoisture);
  srv.on("/readPump", handlePump);
  srv.on("/waterPlant", handleWater);
  srv.begin();
  Serial.println("Server Started");
  
}

void loop() {
  
    srv.handleClient();

  // Get Sensor Data
  unsigned long sensMillis2 = millis();
  if (sensMillis2 - sensMillis >= sensorLoop) {
      sensMillis = sensMillis2;
      temperature = dht.getTemperature();
      humidity = dht.getHumidity();
      light = digitalRead(LDRPin);
      senseSoil();
      //digitalWrite(LEDL, !light); 
      //Serial.print("s");
      printSerial();
    }
    else {
      Serial.print(".");
    }
  
  // Send to ThingSpeak
  if (firstRun){
    senseSoil();
    firstRun = false;
  }
  if (uploadData){
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= onlineLoop) {
      previousMillis = currentMillis;
      digitalWrite(LEDL, HIGH);
      senseSoil();
      Serial.println("");

      // Send Data to ThingSpeak
      sendThing();
      
      digitalWrite(LEDL, LOW);
      
      if (water == true){
        pumpWater(waterTimer);
      }
      else {
        Serial.println("No water needed <3 "); 
      }
    }
    
  }
  else {
    unsent++;
    Serial.println("+++DATA UPLOAD: OFF - "+ String(unsent));
    printSerial();
    if (water == true){
        pumpWater(waterTimer);
      }
      else {
        Serial.println("No water needed <3"); 
      }
  }
  
  
  sensorOff();
  
  if (uploadData){
    delay(220);
  }
  else {
    delay(offlineLoop);
  }
}

/// Connect to Wifi
void connectWifi()
{
  delay(500);
  Serial.println("");
  Serial.println("Initializing");
  delay(100);
  Serial.print("Connecting to " + *MY_SSID);
  delay(500);
  WiFi.begin(MY_SSID, MY_PWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    wattempt++;
    if (wattempt >= 10){
      Serial.println("Connection failed, exiting...)");
      wattempt=0;
      return;
    }
  }
  Serial.println("");
  delay(500);
  Serial.println("Connected to "+ *MY_SSID);
  Serial.println("");
  if (mdns.begin("esp8266", WiFi.localIP()))
    Serial.println("MDNS responder started");
}

/// Update ThingSpeak
void sendThing() {
  WiFiClient client;
  Serial.println("+++DATA UPLOAD: ON - " + String(sent));
      // Check if Wifi is OK
  if (WiFi.status() != WL_CONNECTED){
    WiFi.disconnect(true);
    Serial.println("Connection Lost - Resetting Wifi");
    connectWifi();
    delay(500);
  }
  if (client.connect(server, 80)) {
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(light);
    postStr += "&field2=";
    postStr += String(temperature);
    postStr += "&field3=";
    postStr += String(humidity);
    postStr += "&field4=";
    postStr += String(moisture);
    postStr += "&field5=";
    postStr += String(water);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);

    sent++;
    printSerial();
  }
  
  else {
    printSerial();
    Serial.println("No connection - Data Not Sent");
    WiFi.disconnect(true);
    Serial.println("Connection Lost - Resetting Wifi");
    connectWifi();
    delay(500);
  }
  client.stop();
}//end send

// Read Soil Sensor
void senseSoil(){
  setSensorPolarity(true);
  delay(flipTimer);
  int valA1 = analogRead(sensorPin);
  delay(flipTimer);
  setSensorPolarity(false);
  delay(flipTimer);
  // invert the reading
  int valA2 = 1023 - analogRead(sensorPin);
  moisture = (valA1 + valA2) / 2;
  if (moisture <= sensLimit){
    water = true;
    digitalWrite(LEDA, LOW);
    digitalWrite(LEDB, HIGH);
  }
  else {
    water = false;
    digitalWrite(LEDA, HIGH);
    digitalWrite(LEDB, LOW);
  }
}

void setSensorPolarity(boolean flip){
  if(flip){
    digitalWrite(voltageFlipPinA, HIGH);
    digitalWrite(voltageFlipPinB, LOW);
  }else{
    digitalWrite(voltageFlipPinA, LOW);
    digitalWrite(voltageFlipPinB, HIGH);
  }
}

void sensorOff (){
  digitalWrite(voltageFlipPinA, LOW);
  digitalWrite(voltageFlipPinB, LOW);
}

void pumpWater(int time){
   Serial.print("Watering... ");
   digitalWrite(PumpPin, HIGH);
   delay(time);
   digitalWrite(PumpPin, LOW);
   Serial.println(" Done!");
}

/// SPIFFS File Handling
bool loadFromSpiffs(String path){
  String dataType = "text/plain";
  if(path.endsWith("/")) path += "index.htm";

  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if(path.endsWith(".html")) dataType = "text/html";
  else if(path.endsWith(".htm")) dataType = "text/html";
  else if(path.endsWith(".css")) dataType = "text/css";
  else if(path.endsWith(".js")) dataType = "application/javascript";
  else if(path.endsWith(".png")) dataType = "image/png";
  else if(path.endsWith(".gif")) dataType = "image/gif";
  else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".ico")) dataType = "image/x-icon";
  else if(path.endsWith(".xml")) dataType = "text/xml";
  else if(path.endsWith(".pdf")) dataType = "application/pdf";
  else if(path.endsWith(".zip")) dataType = "application/zip";
  File dataFile = SPIFFS.open(path.c_str(), "r");
  if (srv.hasArg("download")) dataType = "application/octet-stream";
  if (srv.streamFile(dataFile, dataType) != dataFile.size()) {
  }

  dataFile.close();
  return true;
}
