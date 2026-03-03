#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

// ---------------- OLED ----------------
#define OLED_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------------- DS18B20 ----------------
#define SENSOR_PIN 4
OneWire oneWire(SENSOR_PIN);
DallasTemperature sensors(&oneWire);

// ---------------- SCT-013 ----------------
#define CURRENT_PIN 34   // ADC1 safe pin
#define ADC_MAX 4095.0
#define VREF 3.3

// Calibration 
float I_CAL = 15.0;   // Adjust later

// Sampling settings
#define SAMPLE_COUNT 1000
#define SAMPLE_DELAY_US 200

// ---------------- Function Prototype ----------------
float measureCurrentRMS();

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  sensors.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED init failed");
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(20, 10);
  display.print("Initializing...");
  display.display();
  delay(1000);
}

void loop() {

  // ---- Temperature ----
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  // ---- Current ----
  float Irms = measureCurrentRMS();

  Serial.print("Temp: ");
  Serial.print(tempC);
  Serial.print(" C  |  Current: ");
  Serial.print(Irms, 2);
  Serial.println(" A");

  // ---- OLED Display ----
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Temp: ");
  
  if (tempC == DEVICE_DISCONNECTED_C) {
    display.print("ERROR");
  } else {
    display.print(tempC, 1);
    display.print(" C");
  }

  display.setCursor(0, 16);
  display.print("Current: ");
  display.print(Irms, 2);
  display.print(" A");

  display.display();

  delay(1000);
}


// Measure AC RMS Current from biased waveform

float measureCurrentRMS() {

  double sum = 0;
  double sumSq = 0;

  // First pass: get DC offset
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    int raw = analogRead(CURRENT_PIN);
    sum += raw;
    delayMicroseconds(SAMPLE_DELAY_US);
  }

  double mean = sum / SAMPLE_COUNT;

  // Second pass: RMS calculation
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    int raw = analogRead(CURRENT_PIN);
    double centered = raw - mean;
    sumSq += centered * centered;
    delayMicroseconds(SAMPLE_DELAY_US);
  }

  double rmsCounts = sqrt(sumSq / SAMPLE_COUNT);

  // Convert ADC counts to volts
  double rmsVolts = (rmsCounts / ADC_MAX) * VREF;

  // Apply calibration factor
  float Irms = rmsVolts * I_CAL;

  return Irms;
}