#ifndef CAMERA_H
#define CAMERA_H

#include <Arduino.h>
#include "esp_camera.h"

class Camera {
public:
    // Initialize the camera
    bool init();

    // Take image into buffer
    camera_fb_t* capture();

    // Release image from buffer
    void release(camera_fb_t* fb);

private:
    camera_config_t config;
};

#endif