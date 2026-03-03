#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// OLED Configuration
#define OLED_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// DS18B20 Configuration
#define SENSOR_PIN 4 // Connect the DS18B20 Data pin to GPIO 4
OneWire oneWire(SENSOR_PIN);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA, SCL
  sensors.begin();    // Start the sensor
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED init failed");
    for(;;);
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
  // Request temperature from sensor
  sensors.requestTemperatures(); 
  float tempC = sensors.getTempCByIndex(0);

  display.clearDisplay();

  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("TEMPERATURE:");

  // Temperature Value
  display.setTextSize(2);
  display.setCursor(0, 15);
  
  if(tempC == DEVICE_DISCONNECTED_C) {
    display.print("ERROR");
  } else {
    display.print(tempC);
    display.print(" C");
  }

  display.display();
  delay(1000); // Update every second
}
