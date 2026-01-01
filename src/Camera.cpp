#include "Camera.h"

#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39

#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

// Threat detection settings.
#define Z_WIDTH  53 
#define Z_HEIGHT 40 
#define MIN_THREAT_MASS 400

bool Camera::init() {
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    // External Clock frequency
    config.xclk_freq_hz = 20000000;

    // Black and white imaging
    config.pixel_format = PIXFORMAT_GRAYSCALE;

    // QQVGA resolution is 160x120
    config.frame_size = FRAMESIZE_QQVGA;

    config.fb_count = 2;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        return false;
    }
    return true;
}

void Camera::calibrate() {
    // Get a frame
    fb = esp_camera_fb_get();
    if (!fb) return;

    // Set base values and constants
    long total_brightness = 0;

    // Loop through all of the pixels and grab color value (brightness)
    for (int i=0; i<fb->len; i+=8) {
        total_brightness += fb->buf[i];
    }

    // Calculate baseline brightness for the room.
    baseline_brightness = total_brightness / (fb->len/8);

    // If baseline is too low, flash the LED 5 times.
    if (baseline_brightness < 40) {
        baseline_brightness = 40;
        pinMode(LED_BUILTIN, OUTPUT);
        for (int i=0; i<6; i++){
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(100);
        }
        // Turn the LED off after the blink.
        digitalWrite(LED_BUILTIN, HIGH);
    }

    // Clear the framebuffer
    esp_camera_fb_return(fb);
    fb = nullptr;
}

Threat Camera::scanSky() {
    Threat result = {false, -1, 0}; // default, no threat

    // Get image
    fb = esp_camera_fb_get();
    if (!fb) return result; // Failed image, return no threat.

    // Prepare the zone scores for each grid zone
    // Grid 0, 1, 2 -> "Front" of drone
    // Grid 3, 4, 5 -> "Middle" of drone
    // Grid 6, 7, 8 -> "Back" of drone
    int zone_scores[9];
    memset(zone_scores, 0, sizeof(zone_scores));

    // Set dynamic threshold (default to 40 if too low)
    int dynamic_threshold = baseline_brightness - SENSITIVITY;
    if (dynamic_threshold < 0) dynamic_threshold = 40;
    // loop through pixels, add to threat zone
    for (int y = 0; y < 120; y ++) {
    // Pre-calculate Y zone once per row
    int grid_y = (y < 40) ? 0 : (y < 80 ? 1 : 2); 
    int y_offset = grid_y * 3;
    int row_start = y * 160;

    for (int x = 0; x < 160; x ++) {
        uint8_t pixel_value = fb->buf[row_start + x];
        
        if (pixel_value < dynamic_threshold) {
            int grid_x = (x < 53) ? 0 : (x < 106 ? 1 : 2);
            zone_scores[y_offset + grid_x]++;
        }
    }
}

    // Find zone with greatest pixel mass
    int max_score = 0;
    for (int i=0; i<9; i++) {
        if (zone_scores[i] > max_score) {
            max_score = zone_scores[i];
            result.zone_index = i;
        }
    }
    result.mass = max_score;

    // Check if the mass is greater than the minimum threat
    if (result.mass > MIN_THREAT_MASS) {
        result.active = true;
    }

    // Clean up frame buffer
    esp_camera_fb_return(fb);
    fb = nullptr;

    return result;
}
