// <a href="https://arduino.ru/forum/pesochnitsa-razdel-dlya-novichkov/sketch-s-mdns-na-esp8266-kompiliruetsya-no-imya-v-brauzere-n" title="https://arduino.ru/forum/pesochnitsa-razdel-dlya-novichkov/sketch-s-mdns-na-esp8266-kompiliruetsya-no-imya-v-brauzere-n" rel="nofollow">https://arduino.ru/forum/pesochnitsa-razdel-dlya-novichkov/sketch-s-mdns...</a>
// http://esp1.local
// Android - Service Browser - для поиска mDNS адресов
#include <ESP8266WiFi.h>                                                // Библиотека для создания Wi-Fi подключения (клиент или точка доступа)
#include <WiFiClient.h>
#include <ESP8266WebServer.h>                                           // Библиотека для управления устройством по HTTP (например из браузера)
#include <FS.h>                                                         // Библиотека для работы с файловой системой
#include <ESP8266mDNS.h>                                                // Библиотека МДНС, позволяющая обращаться к плате по имени, а не по адресу

MDNSResponder mdns;                                                     // назначаем экземпляр класса ESP8266mDNS

const char* ssid = "rosel_cam";                                             // Указываем сид сети, к которой будем подключаться
const char* password = "88619675846";                            // Указываем пароль сети, к которой будем подключаться
const int led = 4;
ESP8266WebServer server(80);                                            // Указываем на каком порту слушать вэбсервер

void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/plain", "Привет от esp8266!");
  digitalWrite(led, 0);
}

void setup() {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);  
  Serial.begin(115200);                                                   // Инициализируем вывод данных на серийный порт со скоростью 9600 бод
  while (!Serial) {
                                                                        // ждём, пока не откроется монитор последовательного порта
                                                                        // для того, чтобы отследить все события в программе
  }
  WiFi.mode(WIFI_OFF);                                                  // Останавливаем вайфай
  delay(1000);
  WiFi.mode(WIFI_STA);                                                  // Запускаем вайфай или возможно стартуем в режиме клиента, пока не разобрался с этой командой
  Serial.println(WiFi.macAddress());                                    // Выводим мак адрес
  WiFi.begin(ssid, password);                                           // Инициализируем подключение к указанной вайфай сети

 // WiFi.setHostname("esp1");                                             // Устанавливаем новое имя нашего клиента
//  Serial.println(WiFi.hostname());                                      // Распечатываем имя клиента
  while (WiFi.status() != WL_CONNECTED) {
  Serial.println("waiting...");  
  delay(1000);

  Serial.println(WiFi.localIP());  
  } 
   if (!MDNS.begin("esp1")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
   Serial.println("MDNS responder started");
  
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");  //активируем сервис MDNS  
  MDNS.addService("http", "tcp", 80); 

}

void loop() {
   MDNS.update();

  // put your main code here, to run repeatedly:
  server.handleClient();
}