#include <BleKeyboard.h>
#include <Keypad.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// === Настройки Wi-Fi ===
const char* ssid = "Dom22";
const char* password = "Dom222222";

// === Пин для измерения напряжения батареи ===
const int batteryPin = 34;

// === Настройки клавиатуры ===
const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32}; 
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
BleKeyboard Keyboard("ESP32 Keypad", "Shelf Co.", 100); 

// === Сервер ===
AsyncWebServer server(80);

// HTML страница
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Уровень заряда</title>
    <style>
        body { font-family: Arial; text-align: center; margin-top: 50px; }
        h2 { font-size: 24px; }
    </style>
</head>
<body>
    <h1>ESP32 Клавиатура</h1>
    <h2 id="battery">Ожидание данных...</h2>
    
    <script>
        function fetchData() {
            fetch('/battery')
                .then(response => response.json())
                .then(data => {
                    document.getElementById("battery").innerText = "🔋 Уровень заряда: " + data.voltage.toFixed(2) + " В (" + data.percent + "%)";
                });
        }

        setInterval(fetchData, 5000);
        fetchData(); // первый запрос
    </script>
</body>
</html>
)rawliteral";

float getBatteryVoltage() {
  float measuredvbat = analogRead(batteryPin);
  measuredvbat *= 3.6 / 4095;
  measuredvbat *= 2;
  return measuredvbat;
}

int getBatteryPercent(float voltage) {
  float minVoltage = 3.0;
  float maxVoltage = 4.2;

  int percent = (voltage - minVoltage) / (maxVoltage - minVoltage) * 100;
  if (percent > 100) percent = 100;
  else if (percent < 0) percent = 0;
  return percent;
}

void setup() {
  Serial.begin(115200);

  Keyboard.begin();
  Serial.println("Клавиатура запущена! Подключите к Bluetooth устройству.");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Подключение к Wi-Fi...");
  }
  Serial.print("IP адрес: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/battery", HTTP_GET, [](AsyncWebServerRequest *request){
    float voltage = getBatteryVoltage();
    int percent = getBatteryPercent(voltage);
    String json = "{"
                  "\"voltage\":" + String(voltage, 2) + ","
                  "\"percent\":" + String(percent) +
                  "}";
    request->send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  char customKey = customKeypad.getKey();

  if (customKey) {
    Serial.print("Нажата: ");
    Serial.println(customKey);
    Keyboard.press(customKey);
    delay(50);
    Keyboard.releaseAll();
  }
}
