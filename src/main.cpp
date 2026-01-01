#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "Camera.h"
#include "Pilot.h"

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


// --- 3. HTML INTERFACE ---
// This is the webpage stored as a text string
String getHtml() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: sans-serif; text-align: center; background-color: #222; color: #fff; user-select: none; }";
  html += ".btn { display: block; width: 90%; padding: 10px; margin: 10px auto; font-size: 24px; border: none; border-radius: 10px; cursor: pointer; font-weight: bold; }";
  html += ".arm { background-color: #4CAF50; color: white; }";
  html += ".disarm { background-color: #f44336; color: white; }";
  html += ".capture { background-color: rgba(203, 54, 244, 1); color: white; }";
  html += "input[type=range] { width: 90%; height: 50px; margin: 30px 0; }";
  html += "#snitch-view { width:320px; height:240px; border:2px solid #555; margin-top: 20px; background-color: #000; display: block; margin-left: auto; margin-right: auto; }";
  html += "</style></head><body>";
  
  html += "<h1>SNITCH CONTROL</h1>";
  html += "<h2 id='status' style='color:#4CAF50;'>STANDBY</h2>";
  html += "<h3 id='thrVal'>Throttle: 0%</h3>";

  // Throttle Slider
  html += "<input type='range' min='1000' max='2000' value='1000' id='slider' oninput='sendThrottle(this.value)'>";

  // Buttons
  html += "<button class='btn arm' onclick='setArm(true)'>ARM (Launch)</button>";
  html += "<button class='btn disarm' onclick='setArm(false)'>DISARM (Kill)</button>";
  html += "<button class='btn capture' id='capture-btn' onclick='toggleStream()'>TOGGLE STREAM</button>";
  // The Video Feed Display
  html += "<img id='snitch-view' src='' alt='Camera Standby'>";

  html += "<script>";
  html += "var streamImage = false;";

  // Function to start and stop the stream
  html += "function toggleStream() {";
  html += "  if (streamImage) {";
  html += "    streamImage = false;";
  html += "    var img = document.getElementById('snitch-view');";
  html += "    img.onload = null;"; 
  html += "    img.src = '';"; // Clear the feed"
  html += "  }";
  html += "  else { ";
  html += "   streamImage=true;";
  html += "   updateImage();";
  html += "  };";
  html += "};";

  // This function creates the 'Daisy Chain'
  html += "function updateImage() {";
  html += "  if (!streamImage) {"; 
  html += "    return }";
  html += "  var img = document.getElementById('snitch-view');";
  // The 'onload' event is the magic part. It waits until the image is 100% received.
  html += "  img.onload = function() {";
  html += "    if (streamImage) {";
  html += "      setTimeout(updateImage, 300);"; // Wait 50ms then ask for the next frame
  html += "    }";
  html += "  };";
  // If there's an error (like a timeout), try again after a short delay
  html += "  img.onerror = function() {";
  html += "    if (streamImage) setTimeout(updateImage, 500);";
  html += "  };";
  html += "  img.src = '/capture?t=' + Date.now();";
  html += "}";

  html += "function setArm(arm) {";
  // 1. UPDATE UI TEXT IMMEDIATELY (Before the network request)
  html += "  var statusText = document.getElementById('status');";
  html += "  statusText.innerText = arm ? 'ARMED!' : 'STANDBY';";
  html += "  statusText.style.color = arm ? '#f44336' : '#4CAF50';";
  // 2. SEND COMMAND TO ESP32
  html += "  fetch(arm ? '/arm' : '/disarm');";
  // 3. START/STOP CAMERA CHAIN
  html += "  if (!arm) {";
  html += "    document.getElementById('slider').value = 1000;";
  html += "    sendThrottle(1000);";
  html += "  }";
  html += "}";

  html += "function sendThrottle(val) {";
  html += "  document.getElementById('thrVal').innerText = 'Throttle: ' + Math.round((val-1000)/10) + '%';";
  html += "  fetch('/throttle?val=' + val);";
  html += "}";
  html += "</script></body></html>";
  
  return html;
}

// --- 4. WEB HANDLERS ---
// These functions run when your phone clicks a button

void handleRoot() {
  server.send(200, "text/html", getHtml());
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

void handleCapture() {
  
  // ... (sending code) ...
  camera_fb_t* fb = esp_camera_fb_get();
  esp_camera_fb_return(fb);
  fb = esp_camera_fb_get();
  
  if (!fb) {
    server.send(500, "text/plain", "Error capturing image.");
    return;
  }
  
  // BMP Header for 160x120 Grayscale (8-bit)
  // This is a "magic" 54-byte header + 1024-byte color table
  const uint8_t bmp_header[54] = {
    0x42, 0x4D, 0x36, 0x4F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x04, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x88, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x4B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  server.setContentLength(sizeof(bmp_header) + 1024 + fb->len);
  server.send(200, "image/bmp", "");
  
  WiFiClient client = server.client();

  // 1. Create the "Box" (1078 bytes)
  uint8_t fullHeader[1078];

  // 2. Put the bmp_header into the start of the box
  memcpy(fullHeader, bmp_header, 54);

  // 3. Fill the rest of the box with the Color Table
  for (int i = 0; i < 256; i++) {
    int idx = 54 + (i * 4);
    fullHeader[idx] = (uint8_t)i;     // Blue
    fullHeader[idx + 1] = (uint8_t)i; // Green
    fullHeader[idx + 2] = (uint8_t)i; // Red
    fullHeader[idx + 3] = 0;          // Reserved
  }
  
  // 4. Send the whole box in ONE SHOT
  client.write(fullHeader, 1078);
  
  
  // 3. CHUNKED SEND: Send pixels in smaller, digestible pieces
  // This prevents the WiFi stack from "choking" on large data
  size_t totalLen = fb->len;
  size_t offset = 0;
  size_t chunkSize = 4096; // Send 4KB at a time

  while (offset < totalLen) {
    size_t remaining = totalLen - offset;
    size_t toWrite = (remaining < chunkSize) ? remaining : chunkSize;
    client.write(fb->buf + offset, toWrite);
    offset += toWrite;
    // yield(); // Optional: uncomment if the web server feels "stuck"
  }

  esp_camera_fb_return(fb);
  client.stop();
  
}

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
  
  server.begin();

  // Success LED, on for 2 seconds
  digitalWrite(LED_BUILTIN, LOW);
  delay(2000);
  digitalWrite(LED_BUILTIN, HIGH);
}



// MAIN LOOP
void loop() {
  // put your main code here, to run repeatedly:
  

  server.handleClient();
  unsigned long start = millis();
  Threat threat = snitchEye.scanSky();
  Serial.printf("Send took: %u ms\n", millis() - start);
  snitchPilot.cameraMotion(threat);
  
  snitchPilot.update();
  

  //If wifi disconnects, cut power.
  if (WiFi.softAPgetStationNum() == 0) {
     snitchPilot.disarm();
  }
}
// put function definitions here:


