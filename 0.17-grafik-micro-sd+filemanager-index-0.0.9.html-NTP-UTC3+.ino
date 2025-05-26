#include "HX711.h"
#include <SPI.h>
#include <SD.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <DHT.h>

#define DHTPIN D1          // Пин, к которому подключен DHT11
#define DHTTYPE DHT11     // Указываем тип датчика DHT11

DHT dht(DHTPIN, DHTTYPE);

#define calibration_factor 7050.0
float units;
float ounces;
int gram;

#define LOADCELL_DOUT_PIN  D4
#define LOADCELL_SCK_PIN  D3
#define SD_CS_PIN D8 // Пин для SD-карты

#ifdef ESP32
  #include <WiFi.h>
  #include <ESPAsyncWebServer.h>
  #include <SPIFFS.h>
#else
  #include <Arduino.h>
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include <FS.h>
#endif

#ifndef STASSID
#define STASSID "rosel_cam"
#define STAPSK "88619675846"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

String gramstring;
AsyncWebServer server(80);
HX711 scale;

// Настройка NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 10800, 60000); // Часовой пояс UTC+1

unsigned long lastDHTReadTime = 0; // Время последнего считывания DHT
const unsigned long DHTReadInterval = 10000; // Интервал считывания DHT (10 секунд)

float temperature = 0.0;
float humidity = 0.0;

void setup() {
  Serial.begin(115200);
  dht.begin(); // Инициализация DHT11
  Serial.println("HX711 scale demo");
  Serial.println("Readings:");
  
  // Инициализация весов
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare();

  // Инициализация SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Инициализация SD-карты
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    return;
  }

  // Создание файла, если он не существует
  if (!SD.exists("data.txt")) {
    File dataFile = SD.open("data.txt", FILE_WRITE);
    if (dataFile) {
      dataFile.println("Weight (grams),Timestamp"); // Заголовок
      dataFile.close();
    } else {
      Serial.println("Error creating data.txt");
    }
  }

  // Подключение к WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(5000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());

  // Инициализация NTP-клиента
  timeClient.begin();

  // Ожидание синхронизации времени
  while (!timeClient.update()) {
    Serial.println("Waiting for time synchronization...");
    delay(1000);
  }
  Serial.println("Time synchronized!");

  // Настройка маршрутов для сервера
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html");
  });

  server.on("/gramstring", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", gramstring.c_str());
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    File dataFile = SD.open("data.txt");
    if (dataFile) {
        String lines[11]; // Массив для хранения последних 7 строк
        int lineCount = 0;

        // Чтение файла построчно
        while (dataFile.available()) {
            String line = dataFile.readStringUntil('\n'); // Читаем строку до символа новой строки
            if (lineCount < 11) {
                lines[lineCount] = line; // Сохраняем строку в массив
                lineCount++;
            } else {
                // Если уже прочитали 7 строк, сдвигаем массив
                for (int i = 0; i < 10; i++) {
                    lines[i] = lines[i + 1]; // Сдвигаем строки влево
                }
                lines[10] = line; // Добавляем новую строку в конец
            }
        }
        dataFile.close();

        // Формируем ответ из последних 7 строк
        String data = "";
        for (int i = 0; i < lineCount; i++) {
            data += lines[i] + "\n"; // Добавляем строки в ответ
        }

        request->send(200, "text/plain", data);
    } else {
        request->send(500, "text/plain", "Failed to open file");
    }
  });

server.on("/all", HTTP_GET, [](AsyncWebServerRequest *request) {
    int page = request->getParam("page") ? request->getParam("page")->value().toInt() : 0;
    int pageSize = 15; // Количество строк на странице
    int startLine = page * pageSize;

    File dataFile = SD.open("data.txt");
    if (dataFile) {
        String data = "";
        int lineCount = 0;

        // Чтение файла построчно
        while (dataFile.available()) {
            String line = dataFile.readStringUntil('\n'); // Читаем строку до символа новой строки
            if (lineCount >= startLine && lineCount < startLine + pageSize) {
                data += line + "\n"; // Добавляем строку в ответ
            }
            lineCount++;
        }
        dataFile.close();

        // Отправляем данные
        request->send(200, "text/plain", data); 
    } else {
        request->send(500, "text/plain", "Failed to open file");
    }
});

// Обработчик для получения общего количества строк
server.on("/count", HTTP_GET, [](AsyncWebServerRequest *request) {
    File dataFile = SD.open("data.txt");
    if (dataFile) {
        int lineCount = 0;
        while (dataFile.available()) {
            dataFile.readStringUntil('\n'); // Читаем строку до символа новой строки
            lineCount++;
        }
        dataFile.close();
        request->send(200, "text/plain", String(lineCount)); // Отправляем общее количество строк
    } else {
        request->send(500, "text/plain", "Failed to open file");
    }
});


  // Файловый менеджер
  server.on("/filemanager", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<html><body><h1>File Manager</h1><ul>";
    File root = SD.open("/");
    File file = root.openNextFile();
    while (file) {
      html += "<li>";
      html += file.name();
      html += " - <a href=\"/view?file=";
      html += file.name();
      html += "\">view</a>";
      html += " - <a href=\"/delete?file=";
      html += file.name();
      html += "\" onclick=\"return confirm('Are you sure?');\">Delete</a>";
      html += "</li>";
      file = root.openNextFile();
    }
    html += "</ul></body></html>";
    request->send(200, "text/html", html);
  });

  // Просмотр файла
  server.on("/view", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("file")) {
        String filename = request->getParam("file")->value();
        File file = SD.open(filename);
        if (file) {
            String fileContent = "";
            while (file.available()) {
                fileContent += String((char)file.read()); // Читаем файл по одному байту
            }
            file.close();
            request->send(200, "text/plain", fileContent); // Отправляем содержимое файла
        } else {
            request->send(404, "text/plain", "File not found");
        }
    } else {
        request->send(400, "text/plain", "Bad Request");
    }
  });

  // Удаление файла
  server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("file")) {
      String filename = request->getParam("file")->value();
      if (SD.remove(filename)) {
        request->send(200, "text/plain", "File deleted");
      } else {
        request->send(404, "text/plain", "File not found");
      }
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

server.on("/calibrate", HTTP_GET, [](AsyncWebServerRequest *request) {
    scale.tare(); // Сброс веса
    request->send(200, "text/plain", "Вес сброшен"); // Отправляем ответ
});

server.on("/dht", HTTP_GET, [](AsyncWebServerRequest *request) {
    String jsonResponse = "{\"temperature\": " + String(temperature) + ", \"humidity\": " + String(humidity) + "}";
    request->send(200, "application/json", jsonResponse);
});


  server.begin();
}

void loop() {
  // Обновление времени
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime(); // Получение временной метки в формате Unix Epoch

  Serial.print("Reading: ");
  units = scale.get_units(15);
  ounces = (units * 0.035274) * 10; // Преобразование в унции
  Serial.print(ounces);
  Serial.print(" Kg");
  Serial.println();

  // Проверка времени для считывания DHT
  if (millis() - lastDHTReadTime >= DHTReadInterval) {
    lastDHTReadTime = millis(); // Обновление времени последнего считывания

    // Считывание данных с DHT
    temperature = dht.readTemperature(); // Считывание температуры
    humidity = dht.readHumidity(); // Считывание влажности

    // Проверка на ошибки считывания
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!"); // Сообщение об ошибке
      // Можно добавить дополнительные действия, например, повторить попытку считывания
      return; // Выход из функции loop, если произошла ошибка
    }
  }

  gramstring = String(ounces);

  // Запись данных в файл на SD-карте с временной меткой
  File dataFile = SD.open("data.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println(gramstring + "," + String(epochTime)); // Используем epochTime для временной метки
    dataFile.close();
  } else {
    Serial.println("Error opening data.txt");
  }

  delay(3600000); 
}
