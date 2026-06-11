// ================================================================
// ESP32 – 8-Thruster PWM Controller
// Receives: "<pwm0,pwm1,pwm2,pwm3,pwm4,pwm5,pwm6,pwm7>\n" over Serial
// Outputs:  Standard ESC PWM (1000–2000µs) on pins below
// ================================================================

#include <ESP32Servo.h>

// --- Pin assignments (change to match your wiring) ---
// T1–T4: Horizontal (FL, FR, BL, BL)
// T5–T8: Vertical Heave (HFL, HFR, HBL, HBR)
const int ESC_PINS[8] = {13, 12, 14, 27, 26, 25, 33, 32};

/*
13 - FL
12 - FR
14 - BL
27 - BR
26 - HFL
25 - HFR
33 - HBL
32 - HBR
*/

// --- PWM limits ---
const int PWM_MIN     = 1000;
const int PWM_MAX     = 2000;
const int PWM_NEUTRAL = 1500;

// --- Failsafe ---
const unsigned long FAILSAFE_MS = 500;   // go neutral if no packet for 500ms
unsigned long lastPacketTime    = 0;
bool failsafeActive             = false;

Servo escs[8];

// ---------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  // ESP32Servo: allocate timers before attaching
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  for (int i = 0; i < 8; i++) {
    escs[i].setPeriodHertz(50);           // standard 50 Hz ESC signal
    escs[i].attach(ESC_PINS[i], PWM_MIN, PWM_MAX);
    escs[i].writeMicroseconds(PWM_NEUTRAL);
  }

  Serial.println("[INFO] ESP32 ESC controller ready");
  Serial.println("[INFO] Waiting for packets: <pwm0,...,pwm7>");
  delay(3000);  // give ESCs time to arm at neutral
}

// ---------------------------------------------------------------
int clamp(int val) {
  return max(PWM_MIN, min(PWM_MAX, val));
}

void sendNeutral() {
  for (int i = 0; i < 8; i++) {
    escs[i].writeMicroseconds(PWM_NEUTRAL);
  }
  Serial.println("[FAILSAFE] All thrusters set to neutral");
}

void applyPWM(int pwm[8]) {
  for (int i = 0; i < 8; i++) {
    escs[i].writeMicroseconds(clamp(pwm[i]));
  }
}

// ---------------------------------------------------------------
// Parse "<1500,1500,1500,1500,1500,1500,1500,1500>"
bool parsePacket(String msg, int pwm[8]) {
  msg.trim();
  if (!msg.startsWith("<") || !msg.endsWith(">")) return false;

  msg = msg.substring(1, msg.length() - 1);  // strip < >

  int idx = 0;
  while (msg.length() > 0 && idx < 8) {
    int comma = msg.indexOf(',');
    String token = (comma == -1) ? msg : msg.substring(0, comma);
    token.trim();

    if (token.length() == 0) return false;
    pwm[idx++] = token.toInt();

    if (comma == -1) break;
    msg = msg.substring(comma + 1);
  }

  return (idx == 8);
}

// ---------------------------------------------------------------
void loop() {
  // --- Failsafe check ---
  if (millis() - lastPacketTime > FAILSAFE_MS) {
    if (!failsafeActive) {
      sendNeutral();
      failsafeActive = true;
    }
  }

  // --- Read serial ---
  if (Serial.available() > 0) {
    String incoming = Serial.readStringUntil('\n');
    incoming.trim();

    if (incoming.length() == 0) return;

    int pwm[8];
    if (parsePacket(incoming, pwm)) {
      applyPWM(pwm);
      lastPacketTime = millis();
      failsafeActive = false;

      // Debug echo
      Serial.print("[PWM] ");
      for (int i = 0; i < 8; i++) {
        Serial.print("T"); Serial.print(i + 1);
        Serial.print(":"); Serial.print(clamp(pwm[i]));
        if (i < 7) Serial.print("  ");
      }
      Serial.println();
    } else {
      Serial.print("[WARN] Bad packet: ");
      Serial.println(incoming);
    }
  }
}