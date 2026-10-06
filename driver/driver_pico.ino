// Raspberry Pi Pico – 8-Thruster PWM Controller
// Receives: "<pwm0,pwm1,pwm2,pwm3,pwm4,pwm5,pwm6,pwm7>\n"
// Outputs:  Standard ESC PWM (1100–1900us) on pins below    

// --- Pin assignments ---
// T1–T4: Horizontal
// T5–T8: Vertical Heave
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
const int PWM_MIN     = 1100;
const int PWM_MAX     = 1900;
const int PWM_NEUTRAL = 1500;

// --- ESC PWM frequency ---
const int PWM_FREQUENCY = 50;

// --- Failsafe ---
const unsigned long FAILSAFE_MS = 500;

unsigned long lastPacketTime = 0;
bool failsafeActive = false;

// Convert microseconds to Pico PWM duty cycle

//
// At 50 Hz:
//
// Period = 20,000 us
//
// Example:
// 1100 us -> 5.5%
// 1500 us -> 7.5%
// 1900 us -> 9.5%
//
// Pico PWM uses a 16-bit duty value (0–65535).

uint16_t microsecondsToDuty(int microseconds)
{
    const uint32_t PERIOD_US = 20000;

    microseconds = constrain(
        microseconds,
        PWM_MIN,
        PWM_MAX
    );

    return (uint16_t)(
        ((uint32_t)microseconds * 65535UL) /
        PERIOD_US
    );
}

// Clamp PWM value
int clampPWM(int val)
{
    return max(PWM_MIN, min(PWM_MAX, val));
}

// Set PWM for one thruster
void setThrusterPWM(int index, int microseconds)
{
    microseconds = clampPWM(microseconds);

    analogWrite(
        ESC_PINS[index],
        microsecondsToDuty(microseconds)
    );
}

// Set all thrusters to neutral
void sendNeutral()
{
    for (int i = 0; i < 8; i++)
    {
        setThrusterPWM(i, PWM_NEUTRAL);
    }

    Serial.println("[FAILSAFE] All thrusters set to neutral");
}

// Apply PWM values to all 8 thrusters
void applyPWM(int pwm[8])
{
    for (int i = 0; i < 8; i++)
    {
        setThrusterPWM(i, pwm[i]);
    }
}

// Parse packet:
//
// <1500,1500,1500,1500,1500,1500,1500,1500>

bool parsePacket(String msg, int pwm[8])
{
    msg.trim();

    // Must start with < and end with >
    if (!msg.startsWith("<") || !msg.endsWith(">"))
    {
        return false;
    }

    // Remove < >
    msg = msg.substring(
        1,
        msg.length() - 1
    );

    int idx = 0;

    while (msg.length() > 0 && idx < 8)
    {
        int comma = msg.indexOf(',');

        String token;

        if (comma == -1)
        {
            token = msg;
        }
        else
        {
            token = msg.substring(
                0,
                comma
            );
        }

        token.trim();

        // Reject empty values
        if (token.length() == 0)
        {
            return false;
        }

        pwm[idx++] = token.toInt();

        if (comma == -1)
        {
            break;
        }

        msg = msg.substring(comma + 1);
    }

    return (idx == 8);
}

// Setup
void setup()
{
    Serial.begin(115200);

    // Configure all 8 PWM outputs
    for (int i = 0; i < 8; i++)
    {
        pinMode(ESC_PINS[i], OUTPUT);

        // Arduino-Pico:
        // analogWriteFreq() is global, so frequency is configured
        // once below.
    }

    // Standard ESC frequency
    analogWriteFreq(PWM_FREQUENCY);

    // 16-bit PWM resolution
    analogWriteRange(65535);

    // Initialize all ESCs to neutral
    for (int i = 0; i < 8; i++)
    {
        setThrusterPWM(
            i,
            PWM_NEUTRAL
        );
    }

    Serial.println(
        "[INFO] Raspberry Pi Pico ESC controller ready"
    );

    Serial.println(
        "[INFO] Waiting for packets: <pwm0,...,pwm7>"
    );

    // Give ESCs time to arm at neutral
    delay(3000);
}

// Main loop
void loop()
{
    // Failsafe check
    if (millis() - lastPacketTime > FAILSAFE_MS)
    {
        if (!failsafeActive)
        {
            sendNeutral();

            failsafeActive = true;
        }
    }

    // Read serial
    if (Serial.available() > 0)
    {
        String incoming =
            Serial.readStringUntil('\n');

        incoming.trim();

        if (incoming.length() == 0)
        {
            return;
        }

        // Parse PWM packet
        int pwm[8];

        if (parsePacket(incoming, pwm))
        {
            // Apply PWM
            applyPWM(pwm);

            // Update communication watchdog
            lastPacketTime = millis();

            failsafeActive = false;

            // Debug echo
            Serial.print("[PWM] ");

            for (int i = 0; i < 8; i++)
            {
                Serial.print("T");
                Serial.print(i + 1);

                Serial.print(":");

                Serial.print(
                    clampPWM(pwm[i])
                );

                if (i < 7)
                {
                    Serial.print("  ");
                }
            }

            Serial.println();
        }
        else
        {
            Serial.print("[WARN] Bad packet: ");
            Serial.println(incoming);
        }
    }
}