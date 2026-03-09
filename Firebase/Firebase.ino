#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// WiFi credentials
#define WIFI_SSID "Galaxy S23 Ultra 7F59"
#define WIFI_PWD "11111111"

// Firebase credentials
#define API_KEY "AIzaSyDDOV-xCVZ7IddShGBGLrU6PikxQLExIVo"
#define DB_URL "https://test1-591e4-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Time tracking for data send interval
unsigned long sendDataPrevMillis = 0;

// Replace these with your sensor readings (example: read from analog pin or sensor library)
float current = 0.0;
float voltage = 0.0;
float power = 0.0;

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PWD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  
  // Initialize Firebase
  config.api_key = API_KEY;
  auth.user.email = "vinurasandaruwan@gmail.com";
  auth.user.password = "2003";
  config.database_url = DB_URL;

  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);
}

void loop() {
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    // Collect your sensor data (example)
    current = analogRead(A0) * (5.0 / 1023.0); // Replace with actual current sensor reading
    voltage = analogRead(34) * (5.0 / 1023.0); // Replace with actual voltage sensor reading
    power = current * voltage;

    // Update Firebase with new data
    if (Firebase.RTDB.setFloat(&fbdo, "/sensor/current", current) && 
        Firebase.RTDB.setFloat(&fbdo, "/sensor/voltage", voltage) && 
        Firebase.RTDB.setFloat(&fbdo, "/sensor/power", power)) {
      Serial.println("Data successfully sent to Firebase");
    } else {
      Serial.print("Error sending data: ");
      Serial.println(fbdo.errorReason());
    }
  }
}