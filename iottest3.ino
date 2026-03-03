#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const char* ssid = "";
const char* password = "";

WiFiServer server(80);

// Pins
#define CURRENT_PIN 34
#define VOLTAGE_PIN 35
#define TEMP_PIN 4
#define RELAY_PIN 5

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

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

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println(WiFi.localIP());
  server.begin();
}

void calculateData() {

  int samples = 200;
  float currentSum = 0;
  float voltageSum = 0;

  for (int i = 0; i < samples; i++) {

    float c = analogRead(CURRENT_PIN);
    float cv = (c * 3.3) / 4095.0;
    currentSum += pow((cv - 1.65), 2);

    float v = analogRead(VOLTAGE_PIN);
    float vv = (v * 3.3) / 4095.0;
    voltageSum += pow(vv, 2);
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

  String request = client.readStringUntil('\r');
  client.flush();

  calculateData();
  float temperature = sensors.getTempCByIndex(0);

  // Relay Control
  if (request.indexOf("/relay?state=on") != -1) {
    digitalWrite(RELAY_PIN, HIGH);
  }
  if (request.indexOf("/relay?state=off") != -1) {
    digitalWrite(RELAY_PIN, LOW);
  }

  // Data API
  if (request.indexOf("/data") != -1) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();

    client.print("{");
    client.print("\"voltage\":"); client.print(voltage,2); client.print(",");
    client.print("\"current\":"); client.print(current,2); client.print(",");
    client.print("\"power\":"); client.print(power,2); client.print(",");
    client.print("\"energy\":"); client.print(energy,3); client.print(",");
    client.print("\"temperature\":"); client.print(temperature,2);
    client.print("}");

    client.stop();
    return;
  }

  // MAIN PAGE
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();

  client.println(R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<style>
body{background:#111;color:white;font-family:Arial;text-align:center;}
button{padding:10px 20px;margin:5px;font-size:16px;}
canvas{max-width:90%;}
</style>
</head>
<body>

<h2>Smart Energy Monitor</h2>

<p>Voltage: <span id="voltage">0</span> V</p>
<p>Current: <span id="current">0</span> A</p>
<p>Power: <span id="power">0</span> W</p>
<p>Energy: <span id="energy">0</span> kWh</p>
<p>Temperature: <span id="temp">0</span> °C</p>

<button onclick="relayOn()">Relay ON</button>
<button onclick="relayOff()">Relay OFF</button>

<canvas id="powerChart"></canvas>

<script>
let ctx = document.getElementById('powerChart').getContext('2d');
let powerData = [];

let chart = new Chart(ctx, {
  type: 'line',
  data: {
    labels: [],
    datasets: [{
      label: 'Power (W)',
      data: powerData,
      borderColor: 'lime',
      borderWidth: 2,
      fill: false
    }]
  },
  options: {
    scales: {
      x: { display:false }
    }
  }
});

function updateData(){
  fetch('/data')
  .then(response => response.json())
  .then(data => {

    document.getElementById("voltage").innerHTML = data.voltage;
    document.getElementById("current").innerHTML = data.current;
    document.getElementById("power").innerHTML = data.power;
    document.getElementById("energy").innerHTML = data.energy;
    document.getElementById("temp").innerHTML = data.temperature;

    if(chart.data.labels.length > 20){
      chart.data.labels.shift();
      chart.data.datasets[0].data.shift();
    }

    chart.data.labels.push('');
    chart.data.datasets[0].data.push(data.power);
    chart.update();
  });
}

function relayOn(){
  fetch('/relay?state=on');
}

function relayOff(){
  fetch('/relay?state=off');
}

setInterval(updateData,1000);
</script>

</body>
</html>
)rawliteral");

  client.stop();
}