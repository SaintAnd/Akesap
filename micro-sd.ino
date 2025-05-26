#include <SPI.h>
#include <SDFS.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[SD Reader]");

  // Инициализация microSD (без указания CS-пина!)
  if (!SDFS.begin()) {
    Serial.println("❌ microSD не обнаружена или не инициализирована!");
    while (true); // Остановить программу
  }
  Serial.println("✅ microSD инициализирована");

  // Открытие корневой директории
  File root = SDFS.open("/", "r"); // Указываем режим "r" для чтения
  if (!root) {
    Serial.println("❌ Не удалось открыть корневую папку");
    return;
  }

  Serial.println("\n📄 Файлы в корне:");
  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    Serial.print(" - ");
    Serial.print(fileName);
    if (file.isDirectory()) {
      Serial.println("/");
    } else {
      Serial.print(" (");
      Serial.print(file.size());
      Serial.println(" байт)");
    }
    file = root.openNextFile();
  }
  file.close();
  root.close();
}

void loop() {
  // Повторять каждые 10 секунд
  delay(10000);
  Serial.println("\n🔄 Обновление списка файлов...");
}
