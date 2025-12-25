#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* ================= WIFI ================= */
const char* ssid = "Surender";
const char* password = "12345678";

/* ================= SERVER ================= */
ESP8266WebServer server(80);

/* ================= OLED ================= */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/* ================= ACS712 ================= */
const int sensorPin = A0;
float sensitivity = 0.185;   // 5A ACS712

/* ================= MEASUREMENTS ================= */
float currentA = 0.0;
float voltageV = 230.0;     // Fixed (can upgrade later)
float powerW  = 0.0;

/* ================= API: /data ================= */
void handleData() {
  String json = "{";
  json += "\"current\":" + String(currentA, 2) + ",";
  json += "\"voltage\":" + String(voltageV, 1) + ",";
  json += "\"power\":" + String(powerW, 1);
  json += "}";

  // 🔑 CORS HEADER (VERY IMPORTANT for GitHub Pages)
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);

  /* OLED init */
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    while (true);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  /* WiFi */
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("ESP8266 IP: ");
  Serial.println(WiFi.localIP());

  /* Web server */
  server.on("/data", handleData);
  server.begin();
  Serial.println("HTTP server started");
}

/* ================= LOOP ================= */
void loop() {
  /* Read ACS712 */
  int adc = analogRead(sensorPin);
  float voltage = (adc / 1023.0) * 5.0;
  currentA = (voltage - 2.5) / sensitivity;
  if (currentA < 0) currentA = 0;

  powerW = currentA * voltageV;

  /* OLED Display */
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Smart Energy Meter");

  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print("I:");
  display.print(currentA, 1);
  display.print("A");

  display.setTextSize(1);
  display.setCursor(0, 44);
  display.print("P:");
  display.print(powerW, 1);
  display.print(" W");

  display.display();

  server.handleClient();
  delay(500);
}
