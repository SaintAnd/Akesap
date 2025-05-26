#include <SPI.h>
#include <SD.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

const char *ssid = "rosel_cam"; // Замените на ваше имя сети
const char *password = "88619675846"; // Замените на ваш пароль

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  if (!SD.begin(D2)) { // Замените D2 на ваш пин CS для SD-карты
    Serial.println("SD card initialization failed!");
    return;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<h1>File Manager</h1><ul>";
    File root = SD.open("/");
    File file = root.openNextFile();
    while (file) {
      html += "<li>";
      html += file.name();
      html += " <a href='/view?file=";
      html += file.name();
      html += "'>View</a> ";
      html += "<a href='/download?file=";
      html += file.name();
      html += "'>Download</a> ";
      html += "<a href='/delete?file=";
      html += file.name();
      html += "'>Delete</a>";
      html += "</li>";
      file = root.openNextFile();
    }
    html += "</ul>";
    request->send(200, "text/html", html);
  });

  server.on("/view", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("file")) {
      String filename = request->getParam("file")->value();
      File file = SD.open(filename.c_str());
      if (file) {
        String content = "<h1>Viewing File: " + filename + "</h1><pre>";
        while (file.available()) {
          content += (char)file.read();
        }
        content += "</pre><a href='/'>Back</a>";
        request->send(200, "text/html", content);
        file.close();
      } else {
        request->send(404, "text/html", "File not found! <a href='/'>Back</a>");
      }
    } else {
      request->send(400, "text/html", "No file specified! <a href='/'>Back</a>");
    }
  });

  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("file")) {
      String filename = request->getParam("file")->value();
      File file = SD.open(filename.c_str());
      if (file) {
        request->send(file, filename, "application/octet-stream");
        file.close();
      } else {
        request->send(404, "text/html", "File not found! <a href='/'>Back</a>");
      }
    } else {
      request->send(400, "text/html", "No file specified! <a href='/'>Back</a>");
    }
  });

  server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("file")) {
      String filename = request->getParam("file")->value();
      if (SD.remove(filename.c_str())) {
        request->send(200, "text/html", "File deleted successfully! <a href='/'>Back</a>");
      } else {
        request->send(500, "text/html", "Failed to delete file! <a href='/'>Back</a>");
      }
    } else {
      request->send(400, "text/html", "No file specified! <a href='/'>Back</a>");
    }
  });

  server.begin();
}

void loop() {
  // Ваш основной код
}
