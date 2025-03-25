/*
 * Heart pulse sensor with Arduino + OLED (Smoothed BPM)
 * Author: Kamal
 * Date: 2025-03-25
 */

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

#define SENSOR_PIN A0
#define HIGH_PULSE_THRESHOLD 510  // Adjust based on sensor behavior

Adafruit_SSD1306 heartBeat(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// ECG graphing
int prevX = 0, prevY = 60;
int x = 0;

// Pulse data
int sensorValue;
int pulseValue;
long lastPulseTime = 0;
long timeBetweenPulses = 0;
int BPM = 0;

// BPM smoothing
#define MAX_SAMPLES 10
int bpmSamples[MAX_SAMPLES] = { 0 };
int sampleIndex = 0;

// Signal smoothing
#define SMOOTH_SAMPLES 5
int smoothBuffer[SMOOTH_SAMPLES];
int smoothIndex = 0;

// Heartbeat icon
bool heartVisible = false;
unsigned long lastHeartToggle = 0;

void setup() {
  Serial.begin(9600);

  if (!heartBeat.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true)
      ;
  }

  delay(1000);
  heartBeat.clearDisplay();
  heartBeat.display();
}

void loop() {
  int rawValue = analogRead(SENSOR_PIN);
  sensorValue = getSmoothedValue(rawValue);
  Serial.print("Sensor: ");
  Serial.print(sensorValue);
  Serial.print("  BPM: ");
  Serial.println(BPM);

  pulseValue = map(sensorValue, 0, 1023, 0, 45);
  int y = 60 - pulseValue;

  // ECG-style waveform
  if (x > SCREEN_WIDTH) {
    x = 0;
    prevX = 0;
    heartBeat.clearDisplay();
  }
  heartBeat.drawLine(prevX, prevY, x, y, WHITE);
  prevX = x;
  prevY = y;
  x++;

  calculateBPM();

  // Toggle heart icon
  if (millis() - lastHeartToggle > 500) {
    heartVisible = !heartVisible;
    lastHeartToggle = millis();
  }

  // Draw BPM + heart
  heartBeat.fillRect(0, 0, SCREEN_WIDTH, 16, BLACK);  // Clear top
  heartBeat.setTextSize(2);
  heartBeat.setTextColor(SSD1306_WHITE);
  heartBeat.setCursor(0, 0);
  heartBeat.print("BPM:");
  heartBeat.setCursor(80, 0);
  heartBeat.print(BPM);

  // Display "MPTC" label
  heartBeat.setTextSize(1);
  heartBeat.setCursor(SCREEN_WIDTH - 40, SCREEN_HEIGHT - 10);  // Bottom-right corner
  heartBeat.print("MPTC");

  if (heartVisible) {
    drawHeartIcon(110, 0);
  }

  heartBeat.display();
}

void calculateBPM() {
  if (sensorValue > HIGH_PULSE_THRESHOLD && millis() - lastPulseTime > 300) {
    timeBetweenPulses = millis() - lastPulseTime;
    lastPulseTime = millis();
    int currentBPM = 60000 / timeBetweenPulses;

    bpmSamples[sampleIndex] = currentBPM;
    sampleIndex = (sampleIndex + 1) % MAX_SAMPLES;

    long total = 0;
    for (int i = 0; i < MAX_SAMPLES; i++) {
      total += bpmSamples[i];
    }
    BPM = total / MAX_SAMPLES;
  }
}

int getSmoothedValue(int rawValue) {
  smoothBuffer[smoothIndex] = rawValue;
  smoothIndex = (smoothIndex + 1) % SMOOTH_SAMPLES;

  int sum = 0;
  for (int i = 0; i < SMOOTH_SAMPLES; i++) {
    sum += smoothBuffer[i];
  }
  return sum / SMOOTH_SAMPLES;
}

void drawHeartIcon(int x, int y) {
  heartBeat.fillCircle(x, y + 2, 2, WHITE);
  heartBeat.fillCircle(x + 4, y + 2, 2, WHITE);
  heartBeat.fillTriangle(x - 1, y + 3, x + 5, y + 3, x + 2, y + 8, WHITE);
}
