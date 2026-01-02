#include "Pilot.h"

// Flight controller Serial Port
#define FC_SERIAL Serial1
#define FC_BAUD 115200
#define MSP_SET_RAW_RC 200


// Public 

Pilot::Pilot(){
    // Set default values, no throttle in initiation
    _isArmed = false;
    _useVision = false;
    _currentThrottle = THROTTLE_OFF;
    _currentYaw = YAW_DEFAULT;
    _currentPitch = PITCH_DEFAULT;
    _currentRoll = ROLL_DEFAULT;
    _lastMspTime = 0;
}

void Pilot::begin(int rxPin, int txPin){
    FC_SERIAL.begin(FC_BAUD, SERIAL_8N1, rxPin, txPin);
}

void Pilot::arm(){
    _isArmed = true;
}

void Pilot::disarm(){
    _isArmed = false;
    _currentThrottle = THROTTLE_OFF;
}

void Pilot::setThrottle(int throttleValue){

    if (throttleValue < 1000) throttleValue = 1000;
    if (throttleValue > 2000) throttleValue = 2000; 

    _currentThrottle = throttleValue;
}

void Pilot::toggleVision() {
    if (!_useVision) {
        _useVision = true;
    }
    else {
        _useVision = false;
    }
}

void Pilot::cameraMotion(Threat &threat){
    if (!_useVision) return;
    // If there isn o threat, reset to defaults and hover
    // If there is a threat, move accordingly.
    if (!threat.active) {
        _currentThrottle = THROTTLE_OFF;
        _currentYaw = YAW_DEFAULT;
        _currentPitch = PITCH_DEFAULT;
        _currentRoll = ROLL_DEFAULT;
    } 
    else if (threat.active) {
        switch (threat.zone_index) {
            case (0):
                NULL;
                break;
            case (1):
                _currentPitch = 1200;
                break;
            case (2):
                NULL;
                break;
            case (3):
                NULL;
                break;
            case (4):
                NULL;
                break;
            case (5):
                NULL;
                break;
            case (6):
                NULL;
                break;
            case (7):
                NULL;
                break;
            case (8):
                NULL;
                break;
        }
    }
}

void Pilot::update(){
    if (millis() - _lastMspTime > 20) {
        _lastMspTime = millis();
        if (_isArmed) {
            sendRC(_currentThrottle, _currentYaw, _currentPitch, _currentRoll, ARM_VALUE);
        }
        else {
            sendRC(THROTTLE_OFF, YAW_DEFAULT, PITCH_DEFAULT, ROLL_DEFAULT, DISARM_VALUE);
        }
    }
}


// Private Helpers

void Pilot::sendRC(uint16_t throttle, uint16_t yaw, uint16_t pitch, uint16_t roll, uint16_t aux1) {
  uint8_t data[16];
  
  // AETR Mapping (Low Byte, High Byte)
  data[0] = roll & 0xFF;     data[1] = roll >> 8;
  data[2] = pitch & 0xFF;    data[3] = pitch >> 8;
  data[4] = throttle & 0xFF; data[5] = throttle >> 8;
  data[6] = yaw & 0xFF;      data[7] = yaw >> 8;
  data[8] = aux1 & 0xFF;     data[9] = aux1 >> 8;
  
  // Clear unused channels
  for(int i=10; i<16; i+=2) {
    data[i] = 1000 & 0xFF;
    data[i+1] = 1000 >> 8;
  }

  sendMsp(MSP_SET_RAW_RC, data, 16);
}

void Pilot::sendMsp(uint8_t cmd, uint8_t *data, uint8_t n_bytes) {
  uint8_t checksum = 0;
  
  FC_SERIAL.write('$');
  FC_SERIAL.write('M');
  FC_SERIAL.write('<');
  FC_SERIAL.write(n_bytes);
  checksum ^= n_bytes;
  
  FC_SERIAL.write(cmd);
  checksum ^= cmd;
  
  for (uint8_t i = 0; i < n_bytes; i++) {
    FC_SERIAL.write(data[i]);
    checksum ^= data[i];
  }
  
  FC_SERIAL.write(checksum);
}
