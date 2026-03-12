#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>


#include <addons/TokenHelper.h>


#define WIFI_SSID "Galaxy S23 Ultra 7F59"
#define WIFI_PASSWORD "11111111"


#define API_KEY "AIzaSyAXRuermsU80P5qGt8bVIv7Jg-e8FQ0KxY"
#define DATABASE_URL "https://project-c6ce3-default-rtdb.asia-southeast1.firebasedatabase.app"

// OLED Settings
#define OLED_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Sensor Pins
#define SENSOR_PIN 4
OneWire oneWire(SENSOR_PIN);
DallasTemperature sensors(&oneWire);

#define PIN_V 34        
#define RELAY_PIN 5     
#define CURRENT_SENSOR_PIN 32  
#define BUZZER_PIN 19   

// ESP32 ADC settings
#define VOLTAGE_REF 3.3
#define ADC_RESOLUTION 4095.0

// ACS712 settings
#define SENSITIVITY_MV_PER_A 100.0
const float R1 = 9400.0;
const float R2 = 4700.0;
const float DIVIDER_COMPENSATION = (R1 + R2) / R2;
const float NOISE_FLOOR = 0.20;

// Temperature Safety Settings
#define TEMP_WARNING_LEVEL 45.0f   
#define TEMP_CRITICAL_LEVEL 55.0f  
#define TEMP_RECOVERY_LEVEL 40.0f  

// ZMPT101B settings
float V_CAL = 0.58;

// Current sensor variables
float offset_voltage = 2.5;
const int FILTER_SIZE = 5;
float current_readings[FILTER_SIZE];
int readIndex = 0;
float total = 0;

// Safety state variables
bool emergencyShutdown = false;
unsigned long lastBeepTime = 0;
int beepPattern = 0;

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Timing variables for Firebase updates
unsigned long sendDataPrevMillis = 0;
const long FIREBASE_INTERVAL = 5000; // Send data every 5 seconds
bool firebaseReady = false;

// Device ID 
String deviceID = "esp32_001";

// Function to calculate Vrms for voltage 
float vrmsCounts(int samples, int delayUs) {
  long sum = 0;
  for (int i = 0; i < 200; i++) {
    sum += analogRead(PIN_V);
    delayMicroseconds(120);
  }
  float mid = sum / 200.0f;

  double sq = 0;
  for (int i = 0; i < samples; i++) {
    float x = analogRead(PIN_V) - mid;
    sq += (double)x * (double)x;
    delayMicroseconds(delayUs);
  }
  return sqrt(sq / samples);
}

// ACS712 Current measurement functions
void calibrateCurrentOffset() {
  const int CAL_SAMPLES = 1000;
  float sum = 0;
  
  for(int i = 0; i < CAL_SAMPLES; i++) {
    int adc = analogRead(CURRENT_SENSOR_PIN);
    float voltage_at_esp = (adc / ADC_RESOLUTION) * VOLTAGE_REF;
    float sensor_voltage = voltage_at_esp * DIVIDER_COMPENSATION;
    sum += sensor_voltage;
    delay(1);
  }
  
  offset_voltage = sum / CAL_SAMPLES;
  
  Serial.print("Offset voltage calibrated to: ");
  Serial.print(offset_voltage, 3);
  Serial.println("V");
}

float measureAC_rms() {
  const int SAMPLES = 200;
  float sum_squared = 0;
  
  for(int i = 0; i < SAMPLES; i++) {
    int adc = analogRead(CURRENT_SENSOR_PIN);
    float voltage_at_esp = (adc / ADC_RESOLUTION) * VOLTAGE_REF;
    float sensor_voltage = voltage_at_esp * DIVIDER_COMPENSATION;
    
    float ac_voltage = sensor_voltage - offset_voltage;
    float current = ac_voltage / (SENSITIVITY_MV_PER_A / 1000.0);
    
    sum_squared += current * current;
    delayMicroseconds(400);
  }
  
  float mean_squared = sum_squared / SAMPLES;
  float rms = sqrt(mean_squared);
  
  if(rms < NOISE_FLOOR) return 0.0;
  return rms;
}

float movingAverageFilter(float newValue) {
  total = total - current_readings[readIndex];
  current_readings[readIndex] = newValue;
  total = total + current_readings[readIndex];
  readIndex = (readIndex + 1) % FILTER_SIZE;
  return total / FILTER_SIZE;
}

float getCurrent() {
  float rms = measureAC_rms();
  return movingAverageFilter(rms);
}

// Handle buzzer based on temperature state
void handleBuzzer(float temp) {
  static unsigned long lastToggle = 0;
  
  if (temp >= TEMP_CRITICAL_LEVEL || temp == DEVICE_DISCONNECTED_C) {
    if (millis() - lastToggle > 200) {
      lastToggle = millis();
      digitalWrite(BUZZER_PIN, !digitalRead(BUZZER_PIN));
    }
  }
  else if (temp >= TEMP_WARNING_LEVEL) {
    if (millis() % 2000 < 1000) {
      digitalWrite(BUZZER_PIN, HIGH);
    } else {
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
  else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// Function to connect to WiFi
void setupWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Connecting to WiFi");
  display.display();
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    Serial.print(".");
    delay(500);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("WiFi Connected");
    display.setCursor(0, 12);
    display.print(WiFi.localIP());
    display.display();
    delay(2000);
  } else {
    Serial.println();
    Serial.println("WiFi Connection Failed!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("WiFi Failed!");
    display.display();
  }
}

// Function to setup Firebase
// Function to setup Firebase with Email/Password
void setupFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  
  auth.user.email = "vinurasandaruwan@gmail.com";   
  auth.user.password = "20030628";      
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  // Check if signed in
  if (Firebase.ready()) {
    Serial.println("Firebase connected successfully");
    firebaseReady = true;
  } else {
    Serial.println("Firebase connection failed");
    firebaseReady = false;
  }
  
  fbdo.setResponseSize(1024);
}

// Function to send data to Firebase with organized folder structure
void sendToFirebase(float temp, float voltage, float current, String status) {
  if (!firebaseReady) {
    Serial.println("Firebase not ready");
    return;
  }
  
  // Get current timestamp
  unsigned long currentTime = millis();
  
  // Create organized data structure
  
  // 1. Send to realtime_data folder (history of all readings)
  FirebaseJson historyJson;
  historyJson.add("temperature", temp);
  historyJson.add("voltage", voltage);
  historyJson.add("current", current);
  historyJson.add("status", status);
  historyJson.add("relay", digitalRead(RELAY_PIN));
  historyJson.add("emergency", emergencyShutdown);
  historyJson.add("timestamp", String(currentTime));
  historyJson.add("device_id", deviceID);
  
  // Push to realtime_data folder 
  String historyPath = "/realtime_data/" + String(currentTime);
  if (Firebase.RTDB.setJSON(&fbdo, historyPath.c_str(), &historyJson)) {
    Serial.println("History data saved");
  } else {
    Serial.println("Failed to save history: " + fbdo.errorReason());
  }
  
  // 2. Update current readings 
  FirebaseJson currentJson;
  currentJson.add("temperature", temp);
  currentJson.add("voltage", voltage);
  currentJson.add("current", current);
  currentJson.add("status", status);
  currentJson.add("relay", digitalRead(RELAY_PIN));
  currentJson.add("emergency", emergencyShutdown);
  currentJson.add("last_update", String(currentTime));
  
  if (Firebase.RTDB.setJSON(&fbdo, "/current/readings", &currentJson)) {
    Serial.println("Current readings updated");
  } else {
    Serial.println("Failed to update current: " + fbdo.errorReason());
  }
  
  // 3. Update device status
  FirebaseJson deviceJson;
  deviceJson.add("online", true);
  deviceJson.add("last_seen", String(currentTime));
  deviceJson.add("ip_address", WiFi.localIP().toString());
  deviceJson.add("wifi_strength", WiFi.RSSI());
  
  String devicePath = "/devices/" + deviceID;
  if (Firebase.RTDB.setJSON(&fbdo, devicePath.c_str(), &deviceJson)) {
    Serial.println("Device status updated");
  }
  
  // 4. Update statistics
  FirebaseJson statsJson;
  statsJson.add("avg_temperature", temp); // You can calculate real averages later
  statsJson.add("max_temperature", TEMP_CRITICAL_LEVEL);
  statsJson.add("system_status", status);
  statsJson.add("uptime_seconds", currentTime / 1000);
  
  if (Firebase.RTDB.setJSON(&fbdo, "/system/statistics", &statsJson)) {
    Serial.println("Statistics updated");
  }
  
  Serial.println("All data sent to Firebase successfully!");
}

// Get status string based on temperature
String getStatusString(float temp) {
  if (temp == DEVICE_DISCONNECTED_C) {
    return "SENSOR_ERROR";
  } else if (temp >= TEMP_CRITICAL_LEVEL) {
    return "CRITICAL";
  } else if (temp >= TEMP_WARNING_LEVEL) {
    return "WARNING";
  } else {
    return "NORMAL";
  }
}

void setup() {
  Serial.begin(115200);
  
  // Configure ESP32 ADC
  analogReadResolution(12);
  
  Wire.begin(21, 22);
  sensors.begin();

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initially relay ON (load connected)
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED initialization failed!");
    for(;;);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display();
  
  // Initialize current readings array
  for(int i = 0; i < FILTER_SIZE; i++) {
    current_readings[i] = 0;
  }
  
  // Connect to WiFi
  setupWiFi();
  
  // Setup Firebase
  setupFirebase();
  
  // Calibrate current sensor offset
  calibrateCurrentOffset();
  
  // Send initial device info to Firebase
  if (firebaseReady) {
    FirebaseJson deviceInfo;
    deviceInfo.add("device_id", deviceID);
    deviceInfo.add("firmware_version", "1.0");
    deviceInfo.add("setup_time", String(millis()));
    Firebase.RTDB.setJSON(&fbdo, "/devices/" + deviceID + "/info", &deviceInfo);
  }
  
  // Startup beep
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
  
  Serial.println("System Started - Temperature Safety Enabled");
  Serial.println("Firebase folder structure created:");
  Serial.println("  ├── /realtime_data/[timestamp] - Historical readings");
  Serial.println("  ├── /current/readings - Latest readings");
  Serial.println("  ├── /devices/esp32_001 - Device status");
  Serial.println("  └── /system/statistics - System stats");
  
  delay(1000);
}

void loop() {
  // Get temperature from DS18B20
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  // Read voltage from ZMPT101B
  float counts = vrmsCounts(2500, 250);
  float Vrms = counts * V_CAL;

  // Read current from ACS712
  float current = getCurrent();

  // Get status string
  String status = getStatusString(tempC);

  // TEMPERATURE SAFETY LOGIC
  if (tempC >= TEMP_CRITICAL_LEVEL || tempC == DEVICE_DISCONNECTED_C) {
    digitalWrite(RELAY_PIN, LOW);
    emergencyShutdown = true;
    Serial.println("!!! CRITICAL TEMPERATURE - Relay OFF !!!");
  }
  else if (tempC >= TEMP_WARNING_LEVEL) {
    if (!emergencyShutdown) {
      digitalWrite(RELAY_PIN, HIGH);
    }
    Serial.println("WARNING: High Temperature");
  }
  else {
    if (emergencyShutdown && tempC <= TEMP_RECOVERY_LEVEL) {
      emergencyShutdown = false;
      Serial.println("System recovered - Temperature normal");
      
      for(int i = 0; i < 2; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(BUZZER_PIN, LOW);
        delay(200);
      }
    }
    
    if (!emergencyShutdown) {
      if (Vrms > 200) {
        digitalWrite(RELAY_PIN, HIGH);
      } else {
        digitalWrite(RELAY_PIN, LOW);
      }
    }
  }

  // Handle buzzer based on temperature
  handleBuzzer(tempC);

  // Send data to Firebase at specified interval
  if (millis() - sendDataPrevMillis > FIREBASE_INTERVAL && Firebase.ready()) {
    sendDataPrevMillis = millis();
    sendToFirebase(tempC, Vrms, current, status);
  }

  // Print to Serial Monitor
  Serial.print("Temp=");
  Serial.print(tempC, 1);
  Serial.print("C [");
  Serial.print(status);
  Serial.print("]  Vrms=");
  Serial.print(Vrms, 1);
  Serial.print("V  I=");
  Serial.print(current, 2);
  Serial.println("A");

  // Display on OLED
  display.clearDisplay();

  // Line 1: Temperature with status
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("T:");
  display.setCursor(18, 0);
  
  if (tempC == DEVICE_DISCONNECTED_C) {
    display.print("ERR");
  } else {
    display.print(tempC, 1);
    display.print("C");
    
    if (tempC >= TEMP_WARNING_LEVEL) {
      display.setCursor(70, 0);
      display.print("!");
      if (tempC >= TEMP_CRITICAL_LEVEL) {
        display.print("!");
      }
    }
  }

  // Voltage
  display.setCursor(0, 12);
  display.print("V:");
  display.setCursor(18, 12);
  display.print(Vrms, 1);
  display.print("V");
  
  // Add Firebase status
  if (firebaseReady) {
    display.setCursor(100, 12);
    display.print("F");
  }

  // Line 3: Current
  display.setCursor(0, 24);
  display.print("I:");
  display.setCursor(18, 24);
  if (current < 0.01) {
    display.print("0.00A");
  } else {
    display.print(current, 2);
    display.print("A");
  }

  display.display();
  delay(600);
}
