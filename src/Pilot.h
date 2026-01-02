#ifndef PILOT_H
#define PILOT_H

#include <Arduino.h>
#include <Camera.h>

class Pilot {
public:
    // Constructor
    Pilot();

    // Starts the connection to the Flight Controller
    void begin(int rxPin, int txPin);

    // Public methods for the webserver
    void arm();
    void disarm();
    void setThrottle(int throttleValue);

    // Process the camera response
    void toggleVision();
    void cameraMotion(Threat &threat);

    // To be called in the main loop
    void update();
private:
    // Constants
    const int THROTTLE_DEFAULT = 1500;
    const int THROTTLE_OFF = 1000;
    const int YAW_DEFAULT = 1500;
    const int PITCH_DEFAULT = 1500;
    const int ROLL_DEFAULT = 1500;
    const int ARM_VALUE = 1800;
    const int DISARM_VALUE = 1000;

    // Internal state
    bool _isArmed;
    bool _useVision;
    int _currentThrottle;
    int _currentYaw;
    int _currentPitch;
    int _currentRoll;
    unsigned long _lastMspTime;

    // Internal helpers: MSP Protocol - Flight Controls
    void sendMsp(uint8_t cmd, uint8_t *data, uint8_t n_bytes);
    void sendRC(uint16_t throttle, uint16_t yaw, uint16_t pitch, uint16_t roll, uint16_t aux1);

};


#endif