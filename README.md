# 🕹️ Tilt-Controlled Bluetooth Gamepad (ESP32 + GY-521)

Build your own wireless gamepad using an **ESP32**, a **GY-521 (MPU6050)** accelerometer/gyroscope, and two physical buttons.  
Tilt the sensor to control the left thumbstick (X/Y axes) and press the buttons for actions.  
The ESP32 emulates a standard Bluetooth gamepad – works with Windows, Android, Linux, and any device that supports BLE HID.

![Demo](https://via.placeholder.com/800x400?text=Tilt+Gamepad+Demo) <!-- Replace with actual image/video if you have one -->

## 🎯 Features

- **Tilt‑controlled joystick** – roll = X axis, pitch = Y axis
- **Adjustable deadzone and smoothing** – no jitter, stable neutral zone
- **Two physical buttons** – map to gamepad buttons (e.g., A and B)
- **Bluetooth Low Energy (BLE) HID** – works as a native gamepad, no extra drivers required
- **Configurable sensitivity** – change `MAX_TILT` for more/less responsive tilt
- **Low‑pass filter** – removes noise from accelerometer readings

## 📦 Hardware Required

| Component               | Quantity |
|-------------------------|----------|
| ESP32 development board | 1        |
| GY‑521 (MPU6050) module | 1        |
| Tactile push buttons    | 2        |
| 10kΩ resistors (optional, if not using internal pull‑ups) | 2 |
| Breadboard & jumper wires | as needed |

## 🔌 Wiring Diagram

| GY‑521          | ESP32          |
|-----------------|----------------|
| VCC             | 3.3V           |
| GND             | GND            |
| SCL             | GPIO22         |
| SDA             | GPIO21         |

| Button          | ESP32                |
|-----------------|----------------------|
| Button A (left) | GPIO15 → 3.3V        |
| Button B (right)| GPIO23 → 3.3V        |

> **Note:** The code reads buttons as **HIGH** when pressed, so connect each button between the GPIO pin and **3.3V** (pull‑down resistor not required if you use INPUT mode, but an external 10kΩ pull‑down is recommended to avoid floating).  
> Alternatively, modify the code to use `INPUT_PULLUP` and connect buttons to GND.

## ⚙️ Software Setup

### 1. Install Required Libraries (Arduino IDE)

- **BleGamepad** by *lemmingDev* – [Library Manager](https://www.arduino.cc/reference/en/libraries/blegamepad/)
- **MPU6050_light** by *eF* – [Library Manager](https://www.arduino.cc/reference/en/libraries/mpu6050_light/)
- **Wire** (built‑in)

### 2. Board Configuration

- Board: `ESP32 Dev Module`
- Partition Scheme: `Huge APP (3MB No OTA/1MB SPIFFS)` (recommended for BLE)
- Upload Speed: `921600`

### 3. Upload the Code

Open the provided `.ino` sketch, select the correct COM port, and upload.

## 🚀 How to Use

### Calibration (First Run)

1. Open the **Serial Monitor** (115200 baud).
2. Keep the GY‑521 **flat and absolutely still** – the code will automatically calculate offsets.
3. After calibration, the ESP32 advertises as **"ESP32 Gamepad"**.

### Pairing with Your Device

| Device | Steps |
|--------|-------|
| **Windows 10/11** | Bluetooth settings → Add device → "ESP32 Gamepad" → Pair |
| **Android** | Settings → Bluetooth → Scan → Tap "ESP32 Gamepad" |
| **Linux** | Use `bluetoothctl` or GUI; the pad appears as `/dev/input/js0` |
| **macOS** | System Preferences → Bluetooth → Pair |

### Testing

- **Windows**: Run `joy.cpl` → select your gamepad → Properties → Test tab.
- **Online tester**: [Gamepad Tester](https://gamepad-tester.com/) (works in any browser).
- **Linux**: `jstest /dev/input/js0`

### Playing Games

Most modern games expect an Xbox controller. If your game doesn't recognise the tilt pad directly:

- **x360ce** – emulates an Xbox 360 controller (recommended for Game Pass / modern games).
- **JoyToKey** or **AntiMicroX** – map joystick axes to keyboard keys (WASD) for older games.
- **Steam Input** – if you play via Steam, add the game as a non‑Steam game and configure the controller in Steam's settings.

## 🛠️ Customisation (Code Tuning)

Open the sketch and adjust these parameters at the top:

```cpp
const float DEADZONE  = 3.0f;   // degrees – movement below this is ignored
const float MAX_TILT  = 45.0f;  // degrees – full joystick deflection at this tilt
const float ALPHA     = 0.8f;   // low‑pass filter (0 = raw, 1 = frozen)
const int   LOOP_HZ   = 50;     // update rate (Hz)