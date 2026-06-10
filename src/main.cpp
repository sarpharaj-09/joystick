#include <Arduino.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include <BleGamepad.h>
#include <Preferences.h>        // ESP32 NVS (non-volatile storage)

MPU6050 mpu(Wire);
BleGamepad bleGamepad;
Preferences prefs;

// Button pins
const int BUTTON_A_PIN = 15;
const int BUTTON_B_PIN = 4;
const int BUTTON_C_PIN = 5;
const int BUTTON_D_PIN = 18;

// LED pins
const int LED_AB_PIN = 19;
const int LED_CD_PIN = 23;

// Hold Button A on boot for 3 seconds to force recalibration
const int RECAL_BUTTON = BUTTON_A_PIN;
const int RECAL_HOLD_MS = 3000;

// Button state
bool buttonA_pressed = false;
bool buttonB_pressed = false;
bool buttonC_pressed = false;
bool buttonD_pressed = false;

// Tuning parameters
const float DEADZONE  = 3.0f;
const float MAX_TILT  = 45.0f;
const float ALPHA     = 0.8f;
const int   LOOP_HZ   = 50;
const int   LOOP_MS   = 1000 / LOOP_HZ;

// Filtered axis state
float joyX = 0.0f;
float joyY = 0.0f;

// ── helpers ──────────────────────────────────────────────────────────────────

float applyDeadzone(float angle) {
    if (fabsf(angle) < DEADZONE) return 0.0f;
    float sign = (angle > 0) ? 1.0f : -1.0f;
    return sign * (fabsf(angle) - DEADZONE) / (MAX_TILT - DEADZONE);
}

float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// ── NVS save/load ─────────────────────────────────────────────────────────────

void saveOffsets() {
    prefs.begin("mpu_cal", false);                      // open NVS namespace
    prefs.putFloat("ax", mpu.getAccXoffset());
    prefs.putFloat("ay", mpu.getAccYoffset());
    prefs.putFloat("az", mpu.getAccZoffset());
    prefs.putFloat("gx", mpu.getGyroXoffset());
    prefs.putFloat("gy", mpu.getGyroYoffset());
    prefs.putFloat("gz", mpu.getGyroZoffset());
    prefs.putBool("valid", true);
    prefs.end();
    Serial.println("[NVS] Calibration saved!");
}

bool loadOffsets() {
    prefs.begin("mpu_cal", true);                       // open read-only
    bool valid = prefs.getBool("valid", false);
    if (valid) {
        mpu.setAccOffsets(
            prefs.getFloat("ax", 0),
            prefs.getFloat("ay", 0),
            prefs.getFloat("az", 0)
        );
        mpu.setGyroOffsets(
            prefs.getFloat("gx", 0),
            prefs.getFloat("gy", 0),
            prefs.getFloat("gz", 0)
        );
        Serial.println("[NVS] Calibration loaded from storage!");
    }
    prefs.end();
    return valid;
}

// ── calibration routine ───────────────────────────────────────────────────────

void runCalibration() {
    // Blink LED_AB to signal calibration mode
    for (int i = 0; i < 6; i++) {
        digitalWrite(LED_AB_PIN, HIGH); delay(150);
        digitalWrite(LED_AB_PIN, LOW);  delay(150);
    }
    Serial.println("Calibrating — keep board FLAT and STILL for 3 seconds...");
    delay(1000);
    mpu.calcOffsets(true, true);
    saveOffsets();
    Serial.println("Calibration done and saved!");

    // Solid LED for 1 second to confirm save
    digitalWrite(LED_AB_PIN, HIGH); delay(1000);
    digitalWrite(LED_AB_PIN, LOW);
}

// ── setup ────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(1000);

    Wire.begin(21, 22);
    Wire.setClock(400000);

    pinMode(BUTTON_A_PIN, INPUT_PULLDOWN);
    pinMode(BUTTON_B_PIN, INPUT_PULLDOWN);
    pinMode(BUTTON_C_PIN, INPUT_PULLDOWN);
    pinMode(BUTTON_D_PIN, INPUT_PULLDOWN);

    pinMode(LED_AB_PIN, OUTPUT);
    pinMode(LED_CD_PIN, OUTPUT);
    digitalWrite(LED_AB_PIN, LOW);
    digitalWrite(LED_CD_PIN, LOW);

    bleGamepad.begin();

    Serial.println("Initializing MPU6050...");
    byte status = mpu.begin();
    if (status != 0) {
        Serial.println("[ERROR] MPU6050 not found — check wiring!");
        Serial.printf("Status: %d\n", status);
        while (true) delay(1000);
    }

    // ── Check if user is holding Button A for forced recalibration ────────────
    Serial.println("Hold Button A for 3 seconds to recalibrate...");
    unsigned long holdStart = millis();
    bool forceRecal = false;

    // Flash LED_CD while waiting
    while (millis() - holdStart < RECAL_HOLD_MS) {
        digitalWrite(LED_CD_PIN, HIGH); delay(100);
        digitalWrite(LED_CD_PIN, LOW);  delay(100);
        if (digitalRead(RECAL_BUTTON) == LOW) {
            forceRecal = false;
            break;                                      // released early, skip
        }
        forceRecal = true;
    }
    digitalWrite(LED_CD_PIN, LOW);

    if (forceRecal) {
        Serial.println("[BOOT] Forced recalibration triggered!");
        runCalibration();
    } else {
        // Try to load saved offsets
        if (!loadOffsets()) {
            Serial.println("[BOOT] No saved calibration found, running first-time calibration...");
            runCalibration();
        }
    }

    Serial.println("Waiting for Bluetooth connection...");
    while (!bleGamepad.isConnected()) {
        delay(100);
        Serial.print(".");
    }
    Serial.println("\nBluetooth Connected!");
}

// ── loop ─────────────────────────────────────────────────────────────────────

void loop() {
    unsigned long t0 = millis();

    mpu.update();

    float Gx = mpu.getAccX();
    float Gy = mpu.getAccY();
    float Gz = mpu.getAccZ();

    float roll  = atan2f(Gy, Gz) * RAD_TO_DEG;
    float pitch = atan2f(-Gx, Gz) * RAD_TO_DEG;

    float rawX = clampf(applyDeadzone(roll),  -1.0f, 1.0f);
    float rawY = clampf(applyDeadzone(pitch), -1.0f, 1.0f);

    joyX = ALPHA * joyX + (1.0f - ALPHA) * rawX;
    joyY = ALPHA * joyY + (1.0f - ALPHA) * rawY;

    // Read all four buttons (Active HIGH)
    bool newButtonA = digitalRead(BUTTON_A_PIN) == HIGH;
    bool newButtonB = digitalRead(BUTTON_B_PIN) == HIGH;
    bool newButtonC = digitalRead(BUTTON_C_PIN) == HIGH;
    bool newButtonD = digitalRead(BUTTON_D_PIN) == HIGH;

    // ── LED control ──────────────────────────────────────────────────────────
    digitalWrite(LED_AB_PIN, (newButtonA || newButtonB) ? HIGH : LOW);
    digitalWrite(LED_CD_PIN, (newButtonC || newButtonD) ? HIGH : LOW);

    // ── Gamepad axes ─────────────────────────────────────────────────────────
    int8_t x_axis = (int8_t)(joyX * 127);
    int8_t y_axis = (int8_t)(joyY * 127);
    bleGamepad.setLeftThumb(x_axis, y_axis);

    // ── Gamepad buttons ──────────────────────────────────────────────────────
    if (newButtonA && !buttonA_pressed) bleGamepad.press(BUTTON_1);
    if (!newButtonA && buttonA_pressed) bleGamepad.release(BUTTON_1);

    if (newButtonB && !buttonB_pressed) bleGamepad.press(BUTTON_2);
    if (!newButtonB && buttonB_pressed) bleGamepad.release(BUTTON_2);

    if (newButtonC && !buttonC_pressed) bleGamepad.press(BUTTON_3);
    if (!newButtonC && buttonC_pressed) bleGamepad.release(BUTTON_3);

    if (newButtonD && !buttonD_pressed) bleGamepad.press(BUTTON_4);
    if (!newButtonD && buttonD_pressed) bleGamepad.release(BUTTON_4);

    buttonA_pressed = newButtonA;
    buttonB_pressed = newButtonB;
    buttonC_pressed = newButtonC;
    buttonD_pressed = newButtonD;

    Serial.printf("X=%.2f Y=%.2f | A=%d B=%d C=%d D=%d\n",
                  joyX, joyY,
                  buttonA_pressed, buttonB_pressed,
                  buttonC_pressed, buttonD_pressed);

    long elapsed = millis() - t0;
    if (elapsed < LOOP_MS) delay(LOOP_MS - elapsed);
}