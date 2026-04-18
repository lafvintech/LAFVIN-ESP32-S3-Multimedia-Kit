#include <Wire.h>
#include <string.h>
#include "MAX30105.h"
#include "heartRate.h"

#define IIC_SDA 2
#define IIC_SCL 1

#define FINGER_ON_THRESHOLD 12000
#define FINGER_OFF_THRESHOLD 6000
#define FINGER_DEBOUNCE_SAMPLES 8
#define REPORT_INTERVAL_MS 1000
#define RATE_AVG_SIZE 8
#define LED_BRIGHTNESS_INITIAL 20
#define SAMPLE_AVERAGE 4
#define SAMPLE_RATE 100
#define PULSE_WIDTH 411
#define ADC_RANGE 16384

MAX30105 particleSensor;

float beatsPerMinute = 0.0f;
int beatAvg = 0;
int rateHistory[RATE_AVG_SIZE] = {0};
byte rateSpot = 0;
byte rateCount = 0;

long lastBeatMs = 0;
unsigned long lastReportMs = 0;
bool fingerPresent = false;
byte fingerOnCount = 0;
byte fingerOffCount = 0;
uint32_t lastIrValue = 0;

void resetHeartRateState() {
  beatsPerMinute = 0.0f;
  beatAvg = 0;
  rateSpot = 0;
  rateCount = 0;
  lastBeatMs = 0;
  memset(rateHistory, 0, sizeof(rateHistory));
}

void updateAverageRate(int bpm) {
  rateHistory[rateSpot++] = bpm;
  rateSpot %= RATE_AVG_SIZE;

  if (rateCount < RATE_AVG_SIZE) {
    rateCount++;
  }

  int total = 0;
  for (byte i = 0; i < rateCount; i++) {
    total += rateHistory[i];
  }
  beatAvg = total / rateCount;
}

float bpmDiff(float a, float b) {
  return (a > b) ? (a - b) : (b - a);
}

bool isReasonableBeat(float bpm) {
  if (bpm < 45.0f || bpm > 180.0f) {
    return false;
  }

  if (rateCount >= 3 && bpmDiff(bpm, (float)beatAvg) > 25.0f) {
    return false;
  }

  return true;
}

void handleFingerDetected() {
  fingerPresent = true;
  fingerOnCount = 0;
  fingerOffCount = 0;
  particleSensor.clearFIFO();
  resetHeartRateState();
  Serial.println("Finger detected. Measuring heart rate...");
}

void handleFingerRemoved() {
  fingerPresent = false;
  fingerOnCount = 0;
  fingerOffCount = 0;
  particleSensor.clearFIFO();
  resetHeartRateState();
  Serial.println("Finger removed. Waiting for a stable signal...");
}

void processHeartRate(uint32_t irValue) {
  if (!checkForBeat(irValue)) {
    return;
  }

  const long nowMs = millis();
  if (lastBeatMs == 0) {
    lastBeatMs = nowMs;
    return;
  }

  const float bpm = 60.0f / ((nowMs - lastBeatMs) / 1000.0f);
  lastBeatMs = nowMs;

  if (!isReasonableBeat(bpm)) {
    return;
  }

  beatsPerMinute = bpm;
  updateAverageRate((int)(bpm + 0.5f));
}

void printStatus() {
  if (!fingerPresent) {
    Serial.println("status=No finger");
    return;
  }

  if (lastIrValue < FINGER_ON_THRESHOLD) {
    Serial.println("status=Signal too weak");
    return;
  }

  if (rateCount == 0) {
    Serial.println("status=Detecting pulse...");
    return;
  }

  Serial.print("BPM=");
  Serial.print(beatsPerMinute, 1);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);
  Serial.print(", IR(raw)=");
  Serial.println(lastIrValue);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing MAX30102...");

  Wire.begin(IIC_SDA, IIC_SCL);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102/MAX30105 was not found. Please check wiring and power.");
    while (1) {
      delay(10);
    }
  }

  const byte ledBrightness = LED_BRIGHTNESS_INITIAL;
  const byte sampleAverage = SAMPLE_AVERAGE;
  const byte ledMode = 2;
  const int sampleRate = SAMPLE_RATE;
  const int pulseWidth = PULSE_WIDTH;
  const int adcRange = ADC_RANGE;

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  particleSensor.setPulseAmplitudeGreen(0);

  resetHeartRateState();

  Serial.println("MAX30102 ready.");
  Serial.println("Place your finger gently on the sensor and keep still.");
}

void loop() {
  particleSensor.check();

  while (particleSensor.available()) {
    lastIrValue = particleSensor.getIR();
    particleSensor.nextSample();

    if (lastIrValue >= FINGER_ON_THRESHOLD) {
      if (fingerOnCount < FINGER_DEBOUNCE_SAMPLES) {
        fingerOnCount++;
      }
      fingerOffCount = 0;
      if (!fingerPresent && fingerOnCount >= FINGER_DEBOUNCE_SAMPLES) {
        handleFingerDetected();
      }
    } else if (lastIrValue <= FINGER_OFF_THRESHOLD) {
      if (fingerOffCount < FINGER_DEBOUNCE_SAMPLES) {
        fingerOffCount++;
      }
      fingerOnCount = 0;
      if (fingerPresent && fingerOffCount >= FINGER_DEBOUNCE_SAMPLES) {
        handleFingerRemoved();
      }
    } else {
      fingerOnCount = 0;
      fingerOffCount = 0;
    }

    if (fingerPresent) {
      processHeartRate(lastIrValue);
    }

    if (millis() - lastReportMs >= REPORT_INTERVAL_MS) {
      lastReportMs = millis();
      printStatus();
    }
  }
}
