#include <OneWire.h>
#include <DallasTemperature.h>

// -------- PIN DEFINITIONS --------
#define CURRENT_PIN 34
#define VOLTAGE_PIN 35
#define TEMP_PIN 4
#define RELAY_PIN 5

// -------- TEMPERATURE SETUP --------
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

// -------- CALIBRATION VALUES --------
float currentCalibration = 30.0;     // For SCT-013-030 (30A max)
float voltageCalibration = 311.0;    // Adjust after calibration

// -------- VARIABLES --------
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

  analogReadResolution(12); // ESP32 ADC 0-4095
}

void loop() {

  // -------- READ CURRENT (RMS Calculation) --------
  float currentSum = 0;
  int samples = 1000;

  for (int i = 0; i < samples; i++) {
    float reading = analogRead(CURRENT_PIN);
    float voltageOut = (reading * 3.3) / 4095.0;
    float centered = voltageOut - 1.65; // midpoint bias
    currentSum += centered * centered;
  }

  float Irms = sqrt(currentSum / samples);
  current = (Irms / 1.0) * currentCalibration; // 1V = 30A

  // -------- READ VOLTAGE --------
  float voltageSum = 0;

  for (int i = 0; i < samples; i++) {
    float reading = analogRead(VOLTAGE_PIN);
    float voltageOut = (reading * 3.3) / 4095.0;
    voltageSum += voltageOut * voltageOut;
  }

  float Vrms = sqrt(voltageSum / samples);
  voltage = Vrms * voltageCalibration;

  // -------- POWER CALCULATION --------
  power = voltage * current;

  // -------- ENERGY (kWh) --------
  unsigned long now = millis();
  float hours = (now - lastTime) / 3600000.0;
  energy += (power * hours) / 1000.0;
  lastTime = now;

  // -------- TEMPERATURE --------
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);

  // -------- RELAY CONTROL (Overload Protection) --------
  if (power > 2000) {   // 2000W limit
    digitalWrite(RELAY_PIN, HIGH);
  } else {
    digitalWrite(RELAY_PIN, LOW);
  }

  // -------- SERIAL OUTPUT --------
  Serial.println("------ ENERGY MONITOR DATA ------");
  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.println(" V");

  Serial.print("Current: ");
  Serial.print(current);
  Serial.println(" A");

  Serial.print("Power: ");
  Serial.print(power);
  Serial.println(" W");

  Serial.print("Energy: ");
  Serial.print(energy);
  Serial.println(" kWh");

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.println("--------------------------------");
  delay(2000);
}