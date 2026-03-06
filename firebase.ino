#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// WiFi Credentials
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Firebase Credentials
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "YOUR_DATABASE_URL"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Sensor Pins
#define CURRENT_SENSOR 34
#define VOLTAGE_SENSOR 35
#define ONE_WIRE_BUS 4
#define RELAY_PIN 5

// Temperature sensor setup
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Variables
float voltage = 0;
float current = 0;
float power = 0;
float temperature = 0;

void setup() {

  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  sensors.begin();

  // Connect WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("Connected");

  // Firebase configuration
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {

  // Read Current Sensor
  int currentRaw = analogRead(CURRENT_SENSOR);
  current = (currentRaw / 4095.0) * 30.0;

  // Read Voltage Sensor
  int voltageRaw = analogRead(VOLTAGE_SENSOR);
  voltage = (voltageRaw / 4095.0) * 250.0;

  // Calculate Power
  power = voltage * current;

  // Read Temperature
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);

  // Send data to Firebase
  Firebase.RTDB.setFloat(&fbdo, "/sensorData/current", current);
  Firebase.RTDB.setFloat(&fbdo, "/sensorData/voltage", voltage);
  Firebase.RTDB.setFloat(&fbdo, "/sensorData/power", power);
  Firebase.RTDB.setFloat(&fbdo, "/sensorData/temperature", temperature);

  // Read Relay Status from Firebase
  if (Firebase.RTDB.getInt(&fbdo, "/relay/status")) {

    int relayState = fbdo.intData();

    if (relayState == 1) {
      digitalWrite(RELAY_PIN, HIGH);
    }
    else {
      digitalWrite(RELAY_PIN, LOW);
    }
  }

  // Serial Monitor Output
  Serial.println("Voltage: " + String(voltage));
  Serial.println("Current: " + String(current));
  Serial.println("Power: " + String(power));
  Serial.println("Temperature: " + String(temperature));
  Serial.println("------------------------");

  delay(1000);
}