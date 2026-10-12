
#include <Wire.h>
#include <ESP32Servo.h>
#include "MS5837.h"

// ================================================================
// ESP32: 8-Thruster Controller + BAR30 (MS5837) Telemetry
//
// Incoming USB Serial:
//   <pwm0,pwm1,pwm2,pwm3,pwm4,pwm5,pwm6,pwm7>\n
//
// Outgoing USB Serial:
//   <pressure,temp,depth>\n
//
// Pressure: mbar (millibar)
// Temperature: degrees Celsius
// Depth: metres
// ================================================================

// ---------------- ESC configuration ----------------

const int ESC_PINS[8] = {
  13, 12, 14, 27, 26, 25, 33, 32
};

// T1-T4: FL, FR, BL, BR
// T5-T8: HFL, HFR, HBL, HBR

const int PWM_MIN     = 1100;
const int PWM_MAX     = 1900;
const int PWM_NEUTRAL = 1500;

const unsigned long FAILSAFE_MS = 500;

Servo escs[8];

unsigned long lastPacketTime = 0;
bool failsafeActive = true;

// ---------------- BAR30 / I2C configuration ----------------

// Default ESP32 DevKit I2C pins.
// Change these if your board uses different pins.
const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;

MS5837 bar30;

bool bar30Ready = false;

const unsigned long SENSOR_INTERVAL_MS = 200; // 5 Hz
unsigned long lastSensorTime = 0;

// Use 1029 for seawater or 997 for freshwater.
const float FLUID_DENSITY = 1029.0f;

// ---------------- Serial packet buffer ----------------

char rxBuffer[96];
size_t rxLength = 0;
bool rxOverflow = false;

// ================================================================
// PWM helpers
// ================================================================

int clampPWM(int value) {
  return constrain(value, PWM_MIN, PWM_MAX);
}

void sendNeutral() {
  for (int i = 0; i < 8; i++) {
    escs[i].writeMicroseconds(PWM_NEUTRAL);
  }
}

void applyPWM(const int pwm[8]) {
  for (int i = 0; i < 8; i++) {
    escs[i].writeMicroseconds(clampPWM(pwm[i]));
  }
}

// ================================================================
// Strict PWM packet parser
// Accepts exactly eight integer values between 1100 and 1900.
// ================================================================

bool parsePacket(const char* input, int pwm[8]) {
  size_t len = strlen(input);

  if (len < 3 || input[0] != '<' || input[len - 1] != '>') {
    return false;
  }

  size_t pos = 1;
  int count = 0;

  while (pos < len - 1) {
    if (count >= 8) return false;

    // Only accept decimal digits for PWM values.
    if (input[pos] < '0' || input[pos] > '9') {
      return false;
    }

    long value = 0;

    while (pos < len - 1 &&
           input[pos] >= '0' && input[pos] <= '9') {
      value = value * 10 + (input[pos] - '0');

      if (value > PWM_MAX) return false;

      pos++;
    }

    if (value < PWM_MIN || value > PWM_MAX) {
      return false;
    }

    pwm[count++] = (int)value;

    if (pos == len - 1) break;

    if (input[pos] != ',') return false;

    pos++;

    // Reject a trailing comma.
    if (pos == len - 1) return false;
  }

  return count == 8 && pos == len - 1;
}

// ================================================================
// Incoming serial processing
// Non-blocking: does not use readStringUntil().
// ================================================================

void processSerial() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      if (!rxOverflow && rxLength > 0) {
        rxBuffer[rxLength] = '\0';

        int pwm[8];

        if (parsePacket(rxBuffer, pwm)) {
          applyPWM(pwm);

          lastPacketTime = millis();
          failsafeActive = false;
        }
      }

      rxLength = 0;
      rxOverflow = false;
      continue;
    }

    if (!rxOverflow) {
      if (rxLength < sizeof(rxBuffer) - 1) {
        rxBuffer[rxLength++] = c;
      } else {
        rxOverflow = true;
      }
    }
  }
}

// ================================================================
// BAR30 initialization
// ================================================================

void initializeBAR30() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);  // Standard I2C: 100 kHz

  bar30.setModel(MS5837::MS5837_30BA);

  if (bar30.init(Wire)) {
    bar30.setFluidDensity(FLUID_DENSITY);
    bar30Ready = true;
  } else {
    // Keep the ESC controller operational even if the
    // BAR30 is disconnected or fails initialization.
    bar30Ready = false;
  }
}

// ================================================================
// BAR30 telemetry
// Format: <pressure,temp,depth>\n
// ================================================================

void publishBAR30() {
  if (!bar30Ready) {
    return;
  }

  unsigned long now = millis();

  if (now - lastSensorTime < SENSOR_INTERVAL_MS) {
    return;
  }

  lastSensorTime = now;

  bar30.read();

  float pressure = bar30.pressure();      // mbar
  float temp     = bar30.temperature();   // deg C
  float depth    = bar30.depth();         // metres

  // Send ONLY machine-readable telemetry on USB Serial.
  // Avoid debug prints because the Jetson shares this port.
  if (isfinite(pressure) &&
      isfinite(temp) &&
      isfinite(depth)) {
    Serial.printf("<%.2f,%.2f,%.3f>\n",
                  pressure, temp, depth);
  }
}

// ================================================================
// Setup
// ================================================================

void setup() {
  Serial.begin(115200);

  // Allocate ESP32 PWM timers.
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  for (int i = 0; i < 8; i++) {
    escs[i].setPeriodHertz(50);
    escs[i].attach(ESC_PINS[i], PWM_MIN, PWM_MAX);
    escs[i].writeMicroseconds(PWM_NEUTRAL);
  }

  sendNeutral();
  lastPacketTime = millis();

  initializeBAR30();

  // No startup/debug messages on Serial.
  // This port carries both PWM commands and telemetry.
}

// ================================================================
// Main loop
// ================================================================

void loop() {
  processSerial();

  // Thruster failsafe
  if (millis() - lastPacketTime > FAILSAFE_MS) {
    if (!failsafeActive) {
      sendNeutral();
      failsafeActive = true;
    }
  }

  // Read and transmit BAR30 measurements
  publishBAR30();
}
