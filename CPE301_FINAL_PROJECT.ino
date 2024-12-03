#include <Wire.h>
#include "RTClib.h"
#include <dht.h>
#include <Stepper.h>
#include <LiquidCrystal.h>

#define TRANSMIT_BUFFER_EMPTY 0x20

// Configuration
const int STEPS_PER_REV = 2038;
Stepper ventMotor(STEPS_PER_REV, 52, 48, 50, 46);
LiquidCrystal screen(8, 7, 6, 5, 4, 3);
dht temperatureSensor;

// RTC Module
RTC_DS1307 clockModule;

// System Modes
enum SystemMode { MODE_OFF, MODE_READY, MODE_ACTIVE, MODE_ALERT };

// Registers and Pins
volatile unsigned char *UART_ControlA = (unsigned char *)0x00C0;
volatile unsigned char *UART_ControlB = (unsigned char *)0x00C1;
volatile unsigned char *UART_ControlC = (unsigned char *)0x00C2;
volatile unsigned int *UART_BaudRate = (unsigned int *)0x00C4;
volatile unsigned char *UART_Data = (unsigned char *)0x00C6;

volatile unsigned char *PORT_K = (unsigned char *)0x108;
volatile unsigned char *DDR_K = (unsigned char *)0x107;
volatile unsigned char *PIN_K = (unsigned char *)0x106;

volatile unsigned char *PORT_F = (unsigned char *)0x31;
volatile unsigned char *DDR_F = (unsigned char *)0x30;
volatile unsigned char *PIN_F = (unsigned char *)0x2F;

volatile unsigned char *ADC_Multiplexer = (unsigned char *)0x7C;
volatile unsigned char *ADC_ControlB = (unsigned char *)0x7B;
volatile unsigned char *ADC_ControlA = (unsigned char *)0x7A;
volatile unsigned int *ADC_Data = (unsigned int *)0x78;

// Function Prototypes
void handleOffMode();
void handleReadyMode();
void handleActiveMode();
void handleAlertMode();
const char* describeMode(SystemMode mode);
void switchMode(SystemMode newMode);

void initializeADC();
unsigned int readAnalogSensor(unsigned char channel);

void adjustVentAngle();
bool isWaterLevelLow();
bool isTemperatureHigh();
void controlFan(bool state);
void updateScreen(float humidity, float temperature);

bool isResetPressed();
bool isStopPressed();
void toggleLEDs(SystemMode mode);

void displayCurrentTime();
void setupUART(unsigned long baudRate);
void sendCharacter(unsigned char character);
void sendText(String message);

// Global State
SystemMode currentMode = MODE_OFF;

void setup() {
    // UART Initialization
    setupUART(9600);

    // RTC Initialization
    clockModule.begin();
    clockModule.adjust(DateTime(F(__DATE__), F(__TIME__)));

    // ADC Setup
    initializeADC();

    // Stepper Motor Setup
    ventMotor.setSpeed(200);

    // LCD Setup
    screen.begin(16, 2);

    // LED and Button Configuration
    *DDR_K |= 0b00001111; // LEDs as output
    *DDR_K &= 0b01110000; // Buttons as input

    // Water Sensor and Fan Configuration
    *DDR_F |= 0b00010111; // Outputs
    *DDR_F &= ~(0x01 << 3); // Inputs
    *PORT_F &= 0b11111110;  // Fan off
}

void loop() {
    toggleLEDs(currentMode);

    switch (currentMode) {
        case MODE_OFF:
            handleOffMode();
            break;
        case MODE_READY:
            handleReadyMode();
            break;
        case MODE_ACTIVE:
            handleActiveMode();
            break;
        case MODE_ALERT:
            handleAlertMode();
            break;
    }
}

// Mode Handlers
void handleOffMode() {
    screen.clear();
    if (isResetPressed()) {
        switchMode(MODE_READY);
    }
}

void handleReadyMode() {
    controlFan(false);
    while (currentMode == MODE_READY) {
        adjustVentAngle();
        if (isTemperatureHigh()) {
            switchMode(MODE_ACTIVE);
        } else if (isWaterLevelLow()) {
            switchMode(MODE_ALERT);
        } else if (isStopPressed()) {
            controlFan(false);
            switchMode(MODE_OFF);
        }
    }
}

void handleActiveMode() {
    controlFan(true);
    while (currentMode == MODE_ACTIVE) {
        adjustVentAngle();
        if (!isTemperatureHigh()) {
            controlFan(false);
            switchMode(MODE_READY);
        } else if (isWaterLevelLow()) {
            controlFan(false);
            switchMode(MODE_ALERT);
        } else if (isStopPressed()) {
            controlFan(false);
            switchMode(MODE_OFF);
        }
    }
}

void handleAlertMode() {
    controlFan(false);
    screen.setCursor(0, 0);
    screen.print("ALERT: WATER LOW");
    while (currentMode == MODE_ALERT) {
        adjustVentAngle();
        if (isResetPressed() && !isWaterLevelLow()) {
            switchMode(MODE_READY);
        } else if (isStopPressed()) {
            switchMode(MODE_OFF);
        }
    }
}

// Helper Functions
void switchMode(SystemMode newMode) {
    if (currentMode != newMode) {
        sendText("Switching to ");
        sendText(describeMode(newMode));
        displayCurrentTime();
        toggleLEDs(newMode);
        currentMode = newMode;
    }
}

const char* describeMode(SystemMode mode) {
    switch (mode) {
        case MODE_OFF: return "OFF";
        case MODE_READY: return "READY";
        case MODE_ACTIVE: return "ACTIVE";
        case MODE_ALERT: return "ALERT";
        default: return "UNKNOWN";
    }
}

void toggleLEDs(SystemMode mode) {
    *PORT_K &= 0b00000000;
    switch (mode) {
        case MODE_OFF: *PORT_K |= 0b00000010; break;  // Yellow
        case MODE_READY: *PORT_K |= 0b00001000; break;  // Green
        case MODE_ACTIVE: *PORT_K |= 0b00000001; break;  // Blue
        case MODE_ALERT: *PORT_K |= 0b00000100; break;  // Red
    }
}

void adjustVentAngle() {
    if (*PIN_K & (0x01 << 6)) {
        ventMotor.step(45);
    }
}

void initializeADC() {
    *ADC_ControlA |= 0b10000000;  // Enable ADC
    *ADC_ControlA &= 0b11011111;  // Disable trigger mode
    *ADC_ControlA &= 0b11110111;  // Disable interrupt
    *ADC_ControlA &= 0b11111000;  // Set prescaler

    *ADC_Multiplexer &= 0b01111111;  // AVCC reference
    *ADC_Multiplexer |= 0b01000000;
    *ADC_Multiplexer &= 0b11011111;  // Right-adjust result
    *ADC_Multiplexer &= 0b11100000;  // Clear channel selection
}

unsigned int readAnalogSensor(unsigned char channel) {
    *ADC_Multiplexer &= 0b11100000;
    *ADC_ControlB &= 0b11110111;

    if (channel > 7) {
        channel -= 8;
        *ADC_ControlB |= 0b00001000;
    }

    *ADC_Multiplexer += channel;
    *ADC_ControlA |= 0x40;  // Start conversion
    while (*ADC_ControlA & 0x40);  // Wait for completion
    return *ADC_Data;
}

bool isWaterLevelLow() {
    *PORT_F |= (0x01 << 2);
    unsigned int level = readAnalogSensor(3);
    *PORT_F &= ~(0x01 << 2);
    return level <= 104;
}

bool isTemperatureHigh() {
    updateScreen(temperatureSensor.humidity, temperatureSensor.temperature);
    return temperatureSensor.temperature > 19;
}

void controlFan(bool state) {
    if (state) {
        *PORT_F |= (1 << 1);
    } else {
        *PORT_F &= ~(1 << 1);
    }
}

void updateScreen(float humidity, float temperature) {
    screen.setCursor(0, 0);
    screen.print("Humidity: ");
    screen.print(humidity);
    screen.setCursor(0, 1);
    screen.print("Temp: ");
    screen.print(temperature);
}

bool isResetPressed() {
    return (*PIN_K & (0x01 << 5));
}

bool isStopPressed() {
    return (*PIN_K & (0x10));
}

void setupUART(unsigned long baudRate) {
    unsigned long FCPU = 16000000;
    unsigned int tbaud = (FCPU / 16 / baudRate - 1);
    *UART_ControlA = 0x20;
    *UART_ControlB = 0x18;
    *UART_ControlC = 0x06;
    *UART_BaudRate = tbaud;
}

void sendCharacter(unsigned char character) {
    while (!(*UART_ControlA & TRANSMIT_BUFFER_EMPTY));
    *UART_Data = character;
}

void sendText(String message) {
    for (int i = 0; i < message.length(); i++) {
        sendCharacter(message[i]);
    }
}

void displayCurrentTime() {
    DateTime now = clockModule.now();
    char buffer[] = "hh:mm:ss";
    sendText("Time: ");
    sendText(now.toString(buffer));
    sendCharacter('\n');
}