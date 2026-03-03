#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ---------- WIFI DETAILS ----------
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

WiFiServer server(80);

// ---------- PIN DEFINITIONS ----------
#define CURRENT_PIN 34
#define VOLTAGE_PIN 35
#define TEMP_PIN 4
#define RELAY_PIN 5

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

// ---------- VARIABLES ----------
float voltage = 0;
float current = 0;
float power = 0;
float energy = 0;
unsigned long lastTime = 0;

void setup() {
  Serial.begin(115200);

  sensors.begin();
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  analogReadResolution(12);

  // Connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void calculateEnergyData() {

  int samples = 500;
  float currentSum = 0;
  float voltageSum = 0;

  for (int i = 0; i < samples; i++) {

    float currentReading = analogRead(CURRENT_PIN);
    float currentVolt = (currentReading * 3.3) / 4095.0;
    float currentCentered = currentVolt - 1.65;
    currentSum += currentCentered * currentCentered;

    float voltageReading = analogRead(VOLTAGE_PIN);
    float voltageOut = (voltageReading * 3.3) / 4095.0;
    voltageSum += voltageOut * voltageOut;
  }

  current = sqrt(currentSum / samples) * 30.0;
  voltage = sqrt(voltageSum / samples) * 311.0;

  power = voltage * current;

  unsigned long now = millis();
  float hours = (now - lastTime) / 3600000.0;
  energy += (power * hours) / 1000.0;
  lastTime = now;

  sensors.requestTemperatures();
}

void loop() {

  WiFiClient client = server.available();
  if (!client) return;

  while (!client.available()) delay(1);
  while (client.available()) client.read();

  calculateEnergyData();
  float temperature = sensors.getTempCByIndex(0);

  String relayStatus = digitalRead(RELAY_PIN) ? "ON" : "OFF";

  // -------- HTML PAGE --------
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta http-equiv='refresh' content='5'>";
  html += "<style>";
  html += "body{background:#111;color:white;font-family:Arial;text-align:center;}";
  html += ".card{background:#222;padding:20px;margin:20px;border-radius:10px;}";
  html += "h1{color:#00ffcc;}";
  html += "</style></head><body>";

  html += "<h1>Smart Home Energy Monitor</h1>";

  html += "<div class='card'>";
  html += "<h2>Voltage: " + String(voltage,2) + " V</h2>";
  html += "<h2>Current: " + String(current,2) + " A</h2>";
  html += "<h2>Power: " + String(power,2) + " W</h2>";
  html += "<h2>Energy: " + String(energy,3) + " kWh</h2>";
  html += "<h2>Temperature: " + String(temperature,2) + " C</h2>";
  html += "<h2>Relay: " + relayStatus + "</h2>";
  html += "</div>";

  html += "</body></html>";

  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();
  client.println(html);
  client.stop();
}