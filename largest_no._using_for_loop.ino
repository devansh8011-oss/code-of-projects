#include <WiFi.h>
#include <time.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// ===== CONFIG =====
const char* ssid     = "Tower-2 801";
const char* password = "rahul757  ";

// NTP — IST
const char* ntpServer      = "pool.ntp.org";
const long  gmtOffset      = 19800;
const int   daylightOffset = 0;

// MAX7219 config
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW   // most common 4-in-1 type
#define MAX_DEVICES   4
#define CS_PIN        7    // D7
#define CLK_PIN       8    // D8
#define DIN_PIN       10   // D10

MD_Parola display = MD_Parola(HARDWARE_TYPE, DIN_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

void setup() {
  Serial.begin(115200);

  // Init display
  display.begin();
  display.setIntensity(5);       // brightness 0-15
  display.displayClear();
  display.setTextAlignment(PA_CENTER);
  display.print("WAIT..");

  // Connect WiFi
  Serial.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  // Sync NTP
  configTime(gmtOffset, daylightOffset, ntpServer);
  struct tm timeinfo;
  Serial.print("Syncing time");
  while (!getLocalTime(&timeinfo)) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nSynced!");
  display.print("SYNC!");
  delay(1000);
}

void loop() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    display.print("ERR");
    delay(1000);
    return;
  }

  // Format: HH:MM:SS — colon blinks every second
  char timeBuf[10];
  if (timeinfo.tm_sec % 2 == 0) {
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &timeinfo);
  } else {
    strftime(timeBuf, sizeof(timeBuf), "%H %M %S", &timeinfo);  // colon off
  }

  display.setTextAlignment(PA_CENTER);
  display.print(timeBuf);

  Serial.println(timeBuf);
  delay(500);  // update every 500ms for smooth blink
}