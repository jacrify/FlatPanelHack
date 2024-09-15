// Arduino code for ESP32 WROOM 32
// Emulating an Altinak flat panel device with optional Bluetooth support

#include "BluetoothSerial.h" // Include BluetoothSerial library
#include <Arduino.h>

#define PWM_PIN 12
#define PWM_FREQUENCY 1000 // in Hz
#define PWM_RESOLUTION 16  // bits
#define PWM_CHANNEL 0

const uint16_t MIN_PWM_VALUE = 50;   // minimum PWM value for min brightness
const uint16_t MAX_PWM_VALUE = 5000; // maximum PWM value for max brightness

#define SERIAL_BAUD_RATE 9600

// Device constants
#define PRODUCT_ID 99
#define FIRMWARE_VERSION 1 // 001

// Variables to store device state
uint8_t brightness = 128; // brightness level (0-255)
bool lightOn = true;      // light on/off status

// Bluetooth Serial object
BluetoothSerial BTSerial;

// Pointer to the active serial stream (either Serial or BTSerial)
Stream *activeSerial = &Serial;

void updatePWM();
void sendResponse(String resp);
String buildResponse(char cmdChar, const char *data);
void processCommand(String cmd);

void setup() {
  // Initialize Serial
  Serial.begin(SERIAL_BAUD_RATE);

  // Initialize Bluetooth Serial
  if (!BTSerial.begin("ESP32_FlatPanel")) {
    // If Bluetooth initialization fails, continue without it
    Serial.println("An error occurred initializing Bluetooth");
  } else {
    Serial.println("Bluetooth initialized");
  }

  // Initialize PWM using ledc
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(PWM_PIN, PWM_CHANNEL);

  // Initially set PWM value to zero
  updatePWM();
}

void loop() {
  // Check if Bluetooth is connected
  if (BTSerial.hasClient()) {
    activeSerial = &BTSerial;
  } else {
    activeSerial = &Serial;
  }

  // Read and process commands from the active serial interface
  static String inputString = "";
  while (activeSerial->available()) {
    char inChar = (char)activeSerial->read();
    if (inChar == '\r') {
      // Process the command
      processCommand(inputString);
      // Clear the input string
      inputString = "";
    } else {
      inputString += inChar;
    }
  }

  // Handle potential Bluetooth dropouts
  // If Bluetooth client disconnects, reset the input string
  if (activeSerial == &BTSerial && !BTSerial.hasClient()) {
    inputString = "";
  }

  yield();
}

// Function to update PWM output
void updatePWM() {
  if (lightOn) {
    // Map brightness (0-255) to PWM value (MIN_PWM_VALUE to MAX_PWM_VALUE)
    uint16_t pwmValue = map(brightness, 0, 255, MIN_PWM_VALUE, MAX_PWM_VALUE);
    ledcWrite(PWM_CHANNEL, pwmValue);
  } else {
    // Light is off
    ledcWrite(PWM_CHANNEL, 0);
  }
}

// Function to send response over the active serial interface
void sendResponse(String resp) {
  activeSerial->print(resp);
  activeSerial->print('\n'); // send LF as per instructions
}

// Function to build responses
String buildResponse(char cmdChar, const char *data) {
  char buffer[16];
  sprintf(buffer, "*%c%02d%s", cmdChar, PRODUCT_ID, data);
  return String(buffer);
}

// Function to process commands
void processCommand(String cmd) {
  // Commands start with '>'
  if (cmd.length() == 0 || cmd.charAt(0) != '>') {
    // Invalid command
    return;
  }

  char cmdCode = cmd.charAt(1);
  String response = "";

  switch (cmdCode) {
  case 'P': // Ping
    // Command: >POOO
    // Response: *PiiOOO
    response = buildResponse('P', "OOO");
    sendResponse(response);
    break;
  case 'O': // Open (dummy out)
    // Command: >OOOO
    // Response: *OiiOOO
    response = buildResponse('O', "OOO");
    sendResponse(response);
    break;
  case 'C': // Close (dummy out)
    // Command: >COOO
    // Response: *CiiOOO
    sendResponse(buildResponse('C', "OOO"));
    break;
  case 'L': // Light on
    // Command: >LOOO
    // Response: *LiiOOO
    lightOn = true;
    updatePWM(); // update PWM output
    response = buildResponse('L', "OOO");
    sendResponse(response);
    break;
  case 'D': // Light off
    // Command: >DOOO
    // Response: *DiiOOO
    lightOn = false;
    // Turn off PWM
    ledcWrite(PWM_CHANNEL, 0);
    response = buildResponse('D', "OOO");
    sendResponse(response);
    break;
  case 'B': // Set Brightness
    // Command: >Bxxx
    // Response: *Biixxx
    if (cmd.length() >= 5) {
      String brightnessStr = cmd.substring(2, 5);
      int b = brightnessStr.toInt();
      if (b >= 0 && b <= 255) {
        brightness = (uint8_t)b;
        updatePWM();
        char buffer[4];
        sprintf(buffer, "%03d", brightness);
        response = buildResponse('B', buffer);
        sendResponse(response);
      }
    }
    break;
  case 'J': // Get Brightness
    // Command: >JOOO
    // Response: *Jiixxx
    {
      char buffer[4];
      sprintf(buffer, "%03d", brightness);
      response = buildResponse('J', buffer);
      sendResponse(response);
    }
    break;
  case 'S': // Get State
    // Command: >SOOO
    // Response: *Siiqrs
    {
      char s, r, q;
      // s: cover status, set to '1' (cover closed)
      s = '1';
      // r: light status, '0' off, '1' on
      r = lightOn ? '1' : '0';
      // q: motor status, set to '0' (motor stopped)
      q = '0';
      char buffer[4];
      buffer[0] = q;
      buffer[1] = r;
      buffer[2] = s;
      buffer[3] = '\0';
      response = buildResponse('S', buffer);
      sendResponse(response);
    }
    break;
  case 'V': // Get Version
    // Command: >VOOO
    // Response: *Viivvv
    response = buildResponse('V', "001");
    sendResponse(response);
    break;
  default:
    // Unknown command
    break;
  }
}