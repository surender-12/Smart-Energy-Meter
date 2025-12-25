#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* ================= WIFI ================= */
const char* ssid = "Surender";
const char* password = "12345678";

/* ============ STATIC IP ============ */
IPAddress local_IP(10, 92, 236, 187);
IPAddress gateway(10, 92, 236, 1);
IPAddress subnet(255, 255, 255, 0);

/* ================= SERVER ================= */
ESP8266WebServer server(80);

/* ================= OLED ================= */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/* ================= ACS712 ================= */
const int sensorPin = A0;
float sensitivity = 0.185;   // ACS712 5A

/* ================= L298N ================= */
#define ENA D5   // PWM
#define IN1 D6
#define IN2 D7

/* ================= MOTOR SPEED LOGIC ================= */
int motorSpeed = 80;              // Default speed
unsigned long lastChange = 0;
int motorState = 0;
/*
  motorState:
  0 = running at 120 (normal)
  1 = running at 200 (boost)
*/

/* ================= MEASUREMENTS ================= */
float currentA = 0.0;
float voltageV = 12.0;     // Motor supply voltage (demo)
float powerW  = 0.0;

/* ================= API ================= */
void handleData() {
  String json = "{";
  json += "\"current\":" + String(currentA, 2) + ",";
  json += "\"voltage\":" + String(voltageV, 1) + ",";
  json += "\"power\":" + String(powerW, 1);
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleRoot() {
  server.send(200, "text/plain",
    "ESP8266 Smart Energy Meter\nUse /data endpoint");
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);

  /* OLED */
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  /* Motor pins */
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);   // Forward direction
  analogWrite(ENA, motorSpeed);

  /* Static IP */
  WiFi.config(local_IP, gateway, subnet);

  /* WiFi */
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  /* Server */
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  lastChange = millis();
}

/* ================= LOOP ================= */
void loop() {
  unsigned long now = millis();

  /* -------- MOTOR TIMING LOGIC -------- */
  if (motorState == 0 && (now - lastChange >= 5000)) {
    // After 5 sec → boost speed
    motorSpeed = 120;
    analogWrite(ENA, motorSpeed);
    motorState = 1;
    lastChange = now;
  }
  else if (motorState == 1 && (now - lastChange >= 3000)) {
    // After 3 sec → back to normal
    motorSpeed = 80;
    analogWrite(ENA, motorSpeed);
    motorState = 0;
    lastChange = now;
  }

  /* -------- READ ACS712 -------- */
  int adc = analogRead(sensorPin);
  float v = (adc / 1023.0) * 5.0;
  currentA = (v - 2.5) / sensitivity;
  if (currentA < 0) currentA = 0;

  powerW = currentA * voltageV;

  /* -------- OLED DISPLAY -------- */
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("DC Motor Demo");

  display.setCursor(0, 12);
  display.print("Speed: ");
  display.print(motorSpeed);

  display.setTextSize(2);
  display.setCursor(0, 26);
  display.print("I:");
  display.print(currentA, 1);
  display.print("A");

  display.setTextSize(1);
  display.setCursor(0, 52);
  display.print("P:");
  display.print(powerW, 1);
  display.print("W");

  display.display();

  server.handleClient();
  delay(200);
}

