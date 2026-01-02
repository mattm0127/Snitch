#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "Camera.h"
#include "Pilot.h"
#include "index_html.h"

// --- 1. WIFI AP CONFIGURATION ---
const char* ssid = "Snitch_Drone";
const char* password = "goldensnitch";


// --- PINS ---
// Pinout specific to Seeed XIAO ESP32S3
#define RX_PIN D7  // Connect to FC TX
#define TX_PIN D6  // Connect to FC RX


// GLOBAL VARIABLES
WebServer server(80);
Camera snitchEye;
Pilot snitchPilot;
SemaphoreHandle_t fcMutex;
camera_fb_t* global_fb = nullptr;

// --- 4. WEB HANDLERS ---
// These functions run when your phone clicks a button

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleArm() {
  snitchPilot.arm(); // Call the Pilot Class!
  server.send(200, "text/plain", "ARMED");
}

void handleDisarm() {
  snitchPilot.disarm(); // Call the Pilot Class!
  server.send(200, "text/plain", "DISARMED");
}

void handleThrottle() {
  if (server.hasArg("val")) {
    int val = server.arg("val").toInt();
    snitchPilot.setThrottle(val); // Update the Pilot Class
  }
  server.send(200, "text/plain", "OK");
}

void handleVision() {
  snitchPilot.toggleVision();
  server.send(200, "text/plain", "OK");
}

void handleCapture() {

  if (!global_fb) {
    server.send(500, "text/plain", "Error capturing image.");
    return;
  }
  
  WiFiClient client = server.client();
  client.setNoDelay(true);

  server.setContentLength(global_fb->len);
  server.send(200, "application/octet-stream", "");
  
  
  // 3. CHUNKED SEND: Send pixels in smaller, digestible pieces
  // This prevents the WiFi stack from "choking" on large data
  size_t totalLen = global_fb->len;
  size_t offset = 0;
  size_t chunkSize = 4096; // Send 4KB at a time
  unsigned long startSend = millis();
  while (offset < totalLen) {
    if (!client.connected()) break;
    if (millis() - startSend > 130) break;

    size_t remaining = totalLen - offset;
    size_t toWrite = remaining;

    client.write(global_fb->buf + offset, toWrite);
    offset += toWrite;
    
    // yield(); // Optional: uncomment if the web server feels "stuck"
  }

  client.stop();
  
}

void taskFunction(void * pvParameters) {
  while (true) {
    unsigned long start = millis();
    global_fb = esp_camera_fb_get();

    // Scan sky and generate THreat
    Threat threat = snitchEye.scanSky(global_fb);
    auto scanTime = millis() - start;

    // Process threat and correct controls
    if (xSemaphoreTake(fcMutex, portMAX_DELAY) == pdTRUE) {
      snitchPilot.cameraMotion(threat);
      xSemaphoreGive(fcMutex);
    }

    // Check for webserver requests
    server.handleClient();
    auto client_time = millis() - start;

    esp_camera_fb_return(global_fb);
    global_fb = nullptr;
    
    auto threadTime = millis() - start;
    Serial.printf("Server: %u ms, Scan: %u ms, Thread: %u ms\n\n", client_time, scanTime, threadTime);
  }
};


// SETUP FUNCTION
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  // Flash the LED if the camera does not initialize properly.
  if (!snitchEye.init()) {
    while(1){
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(100);
    }
  }

  // Calibrate camera, flash 3 times then calibrate
  for (int i=0; i<5; i++){
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(500);
  }
  digitalWrite(LED_BUILTIN, HIGH);
  snitchEye.calibrate();
  delay(50);

  // Initialize Flight Controller
  snitchPilot.begin(RX_PIN, TX_PIN);

  // B. Initialize WiFi
  WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // C. Configure Web Server Routes
  server.on("/", handleRoot);        // Load the page
  server.on("/arm", handleArm);      // Click "Arm"
  server.on("/disarm", handleDisarm);// Click "Disarm"
  server.on("/throttle", handleThrottle); // Move Slider
  server.on("/capture", handleCapture); // Handle image display
  server.on("/vision", handleVision);
  
  server.begin();

  // Start Thread on Core 0
  fcMutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(
    taskFunction,
    "Imaging",
    8192,
    NULL,
    1,
    NULL,
    0
  );

  // Success LED, on for 2 seconds
  digitalWrite(LED_BUILTIN, LOW);
  delay(2000);
  digitalWrite(LED_BUILTIN, HIGH);
}


// MAIN LOOP
void loop() {
  // put your main code here, to run repeatedly:
  
  // Start the timer
  unsigned long start = millis();

  // Update FC with current controls.
  if (xSemaphoreTake(fcMutex, portMAX_DELAY) == pdTRUE) {
  snitchPilot.update();
  xSemaphoreGive(fcMutex);
  }

  //If wifi disconnects, cut power.
  if (WiFi.softAPgetStationNum() == 0) {
     snitchPilot.disarm();
  }

  vTaskDelay(pdMS_TO_TICKS(20));
  Serial.printf("Main loop: %u ms\n\n", millis() - start);
}
// put function definitions here:


