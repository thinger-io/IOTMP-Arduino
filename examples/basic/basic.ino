// Basic IOTMP Arduino Example
//
// This example shows how to connect an ESP32 to Thinger.io
// using the IOTMP protocol with the shared embedded core.

// TODO: Replace with platform-specific include once ported
// #include <ThingerESP32.h>
#include <ThingerClient.h>

// WiFi and Thinger.io credentials
#define WIFI_SSID       "your_wifi"
#define WIFI_PASSWORD   "your_password"
#define THINGER_USER    "your_username"
#define THINGER_DEVICE  "your_device"
#define THINGER_CRED    "your_credential"

// TODO: Full implementation once ThingerClient is ported

void setup() {
    Serial.begin(115200);
    Serial.println("IOTMP Arduino - TODO: implementation pending");
}

void loop() {
    // thing.handle();
    delay(1000);
}
