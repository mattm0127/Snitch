#ifndef CAMERA_H
#define CAMERA_H

#include <Arduino.h>
#include "esp_camera.h"

struct Threat {
    bool active; // Is there an active threat
    int zone_index; // Which zone index has the threat.
    int mass; // Mass (pixel ct) of threat.
};

class Camera {
public:
    // Initialize the camera
    bool init();

    // Calibrate brightness threshold
    void calibrate();

    // Scan for incoming attack
    Threat scanSky();

private:
    // Config info
    camera_config_t config;

    // Framebuffer pointer
    camera_fb_t* fb = nullptr;

    // Baseline brightness level for shadow comparison
    int baseline_brightness = 0;

    // Sensitivity from baseline
    const int SENSITIVITY = 50;
};

#endif