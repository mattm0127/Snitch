#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "Camera.h"
#include "Pilot.h"

// --- 1. WIFI AP CONFIGURATION ---
const char* ssid = "Snitch_Drone";
const char* password = "goldensnitch";


// --- PINS ---
// Check your specific board pinout!
#define RX_PIN D7  // Connect to FC TX
#define TX_PIN D6  // Connect to FC RX


// GLOBAL VARIABLES
WebServer server(80);
Camera snitchEye;
//Pilot snitchPilot;


// --- 3. HTML INTERFACE ---
// This is the webpage stored as a text string
String getHtml() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: sans-serif; text-align: center; background-color: #222; color: #fff; user-select: none; }";
  html += ".btn { display: block; width: 90%; padding: 20px; margin: 10px auto; font-size: 24px; border: none; border-radius: 10px; }";
  html += ".arm { background-color: #4CAF50; color: white; }";
  html += ".disarm { background-color: #f44336; color: white; }";
  html += "input[type=range] { width: 90%; height: 50px; margin: 30px 0; }";
  html += "</style></head><body>";
  
  html += "<h1>SNITCH CONTROL</h1>";
  html += "<h2 id='status'>STANDBY</h2>";
  html += "<h3 id='thrVal'>Throttle: 0%</h3>";

  // The Slider (sends values 1000-2000)
  html += "<input type='range' min='1000' max='2000' value='1000' id='slider' oninput='sendThrottle(this.value)'>";

  // The Buttons
  html += "<button class='btn arm' onclick='setArm(true)'>ARM (Launch)</button>";
  html += "<button class='btn disarm' onclick='setArm(false)'>DISARM (Kill)</button>";

  // The JavaScript (Runs on your phone)
  html += "<script>";
  html += "function sendThrottle(val) {";
  html += "  document.getElementById('thrVal').innerText = 'Throttle: ' + Math.round((val-1000)/10) + '%';";
  html += "  fetch('/throttle?val=' + val);"; // Sends "GET /throttle?val=1500"
  html += "}";
  html += "function setArm(arm) {";
  html += "  fetch(arm ? '/arm' : '/disarm');";
  html += "  document.getElementById('status').innerText = arm ? 'ARMED!' : 'STANDBY';";
  html += "  document.getElementById('status').style.color = arm ? 'red' : 'green';";
  html += "  if(!arm) { document.getElementById('slider').value = 1000; sendThrottle(1000); }";
  html += "}";
  html += "</script>";
  
  html += "</body></html>";
  return html;
}

// --- 4. WEB HANDLERS ---
// These functions run when your phone clicks a button

void handleRoot() {
  server.send(200, "text/html", getHtml());
}

void handleArm() {
  //snitchPilot.arm(); // Call the Pilot Class!
  server.send(200, "text/plain", "ARMED");
}

void handleDisarm() {
  //snitchPilot.disarm(); // Call the Pilot Class!
  server.send(200, "text/plain", "DISARMED");
}

void handleThrottle() {
  if (server.hasArg("val")) {
    int val = server.arg("val").toInt();
    //snitchPilot.setThrottle(val); // Update the Pilot Class
  }
  server.send(200, "text/plain", "OK");
}


// SETUP FUNCTION
void setup() {
  // put your setup code here, to run once:
  Serial.begin(20000000);

  // Flash the LED if the camera does not initialize properly.
  if (!snitchEye.init()) {
    pinMode(LED_BUILTIN, OUTPUT);
    while(1){
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(100);
    }
  }

  // Calibrate camera 
  delay(1000);
  snitchEye.calibrate();

  // Initialize Flight Controller
  //snitchPilot.begin(RX_PIN, TX_PIN);

  // B. Initialize WiFi
  WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // C. Configure Web Server Routes
  server.on("/", handleRoot);        // Load the page
  server.on("/arm", handleArm);      // Click "Arm"
  server.on("/disarm", handleDisarm);// Click "Disarm"
  server.on("/throttle", handleThrottle); // Move Slider
  
  server.begin();
}



// MAIN LOOP
void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  //snitchPilot.update();

  // If wifi disconnects, cut power.
  if (WiFi.softAPgetStationNum() == 0) {
     //snitchPilot.disarm();
  }
}
// put function definitions here:
