#include <Arduino.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include <BleGamepad.h>

MPU6050 mpu(Wire);
BleGamepad bleGamepad;

// Button pins
const int BUTTON_A_PIN = 15;   // Left button
const int BUTTON_B_PIN = 23;   // Right button

// Button state
bool buttonA_pressed = false;
bool buttonB_pressed = false;

// Tuning parameters
const float DEADZONE  = 3.0f;   // degrees of tilt to ignore
const float MAX_TILT  = 45.0f;  // degrees = full axis deflection
const float ALPHA     = 0.8f;   // low-pass filter weight (0=raw, 1=frozen)
const int   LOOP_HZ   = 50;     // update rate
const int   LOOP_MS   = 1000 / LOOP_HZ;

// Filtered axis state
float joyX = 0.0f;
float joyY = 0.0f;

// ── helpers ──────────────────────────────────────────────────────────────────

float applyDeadzone(float angle) {
    if (fabsf(angle) < DEADZONE) return 0.0f;
    // scale so edge of deadzone maps to 0, not a jump
    float sign = (angle > 0) ? 1.0f : -1.0f;
    return sign * (fabsf(angle) - DEADZONE) / (MAX_TILT - DEADZONE);
}

float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// ── setup ────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Wire.begin(21, 22);          // SDA=GPIO21, SCL=GPIO22
    Wire.setClock(400000);       // 400 kHz fast-mode I2C

    // Configure button pins
    pinMode(BUTTON_A_PIN, INPUT);
    pinMode(BUTTON_B_PIN, INPUT);

    // Initialize Bluetooth Gamepad
    bleGamepad.begin();

    // Initialize MPU6050
    Serial.println("Initializing MPU6050...");
    byte status = mpu.begin();
    
    if (status != 0) {
        Serial.println("[ERROR] MPU6050 not found — check wiring!");
        Serial.printf("Status: %d\n", status);
        while (true) delay(1000);
    }

    Serial.println("Calibrating — keep board FLAT and STILL for 3 seconds...");
    delay(1000);
    mpu.calcOffsets(true, true);  // gyro & accel offsets
    Serial.println("Done!");
    
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

    mpu.update();  // Read fresh sensor data

    // Get accelerometer data (in m/s²)
    float Gx = mpu.getAccX();
    float Gy = mpu.getAccY();
    float Gz = mpu.getAccZ();

    // Tilt angles from accelerometer
    float roll  = atan2f(Gy, Gz) * RAD_TO_DEG;   // left/right  → X axis
    float pitch = atan2f(-Gx, Gz) * RAD_TO_DEG;  // fwd/back    → Y axis

    // Deadzone + normalise to -1..+1
    float rawX = clampf(applyDeadzone(roll),  -1.0f, 1.0f);
    float rawY = clampf(applyDeadzone(pitch), -1.0f, 1.0f);

    // Low-pass filter to smooth jitter
    joyX = ALPHA * joyX + (1.0f - ALPHA) * rawX;
    joyY = ALPHA * joyY + (1.0f - ALPHA) * rawY;

    // Read buttons (Active HIGH)
    bool newButtonA = digitalRead(BUTTON_A_PIN) == HIGH;
    bool newButtonB = digitalRead(BUTTON_B_PIN) == HIGH;

    // Map to 8-bit gamepad range (-127 to +127)
    int8_t x_axis = (int8_t)(joyX * 127);
    int8_t y_axis = (int8_t)(joyY * 127);

    // Send joystick axes
    bleGamepad.setLeftThumb(x_axis, y_axis);

    // Handle button press/release (on state change only)
    if (newButtonA && !buttonA_pressed) bleGamepad.press(BUTTON_1);
    if (!newButtonA && buttonA_pressed) bleGamepad.release(BUTTON_1);
    if (newButtonB && !buttonB_pressed) bleGamepad.press(BUTTON_2);
    if (!newButtonB && buttonB_pressed) bleGamepad.release(BUTTON_2);

    buttonA_pressed = newButtonA;
    buttonB_pressed = newButtonB;

    Serial.printf("X=%.2f Y=%.2f | A=%d B=%d\n", joyX, joyY, buttonA_pressed, buttonB_pressed);

    // Hold loop timing
    long elapsed = millis() - t0;
    if (elapsed < LOOP_MS) delay(LOOP_MS - elapsed);
}