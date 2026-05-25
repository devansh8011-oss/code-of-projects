#include <Wire.h>
#include "Adafruit_TCS34725.h"

Adafruit_TCS34725 tcs = Adafruit_TCS34725(
  TCS34725_INTEGRATIONTIME_50MS,
  TCS34725_GAIN_1X
);

// ======================================
// RAWC ZONES
//
//  0 ── 20 ── 78 ── 79 ── 109 ── 110 ── ∞
//  BLACK  UNKNOWN  SILVER        WHITE
//
//  Open air rawC = 75  → falls in UNKNOWN ✓
//  SILVER   rawC = 83  → falls in SILVER  ✓
//  WHITE    rawC = 127 → falls in WHITE   ✓
//  BLACK    rawC = 13  → falls in BLACK   ✓
// ======================================

#define BLACK_RAWC_MAX   20
#define SILVER_RAWC_MIN  79    // midpoint of (75 + 83) = 79
#define SILVER_RAWC_MAX  109
#define WHITE_RAWC_MIN   110

#define BLUE_B_OVER_R    30
#define BLUE_B_OVER_G    15

// ======================================
// MOVING AVERAGE — 5 SAMPLES
// ======================================

#define SAMPLES 5

uint16_t rBuf[SAMPLES] = {0};
uint16_t gBuf[SAMPLES] = {0};
uint16_t bBuf[SAMPLES] = {0};
uint16_t cBuf[SAMPLES] = {0};
int bufIndex    = 0;
int sampleCount = 0;

void pushSample(uint16_t r, uint16_t g, uint16_t b, uint16_t c) {
  rBuf[bufIndex] = r;
  gBuf[bufIndex] = g;
  bBuf[bufIndex] = b;
  cBuf[bufIndex] = c;
  bufIndex = (bufIndex + 1) % SAMPLES;
  if (sampleCount < SAMPLES) sampleCount++;
}

void getAveraged(uint16_t &r, uint16_t &g, uint16_t &b, uint16_t &c) {
  uint32_t sR = 0, sG = 0, sB = 0, sC = 0;
  for (int i = 0; i < sampleCount; i++) {
    sR += rBuf[i]; sG += gBuf[i];
    sB += bBuf[i]; sC += cBuf[i];
  }
  r = sR / sampleCount;
  g = sG / sampleCount;
  b = sB / sampleCount;
  c = sC / sampleCount;
}

// ======================================
// COLOUR DETECTION
// ======================================

String detectColor(int r, int g, int b, uint16_t rawC) {

  // BLACK
  if (rawC <= BLACK_RAWC_MAX) {
    return "BLACK";
  }

  // BLUE — ratio based, rawC level irrelevant
  if ((b - r) >= BLUE_B_OVER_R &&
      (b - g) >= BLUE_B_OVER_G) {
    return "BLUE";
  }

  // WHITE — highest rawC zone
  if (rawC >= WHITE_RAWC_MIN) {
    return "WHITE";
  }

  // SILVER — rawC 79 to 109
  // open air rawC 75 sits just below 79 → UNKNOWN
  // silver rawC 83 sits just above 79 → SILVER
  if (rawC >= SILVER_RAWC_MIN &&
      rawC <= SILVER_RAWC_MAX) {
    return "SILVER";
  }

  // open air or unrecognised
  return "UNKNOWN";
}

// ======================================

void setup() {
  Serial.begin(9600);

  if (!tcs.begin()) {
    Serial.println("ERROR: TCS34725 not found. Check wiring.");
    while (1);
  }

  Serial.println("================================");
  Serial.println("TCS34725 Ready");
  Serial.println("Detecting: BLACK | SILVER | BLUE | WHITE");
  Serial.println("================================");
}

// ======================================

void loop() {

  uint16_t rawR, rawG, rawB, rawC;
  tcs.getRawData(&rawR, &rawG, &rawB, &rawC);

  pushSample(rawR, rawG, rawB, rawC);

  uint16_t avgR, avgG, avgB, avgC;
  getAveraged(avgR, avgG, avgB, avgC);

  int r = 0, g = 0, b = 0;
  if (avgC != 0) {
    r = constrain((int)(((float)avgR / avgC) * 255.0), 0, 255);
    g = constrain((int)(((float)avgG / avgC) * 255.0), 0, 255);
    b = constrain((int)(((float)avgB / avgC) * 255.0), 0, 255);
  }

  String detected = detectColor(r, g, b, avgC);

  Serial.println("================================");
  Serial.print("R: ");      Serial.print(r);
  Serial.print("  G: ");    Serial.print(g);
  Serial.print("  B: ");    Serial.print(b);
  Serial.print("  rawC: "); Serial.println(avgC);
  Serial.print(">> ");      Serial.println(detected);
  Serial.println("================================");

  delay(200);
}