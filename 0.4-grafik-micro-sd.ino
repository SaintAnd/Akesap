/*
Добавляем в SPIFFS index.html с нужным содержимым
*/
/*
  Заметки ESPшника - Урок 6 - Построение графиков по данным с датчиков в браузере.
  Mautoz Tech https://www.youtube.com/channel/UCWN_KT_CAjGZQG5grHldL8w
  Заметки ESPшника - https://www.youtube.com/channel/UCQAbEIaWFdARXKqcufV6y_g
*/
/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  Default DHT pin is D4 (GPIO2)
*********/

#include "HX711.h" //This library can be obtained here http://librarymanager/All#Avia_HX711

#define calibration_factor 7050.0 //This value is obtained using the SparkFun_HX711_Calibration sketch
float units;                       // Задаём переменную для измерений в граммах
float ounces;                      // Задаём переменную для измерений в унциях
int gram;                      // Задаём переменную 

#define LOADCELL_DOUT_PIN  D4
#define LOADCELL_SCK_PIN  D3
#ifdef ESP32
  #include <WiFi.h>
  #include <ESPAsyncWebServer.h>
  #include <SPIFFS.h>
#else
  #include <Arduino.h>
  #include <ESP8266WiFi.h>
  #include <Hash.h>
  #include <ESPAsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include <FS.h>
#endif
#include "DHT.h"
#define DHTPIN 2
DHT dht(DHTPIN, DHT11);

#ifndef STASSID
#define STASSID "rosel_cam"
#define STAPSK "88619675846"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

String temperature;
String humidity;
String gramstring;
AsyncWebServer server(80);

HX711 scale;

void setup(){
  Serial.begin(115200);
  Serial.println("HX711 scale demo");
  Serial.println("Readings:");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor); //This value is obtained by using the SparkFun_HX711_Calibration sketch
  scale.tare();	//Assuming there is no weight on the scale at start up, reset the scale to 0
  dht.begin();
  if (!SPIFFS.begin ()) {
    Serial.println ("An Error has occurred while mounting SPIFFS");
    return;
  }
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });
  server.on("/gramstring", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", gramstring.c_str());
  });
  server.begin();
}

void loop(){
    Serial.print("Reading: ");

    units = scale.get_units(10); 
    gram = scale.get_units(); 


  ounces = (units * 0.035274) * 10;    // Переводим вес из унций в граммы
  Serial.print(ounces);
//  Serial.print(scale.get_units(), 1); //scale.get_units() returns a float
  Serial.print(" Kg"); //You can change this to kg but you'll need to refactor the calibration_factor
  Serial.println();
  gramstring=String(ounces);
  delay(100);
}