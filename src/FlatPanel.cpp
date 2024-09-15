// Arduino code for ESP32 WROOM 32
// Emulating an Altinak flat panel device

#include <Arduino.h>

#define PWM_PIN 12
#define PWM_FREQUENCY 1000 // in Hz
#define PWM_RESOLUTION 16  // bits
#define PWM_CHANNEL 0

const uint16_t MIN_PWM_VALUE = 0;     // minimum PWM value for min brightness
const uint16_t MAX_PWM_VALUE = 65535; // maximum PWM value for max brightness

#define SERIAL_BAUD_RATE 9600

// Device constants
#define PRODUCT_ID 99
#define FIRMWARE_VERSION 1 // 001

// Variables to store device state
uint8_t brightness = 0; // brightness level (0-255)
bool lightOn = false;   // light on/off status

void setup() {
  // Initialize Serial
  Serial.begin(SERIAL_BAUD_RATE);

  // Initialize PWM using ledc
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(PWM_PIN, PWM_CHANNEL);

  // Initially set PWM value to zero
  ledcWrite(PWM_CHANNEL, 0);
}

// Function to build responses
String buildResponse(char cmdChar, const char *data) {
  char buffer[16];
  sprintf(buffer, "*%c%02d%s", cmdChar, PRODUCT_ID, data);
  return String(buffer);
}

// Function to send response over Serial
void sendResponse(String resp) {
  Serial.print(resp);
  Serial.print('\n'); // send LF as per instructions
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
    // Response: >PiiOOO
    response = buildResponse('P', "OOO");
    sendResponse(response);
    break;
  case 'O': // Open (dummy out)
    // Command: >OOOO
    // Response: >OiiOOO
    response = buildResponse('O', "OOO");
    sendResponse(response);
    break;
  case 'C': // Close (dummy out)
    // Command: >COOO
    // Response: >CiiOOO
    response = buildResponse('C', "OOO");
    sendResponse(response);
    break;
  case 'L': // Light on
    // Command: >LOOO
    // Response: >LiiOOO
    lightOn = true;
    updatePWM(); // update PWM output
    response = buildResponse('L', "OOO");
    sendResponse(response);
    break;
  case 'D': // Light off
    // Command: >DOOO
    // Response: >DiiOOO
    lightOn = false;
    // Turn off PWM
    ledcWrite(PWM_CHANNEL, 0);
    response = buildResponse('D', "OOO");
    sendResponse(response);
    break;
  case 'B': // Set Brightness
    // Command: >Bxxx
    // Response: >Biixxx
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
    // Response: >Jiixxx
    {
      char buffer[4];
      sprintf(buffer, "%03d", brightness);
      response = buildResponse('J', buffer);
      sendResponse(response);
    }
    break;
  case 'S': // Get State
    // Command: >SOOO
    // Response: >Siiqrs
    {
      char s, r, q;
      // s: cover status, set to '2' (cover open)
      s = '2';
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
    // Response: >Vii001
    response = buildResponse('V', "001");
    sendResponse(response);
    break;
  default:
    // Unknown command
    break;
  }
}

void loop() {
  // Read and process commands from Serial
  static String inputString = "";
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\r') {
      // Process the command
      processCommand(inputString);
      // Clear the input string
      inputString = "";
    } else {
      inputString += inChar;
    }
  }

  // Other code can go here
}

// Function to process commands

