#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// WiFi credentials
#define WIFI_SSID "Galaxy S23 Ultra 7F59"
#define WIFI_PWD "11111111"

// Firebase credentials
#define API_KEY "AIzaSyDBFy0g9TAQawRNsO6F9IRhnkP5TZQQLfU"
#define DB_URL "https://test1-230d1-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Time tracking for Firebase updates
unsigned long sendDataPrevMillis = 0;

// Hardware pin for LED
const int ledPin = 2;  // Change this to your pin

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);  // Initially turn off the LED

  // Start Serial Monitor for debugging
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PWD);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\nConnected with IP: ");
  Serial.println(WiFi.localIP());

  // Firebase config
  config.api_key = API_KEY;
  auth.user.email = "vinurasandaruwan@gmail.com";  // Replace with your Firebase email
  auth.user.password = "20030628";       // Replace with your Firebase password
  config.database_url = DB_URL;

  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);
  fbdo.setResponseSize(2048);
  config.timeout.serverResponse = 10 * 1000;  // Set timeout to 10 seconds
}

void loop() {
  // Firebase update interval (1 second)
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    // Reading current LED state
    int ledState = digitalRead(ledPin);  // Read current LED state

    // Send the LED state to Firebase
    if (Firebase.RTDB.setInt(&fbdo, "/led/state", ledState)) {
      Serial.print("LED state updated: ");
      Serial.println(ledState == HIGH ? "ON" : "OFF");
    } else {
      Serial.print("Failed to update LED state: ");
      Serial.println(fbdo.errorReason());
    }
  }
}
