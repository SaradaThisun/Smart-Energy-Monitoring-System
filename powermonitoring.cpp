const int CURRENT_PIN = A0;
const float ADC_REF = 5.0;
const int ADC_MAX = 1023;

// SCT-013-030 = 30A : 1V RMS
const float CURRENT_PER_VOLT = 30.0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  const int samples = 1000;

  long sumRaw = 0;
  float sumSq = 0;

  // Step 1: find actual DC offset
  for (int i = 0; i < samples; i++) {
    int raw = analogRead(CURRENT_PIN);
    sumRaw += raw;
    delayMicroseconds(200);
  }

  float offset = (float)sumRaw / samples;

  // Step 2: calculate AC RMS around that offset
  for (int i = 0; i < samples; i++) {
    int raw = analogRead(CURRENT_PIN);
    float centered = raw - offset;
    sumSq += centered * centered;
    delayMicroseconds(200);
  }

  float rmsCounts = sqrt(sumSq / samples);

  // Convert counts to volts RMS
  float vrms = (rmsCounts * ADC_REF) / ADC_MAX;

  // Convert volts RMS to current
  float current = vrms * CURRENT_PER_VOLT;

  // Noise floor: ignore tiny readings
  if (current < 0.15) {
    current = 0.0;
  }

  Serial.print("Offset: ");
  Serial.print(offset, 2);
  Serial.print("   Vrms: ");
  Serial.print(vrms, 4);
  Serial.print(" V   Current: ");
  Serial.print(current, 3);
  Serial.println(" A");

  delay(1000);
}
