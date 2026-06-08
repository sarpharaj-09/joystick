#include <Wire.h>
#include <MPU6050_light.h>

MPU6050 mpu(Wire);

void setup() {
  Serial.begin(115200);
  Wire.begin();                // SDA=21, SCL=22 (default for ESP32)
  
  byte status = mpu.begin();
  Serial.print("MPU6050 status: ");
  Serial.println(status);
  while (status != 0) { }     // stop if no sensor found
  
  Serial.println("Calculating offsets, do not move the sensor...");
  mpu.calcOffsets(true, true); // gyro & accel offsets
  Serial.println("Done!");
}

void loop() {
  mpu.update();  // read fresh data
  
  // Accelerometer (m/s²)
  Serial.print("Acc X: "); Serial.print(mpu.getAccX());
  Serial.print("\tY: ");   Serial.print(mpu.getAccY());
  Serial.print("\tZ: ");   Serial.println(mpu.getAccZ());
  
  // Gyroscope (deg/s)
  Serial.print("Gyro X: "); Serial.print(mpu.getGyroX());
  Serial.print("\tY: ");    Serial.print(mpu.getGyroY());
  Serial.print("\tZ: ");    Serial.println(mpu.getGyroZ());
  
  // Temperature (°C)
  Serial.print("Temp: ");   Serial.print(mpu.getTemp());
  Serial.println(" °C");
  
  // Angle (from integrated gyro, optional)
  Serial.print("Angle X: "); Serial.print(mpu.getAngleX());
  Serial.print("\tY: ");     Serial.print(mpu.getAngleY());
  Serial.print("\tZ: ");     Serial.println(mpu.getAngleZ());
  
  Serial.println("------------------------");
  delay(500);
}