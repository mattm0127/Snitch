#ifndef PILOT_H
#define PILOT_H

#include <Arduino.h>

class Pilot {
public:
    // Constructor
    Pilot();

    // Starts the connection to the Flight Controller
    void begin(int rxPin, int txPin);

    // To be called in the main loop
    void update();

    // Public methods for the webserver
    void arm();
    void disarm();
    void setThrottle(int throttleValue);

private:
    // Internal state
    bool _isArmed;
    int _currentThrottle;
    unsigned long _lastMspTime;

    // Internal helpers: MSP Protocol - Flight Controls
    void sendMsp(uint8_t cmd, uint8_t *data, uint8_t n_bytes);
    void sendRC(uint16_t throttle, uint16_t yaw, uint16_t pitch, uint16_t roll, uint16_t aux1);

};


#endif