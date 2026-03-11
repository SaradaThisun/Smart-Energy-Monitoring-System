#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

#define OLED_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define SENSOR_PIN 4
OneWire oneWire(SENSOR_PIN);
DallasTemperature sensors(&oneWire);

#define PIN_V 34        // ZMPT101B connected to GPIO34
#define RELAY_PIN 5     // GPIO5 for controlling the relay
#define CURRENT_SENSOR_PIN 32  // ACS712 connected to GPIO32

// ESP32 ADC settings
#define VOLTAGE_REF 3.3
#define ADC_RESOLUTION 4095.0

// ACS712 settings 
#define SENSITIVITY_MV_PER_A 100.0  // For ACS712-20A
const float R1 = 9400.0;      // Voltage divider resistor 1
const float R2 = 4700.0;      // Voltage divider resistor 2
const float DIVIDER_COMPENSATION = (R1 + R2) / R2;
const float NOISE_FLOOR = 0.20;  // Minimum current to display

// ZMPT101B settings
float V_CAL = 0.58;  // You'll need to calibrate this

// Current sensor variables
float offset_voltage = 2.5;
const int FILTER_SIZE = 5;
float current_readings[FILTER_SIZE];
int readIndex = 0;
float total = 0;

// Function to calculate Vrms for voltage (ZMPT101B)
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

// ACS712 Current measurement functions (from your working code)
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

void setup() {
  Serial.begin(115200);
  
  // Configure ESP32 ADC
  analogReadResolution(12);  // 12-bit (0-4095)
  
  Wire.begin(21, 22);  // SDA, SCL for OLED
  sensors.begin();

  pinMode(RELAY_PIN, OUTPUT);  // Set relay pin as output
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED initialization failed!");
    for(;;);  // OLED initialization failed, stop here
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display();
  
  // Initialize current readings array
  for(int i = 0; i < FILTER_SIZE; i++) {
    current_readings[i] = 0;
  }
  
  // Calibrate current sensor offset
  calibrateCurrentOffset();
  
  delay(1000);
}

void loop() {
  // Get temperature from DS18B20
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  // Read voltage from ZMPT101B
  float counts = vrmsCounts(2500, 250);
  float Vrms = counts * V_CAL;

  // Read current from ACS712 (using your working code)
  float current = getCurrent();

  // Print to Serial Monitor
  Serial.print("Temp=");
  Serial.print(tempC, 1);
  Serial.print("C  Vrms=");
  Serial.print(Vrms, 1);
  Serial.print("V  Current=");
  Serial.print(current, 3);
  Serial.println("A");

  // Clear display
  display.clearDisplay();

  // Display Temperature (line 1)
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("T:");
  display.setCursor(18, 0);
  if (tempC == DEVICE_DISCONNECTED_C) display.print("ERR");
  else {
    display.print(tempC, 1);
    display.print("C");
  }

  // Display Voltage (line 2)
  display.setTextSize(1);
  display.setCursor(0, 12);
  display.print("V:");
  display.setCursor(18, 12);
  display.print(Vrms, 1);
  display.print("V");

  // Display Current (line 3)
  display.setTextSize(1);
  display.setCursor(0, 24);
  display.print("I:");
  display.setCursor(18, 24);
  if (current < 0.01) {
    display.print("0.00A");
  } else {
    display.print(current, 2);
    display.print("A");
  }

  // Update display
  display.display();

  // Turn on/off relay based on voltage
  if (Vrms > 100) {  // Relay ON if Vrms > 100V
    digitalWrite(RELAY_PIN, HIGH);
  } else {
    digitalWrite(RELAY_PIN, LOW);   // Relay OFF if Vrms < 100V
  }

  delay(600);  // Update every 600ms
}
