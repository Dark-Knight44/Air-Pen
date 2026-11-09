
#include <Arduino.h>
#include <Wire.h>
#include <BleMouse.h>
#include <math.h>

// ---------------- HARDWARE ----------------

#define MPU_ADDR       0x68
#define SDA_PIN        21
#define SCL_PIN        22
#define BUTTON_PIN     19

// MPU6050 / MPU6500 registers
#define PWR_MGMT_1     0x6B
#define CONFIG_REG     0x1A
#define SMPLRT_DIV     0x19
#define GYRO_CONFIG    0x1B
#define GYRO_XOUT_H    0x43

BleMouse bleMouse("ESP32 Air Pen", "ESP32", 100);

// ---------------- USER SETTINGS ----------------

constexpr float SENSITIVITY = 5.0f;
constexpr float DEADZONE = 2.5f;

// Preserve your original axis directions
constexpr float X_DIRECTION = -1.0f;
constexpr float Y_DIRECTION = -1.0f;

// MPU configured for +/-250 degrees/second
constexpr float GYRO_SCALE = 131.0f;

// Filtering: higher = faster response, lower = smoother
constexpr float FILTER_ALPHA = 0.35f;

// Sampling interval: 5ms = 200Hz
constexpr uint32_t SAMPLE_INTERVAL_US = 5000;

// Button debounce interval
constexpr uint32_t DEBOUNCE_MS = 25;

// ---------------- VARIABLES ----------------

float gyroXOffset = 0.0f;
float gyroZOffset = 0.0f;

float filteredX = 0.0f;
float filteredZ = 0.0f;

float movementX = 0.0f;
float movementY = 0.0f;

uint32_t previousSampleTime = 0;

// Button state
bool lastRawButton = HIGH;
bool stableButton = HIGH;
bool mousePressed = false;

uint32_t lastButtonChange = 0;
bool wasConnected = false;

// ---------------- I2C FUNCTIONS ----------------

bool writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.write(value);

    return Wire.endTransmission() == 0;
}

bool readGyroscope(int16_t &gx, int16_t &gy, int16_t &gz) {

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(GYRO_XOUT_H);

    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    if (Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)6) != 6) {
        while (Wire.available()) {
            Wire.read();
        }
        return false;
    }

    uint16_t rawX = ((uint16_t)Wire.read() << 8) |
                     (uint16_t)Wire.read();

    uint16_t rawY = ((uint16_t)Wire.read() << 8) |
                     (uint16_t)Wire.read();

    uint16_t rawZ = ((uint16_t)Wire.read() << 8) |
                     (uint16_t)Wire.read();

    gx = (int16_t)rawX;
    gy = (int16_t)rawY;
    gz = (int16_t)rawZ;

    return true;
}

// ---------------- SENSOR INITIALIZATION ----------------

void initializeMPU() {

    Serial.println("Initializing motion sensor...");

    if (!writeRegister(PWR_MGMT_1, 0x01)) {
        Serial.println("MPU communication error.");
        while (true) delay(1000);
    }

    // Enable digital low-pass filter
    if (!writeRegister(CONFIG_REG, 0x03)) {
        Serial.println("Failed to configure MPU filter.");
        while (true) delay(1000);
    }

    // 1kHz / (1 + 4) = 200Hz with DLPF enabled
    if (!writeRegister(SMPLRT_DIV, 4)) {
        Serial.println("Failed to configure sample rate.");
        while (true) delay(1000);
    }

    // Gyroscope range: +/- 250 degrees/second
    if (!writeRegister(GYRO_CONFIG, 0x00)) {
        Serial.println("Failed to configure gyroscope.");
        while (true) delay(1000);
    }

    delay(100);
    Serial.println("Motion sensor initialized.");
}

// ---------------- GYROSCOPE CALIBRATION ----------------

void calibrateGyroscope() {

    Serial.println();
    Serial.println("Gyroscope calibration");
    Serial.println("Keep the Air Pen completely still...");

    delay(2000);

    const int samples = 600;

    int64_t xSum = 0;
    int64_t zSum = 0;

    int successfulSamples = 0;

    for (int i = 0; i < samples; i++) {

        int16_t gx, gy, gz;

        if (readGyroscope(gx, gy, gz)) {
            xSum += gx;
            zSum += gz;
            successfulSamples++;
        }

        delay(3);
    }

    if (successfulSamples < samples * 0.9f) {
        Serial.println("Calibration failed. Check sensor connections.");
        while (true) delay(1000);
    }

    gyroXOffset = (float)xSum / successfulSamples;
    gyroZOffset = (float)zSum / successfulSamples;

    filteredX = 0.0f;
    filteredZ = 0.0f;

    Serial.println("Calibration complete.");
    Serial.print("X offset: ");
    Serial.println(gyroXOffset);

    Serial.print("Z offset: ");
    Serial.println(gyroZOffset);
}

// ---------------- BUTTON HANDLING ----------------

void updateButton() {

    bool connected = bleMouse.isConnected();

    if (!connected) {
        mousePressed = false;
        wasConnected = false;
    }

    if (connected && !wasConnected) {
        wasConnected = true;
    }

    bool rawButton = digitalRead(BUTTON_PIN);
    uint32_t now = millis();

    if (rawButton != lastRawButton) {
        lastRawButton = rawButton;
        lastButtonChange = now;
    }

    if ((now - lastButtonChange) >= DEBOUNCE_MS &&
        rawButton != stableButton) {

        stableButton = rawButton;
    }

    if (!connected) {
        return;
    }

    // Press while physically holding the button
    if (stableButton == LOW && !mousePressed) {
        bleMouse.press(MOUSE_LEFT);
        mousePressed = true;
    }

    // Release when the button is released
    if (stableButton == HIGH && mousePressed) {
        bleMouse.release(MOUSE_LEFT);
        mousePressed = false;
    }
}

// ---------------- MOUSE MOVEMENT ----------------

void updateMouseMovement(float velocityX,
                         float velocityY,
                         float dt) {

    if (!bleMouse.isConnected()) {
        movementX = 0.0f;
        movementY = 0.0f;
        return;
    }

    // Preserve fractional movements
    movementX += velocityX * dt;
    movementY += velocityY * dt;

    int moveX = (int)movementX;
    int moveY = (int)movementY;

    // BleMouse movement arguments are signed 8-bit values
    moveX = constrain(moveX, -127, 127);
    moveY = constrain(moveY, -127, 127);

    movementX -= moveX;
    movementY -= moveY;

    if (moveX != 0 || moveY != 0) {
        bleMouse.move((int8_t)moveX, (int8_t)moveY);
    }
}

// ---------------- SETUP ----------------

void setup() {

    Serial.begin(115200);

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(400000);
    Wire.setTimeOut(20);

    initializeMPU();

    calibrateGyroscope();

    bleMouse.begin();

    previousSampleTime = micros();

    Serial.println();
    Serial.println("ESP32 Air Pen is ready.");
    Serial.println("Waiting for Bluetooth connection...");
}

// ---------------- MAIN LOOP ----------------

void loop() {

    updateButton();

    uint32_t currentTime = micros();

    uint32_t elapsed = currentTime - previousSampleTime;

    if (elapsed < SAMPLE_INTERVAL_US) {
        return;
    }

    previousSampleTime = currentTime;

    float dt = elapsed / 1000000.0f;

    // Avoid cursor jumps after a delayed loop iteration
    if (dt > 0.05f || dt <= 0.0f) {
        movementX = 0.0f;
        movementY = 0.0f;
        return;
    }

    int16_t gxRaw, gyRaw, gzRaw;

    if (!readGyroscope(gxRaw, gyRaw, gzRaw)) {
        movementX = 0.0f;
        movementY = 0.0f;
        return;
    }

    // Convert raw gyroscope readings into degrees/second
    float gx = (gxRaw - gyroXOffset) / GYRO_SCALE;
    float gz = (gzRaw - gyroZOffset) / GYRO_SCALE;

    // Low-pass filtering
    filteredX += FILTER_ALPHA * (gx - filteredX);
    filteredZ += FILTER_ALPHA * (gz - filteredZ);

    // Apply deadzone after filtering
    if (fabsf(filteredX) < DEADZONE) {
        filteredX = 0.0f;
    }

    if (fabsf(filteredZ) < DEADZONE) {
        filteredZ = 0.0f;
    }

    // Convert angular velocity into cursor velocity
    float velocityX =
        filteredZ * SENSITIVITY * X_DIRECTION;

    float velocityY =
        filteredX * SENSITIVITY * Y_DIRECTION;

    updateMouseMovement(velocityX, velocityY, dt);
}
