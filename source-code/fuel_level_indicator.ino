
#include <LiquidCrystal.h>

// ========== LCD Pins (RS, E, D4, D5, D6, D7) ==========
LiquidCrystal lcd(2, 15, 19, 18, 5, 4);

#define TRIG_PIN 27
#define ECHO_PIN 26

// ---------------- Tank calibration ----------------
const float TANK_HEIGHT_CM = 30.0;   // Height of tank in cm

// ---------------- Parameters ----------------
const int RAW_SAMPLES = 5;
const unsigned long MEASUREMENT_INTERVAL = 2000; // ms
const float SUDDEN_DROP_THRESHOLD = 10.0;        // % drop considered theft

// ---------------- State ----------------
float prevLevel = 100.0;
unsigned long lastMeasure = 0;

// ---------------- Functions ----------------
float readMedianDistanceCm() {
  float samples[RAW_SAMPLES];
  for (int i = 0; i < RAW_SAMPLES; i++) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    unsigned long dur = pulseIn(ECHO_PIN, HIGH, 30000UL); // timeout 30ms
    float dist = (dur > 0) ? (dur * 0.034f) / 2.0f : TANK_HEIGHT_CM;

    samples[i] = dist;
    delay(50);
  }

  // sort
  for (int i = 0; i < RAW_SAMPLES - 1; i++) {
    for (int j = i + 1; j < RAW_SAMPLES; j++) {
      if (samples[j] < samples[i]) {
        float tmp = samples[i]; samples[i] = samples[j]; samples[j] = tmp;
      }
    }
  }
  return samples[RAW_SAMPLES / 2];
}

float distanceToPercent(float distanceCm) {
  float pct = (1.0 - (distanceCm / TANK_HEIGHT_CM)) * 100.0;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.print("Fuel Monitor...");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  delay(1000);
  lcd.clear();
}

// ---------------- Loop ----------------
void loop() {
  unsigned long now = millis();
  if (now - lastMeasure >= MEASUREMENT_INTERVAL) {
    lastMeasure = now;

    float dist = readMedianDistanceCm();
    float level = distanceToPercent(dist);

    String status;
    if (level < prevLevel)
    {
      float drop = prevLevel - level;
      if (drop >= SUDDEN_DROP_THRESHOLD) 
      {
        status = "Theft/Leakage!";
        gsm("Petrol Theft");   // 🔔 Send SMS when theft detected
      } 
      else
      {
        status = "Gradual Reduce";
      }
    }
    else if (level > prevLevel)
    {
      status = "Fuel Added";
    } 
    else 
    {
      status = "Stable";
    }

    // --- Display on LCD ---
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fuel: ");
    lcd.print(level, 1);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print(status);

    // --- Debug Serial ---
    Serial.print("Raw distance: ");
    Serial.print(dist, 1);
    Serial.print(" cm | Fuel: ");
    Serial.print(level, 1);
    Serial.print("% | Status: ");
    Serial.println(status);

    prevLevel = level;
  }
}

/// --- GSM Function ---
void gsm(String a) 
{
  Serial.println("AT");              // Send AT command
  delay(100);

  Serial.println("AT+CMGF=1");        // Set SMS mode to text
  delay(100);

  Serial.print("AT+CMGS=\"9159883566\"\r"); // your number
  delay(100);

  Serial.print(a);              // SMS content
  delay(100);

  Serial.write(0x1A);                 // End SMS (CTRL+Z)
  delay(100);
}
