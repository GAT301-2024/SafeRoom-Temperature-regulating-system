#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define GREEN_LED 27
#define RED_LED 25
#define BLUE_LED 26

#define LDR_PIN 35
#define RELAY_PIN 33

const float T0 = 37.0;
const float threshold = 5.0;

unsigned long hotStartTime = 0;
unsigned long coldStartTime = 0;
bool hotTimerRunning = false;
bool coldTimerRunning = false;

String currentState = "NORMAL";
float currentTemp = 0.0;

const char* ssid = "GEEH ENTERPRISES_UL";
const char* password = "Geeh37425#.";

WebServer server(80);

// ==========================
// 📡 SETUP
// ==========================
void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Setup Web Server
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

// ==========================
// 🔁 LOOP
// ==========================
void loop() {
  server.handleClient();

  float temp = dht.readTemperature();
  if (isnan(temp)) {
    Serial.println("DHT read failed.");
    delay(1000);
    return;
  }

  currentTemp = temp;
  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.print("°C - ");

  // =======================
  // NORMAL
  // =======================
  if (temp >= T0 - threshold && temp <= T0 + threshold) {
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(RELAY_PIN, LOW);
    currentState = "NORMAL";
    Serial.println("State: NORMAL");
    resetTimers();
  }

  // =======================
  // HOT
  // =======================
  else if (temp > T0 + threshold) {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(RED_LED, HIGH);

    if (!hotTimerRunning) {
      hotTimerRunning = true;
      hotStartTime = millis();
    }

    if (millis() - hotStartTime >= 60000) {
      if (analogRead(LDR_PIN) > 1000) {
        digitalWrite(RELAY_PIN, HIGH);
        currentState = "HOT";
        Serial.println("State: HOT - AC ON");
      }
    }

    coldTimerRunning = false;
    coldStartTime = 0;
  }

  // =======================
  // COLD
  // =======================
  else if (temp < T0 - threshold) {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, HIGH);

    if (!coldTimerRunning) {
      coldTimerRunning = true;
      coldStartTime = millis();
    }

    if (millis() - coldStartTime >= 60000) {
      if (analogRead(LDR_PIN) > 1000) {
        digitalWrite(RELAY_PIN, HIGH);
        currentState = "COLD";
        Serial.println("State: COLD - AC ON");
      }
    }

    hotTimerRunning = false;
    hotStartTime = 0;
  }

  delay(1000);
}

// ==========================
// 🧹 TIMER RESET
// ==========================
void resetTimers() {
  hotTimerRunning = false;
  coldTimerRunning = false;
  hotStartTime = 0;
  coldStartTime = 0;
}

// ==========================
// 🌐 WEB SERVER ROOT PAGE
// ==========================
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>SafeRoom Temp Dashboard</title>";
  html += "<style>";
  html += "body{font-family:Arial;text-align:center;background:#f7f7f7;padding:30px;}";
  html += ".card{background:#fff;padding:20px;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,0.1);display:inline-block;}";
  html += "h2{color:#333;}";
  html += ".state{font-size:24px;font-weight:bold;}";
  html += ".temp{font-size:40px;color:#007BFF;}";
  html += ".relay{margin-top:10px;font-size:18px;color:#555;}";
  html += "</style></head><body>";
  html += "<div class='card'>";
  html += "<h2>Room Temperature Monitor</h2>";
  html += "<div class='temp'>" + String(currentTemp, 1) + " °C</div>";
  html += "<div class='state'>State: " + currentState + "</div>";
  html += "<div class='relay'>AC Status: " + String(digitalRead(RELAY_PIN) ? "ON" : "OFF") + "</div>";
  html += "<div class='relay'>WiFi Signal: " + String(WiFi.RSSI()) + " dBm</div>";
  html += "<p><small>Updated every second</small></p>";
  html += "</div></body></html>";

  server.send(200, "text/html", html);
}
