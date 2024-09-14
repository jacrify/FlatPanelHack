#include <Arduino.h>

const int ledPin = 12; // GPIO12 connected to MOSFET gate
const int bits=8;
const int freq=1000;
const long maxBrightness=(1 << bits) - 1;

// void setup() {
//   // Setup PWM on GPIO12
//   ledcSetup(0, freq, bits); // Channel 0, 5kHz frequency, 16-bit resolution
//   ledcAttachPin(ledPin, 0);
// }

// void loop() {
  
//   // Increase brightness from 0 to 65535
//   for (int brightness = 0; brightness <= maxBrightness; brightness++) {
//     ledcWrite(0, brightness);
//     delay(1000 / maxBrightness); // Shorter delay for smoother transition
//   }

//   // Decrease brightness from 65535 to 0
//   for (int brightness = maxBrightness; brightness >= 0; brightness--) {
//     ledcWrite(0, brightness);
//     delay(1000 / maxBrightness); // Shorter delay for smoother transition
//   }
// }

/*
What: LEDLightBoxAlnitak - PC controlled lightbox implmented using the
        Alnitak (Flip-Flat/Flat-Man) command set found here:
        http://www.optecinc.com/astronomy/pdf/Alnitak%20Astrosystems%20GenericCommandsR3.pdf

Who:
        Created By: Jared Wellman - jared@mainsequencesoftware.com

When:
        Last modified:  2013/May/05


Typical usage on the command prompt:
Send     : >S000\n      //request state
Recieve  : *S19000\n    //returned state

Send     : >B128\n      //set brightness 128
Recieve  : *B19128\n    //confirming brightness set to 128

Send     : >J000\n      //get brightness
Recieve  : *B19128\n    //brightness value of 128 (assuming as set from above)

Send     : >L000\n      //turn light on (uses set brightness value)
Recieve  : *L19000\n    //confirms light turned on

Send     : >D000\n      //turn light off (brightness value should not be
changed) Recieve  : *D19000\n    //confirms light turned off.
*/


long brightness = 0;

enum devices {
  FLAT_MAN_L = 10,
  FLAT_MAN_XL = 15,
  FLAT_MAN = 19,
  FLIP_FLAT = 99
};

enum motorStatuses { STOPPED = 0, RUNNING };

enum lightStatuses { OFF = 0, ON };

enum shutterStatuses {
  UNKNOWN = 0, // ie not open or closed...could be moving
  CLOSED,
  OPEN
};

int deviceId = FLIP_FLAT;
int motorStatus = STOPPED;
int lightStatus = OFF;
int coverStatus = UNKNOWN;

void setup() {
  // initialize the serial communication:
  Serial.begin(9600);
  // initialize the ledPin as an output:
  ledcSetup(0, freq, bits); // Channel 0, 5kHz frequency, 16-bit resolution
  ledcAttachPin(ledPin, 0);
  ledcWrite(0, 255);
}


void SetShutter(int val) {
  if (val == OPEN && coverStatus != OPEN) {
    coverStatus = OPEN;
    // TODO: Implement code to OPEN the shutter.
  } else if (val == CLOSED && coverStatus != CLOSED) {
    coverStatus = CLOSED;
    // TODO: Implement code to CLOSE the shutter
  } else {
    // TODO: Actually handle this case
    coverStatus = val;
  }
}

void handleSerial() {
  if (Serial.available() >= 6) // all incoming communications are fixed length
                               // at 6 bytes including the \n
  {
    char str[20];
    memset(str, 0, 20);

    // Read the command until newline
    Serial.readBytesUntil('\r', str, 20);

    char cmd = str[1]; // Command character
    char data[4];      // To hold data (3 digits and null terminator)
    strncpy(data, str + 2, 3);
    data[3] = '\0'; // Null-terminate data

    // Useful for debugging to make sure your commands came through and are
    // parsed correctly.
    if (false) {
      char temp[20];
      sprintf(temp, "cmd = >%c, data = %s", cmd, data);
      Serial.print(temp);
      Serial.print('\n');
    }

    char response[20];

    switch (cmd) {
    /*
        Ping device
        Request: >P000\n
        Return : *Pii000\n
                id = deviceId
    */
    case 'P':
      sprintf(response, "*P%dOOO", deviceId);
      Serial.print(response);
      Serial.print('\n');
      break;

    /*
        Open shutter
        Request: >O000\n
        Return : *Oii000\n
                id = deviceId

        This command is only supported on the Flip-Flat!
    */
    case 'O':
      sprintf(response, "*O%dOOO", deviceId);
      SetShutter(OPEN);
      Serial.print(response);
      Serial.print('\n');
      break;

    /*
        Close shutter
        Request: >C000\n
        Return : *Cii000\n
                id = deviceId

        This command is only supported on the Flip-Flat!
    */
    case 'C':
      sprintf(response, "*C%dOOO", deviceId);
      SetShutter(CLOSED);
      Serial.print(response);
      Serial.print('\n');
      break;

    /*
        Turn light on
        Request: >L000\n
        Return : *Lii000\n
                id = deviceId
    */
    case 'L':
      sprintf(response, "*L%dOOO", deviceId);
      Serial.print(response);
      Serial.print('\n');
      lightStatus = ON;
      ledcWrite(0, brightness);
      break;

    /*
        Turn light off
        Request: >D000\n
        Return : *Dii000\n
                id = deviceId
    */
    case 'D':
      sprintf(response, "*D%dOOO", deviceId);
      Serial.print(response);
      Serial.print('\n');
      lightStatus = OFF;
      ledcWrite(0, 0);
      break;

    /*
        Set brightness
        Request: >Bxxx\n
                xxx = brightness value from 000-255
        Return : *Biixxx\n
                id = deviceId
                xxx = value that brightness was set from 000-255
    */
    case 'B':
      brightness = atoi(data);
      if (lightStatus == ON)
        ledcWrite(0, brightness);
      sprintf(response, "*B%d%03d", deviceId, brightness);
      Serial.print(response);
      Serial.print('\n');
      break;

    /*
        Get brightness
        Request: >J000\n
        Return : *JiiXXX\n
                id = deviceId
                XXX = current brightness value from 000-255
    */
    case 'J':
      sprintf(response, "*J%d%03d", deviceId, brightness);
      Serial.print(response);
      Serial.print('\n');
      break;

    /*
        Get device status:
        Request: >S000\n
        Return : *SiiMLC\n
                id = deviceId
                M  = motor status (0 stopped, 1 running)
                L  = light status (0 off, 1 on)
                C  = Cover Status (0 moving, 1 closed, 2 open)
    */
    case 'S':
      sprintf(response, "*S%d%d%d%d", deviceId, motorStatus, lightStatus,
              coverStatus);
      Serial.print(response);
      Serial.print('\n'); // Add just the LF character
  
      break;

    /*
        Get firmware version
        Request: >V000\n
        Return : *Vii001\n
                id = deviceId
    */
    case 'V':
      sprintf(response, "*V%d001", deviceId);
      Serial.print(response);
      Serial.print('\n');
      break;

    default:
      // Handle unknown command
      Serial.print("*E000"); // Example error response
      Serial.print('\n');
      break;
    }

    // Clear any remaining bytes in the buffer
    while (Serial.available() > 0)
      Serial.read();
  }
}

void loop() { handleSerial(); }