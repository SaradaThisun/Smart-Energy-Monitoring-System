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

#define DS18_PIN 4
OneWire oneWire(DS18_PIN);
DallasTemperature sensors(&oneWire);

const int PIN_V = 34;
float V_CAL = 0.58;

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

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  sensors.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    for(;;);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display();
}

void loop() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  float counts = vrmsCounts(2500, 250);
  float Vrms = counts * V_CAL;

  Serial.print("Temp=");
  Serial.print(tempC, 1);
  Serial.print("C  Vrms=");
  Serial.println(Vrms, 1);

  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("T:");

  display.setTextSize(2);
  display.setCursor(18, 0);
  if (tempC == DEVICE_DISCONNECTED_C) display.print("ERR");
  else {
    display.print(tempC, 1);
    display.print("C");
  }

  display.setTextSize(1);
  display.setCursor(0, 18);
  display.print("V:");

  display.setTextSize(2);
  display.setCursor(18, 16);
  display.print(Vrms, 0);
  display.print("V");

  display.display();
  delay(600);
}
