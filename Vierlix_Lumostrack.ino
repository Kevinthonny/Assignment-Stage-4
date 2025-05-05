#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <ESP32Servo.h>

Adafruit_ADS1115 ads;

Servo servoX;
Servo servoY;

const int servoXPin = 18;
const int servoYPin = 19;

int posX = 90;
int posY = 90;

const int minServo = 0;
const int maxServo = 180;

const int threshold = 100;

int prevVerticalDiff = 0;
int prevHorizontalDiff = 0;

int verticalTrend = 0;
int horizontalTrend = 0;

unsigned long lastMoveTime = 0;
const unsigned long predictionInterval = 5000;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!ads.begin()) {
    Serial.println("Gagal koneksi ADS1115.");
    while (1);
  }

  servoX.setPeriodHertz(50);
  servoY.setPeriodHertz(50);
  servoX.attach(servoXPin, 500, 2400);
  servoY.attach(servoYPin, 500, 2400);

  servoX.write(posX);
  servoY.write(posY);

  Serial.println("LumosTrack ESP32 AI Siap");
}

void loop() {
  int LDR_TL = ads.readADC_SingleEnded(0);
  int LDR_TR = ads.readADC_SingleEnded(1);
  int LDR_BL = ads.readADC_SingleEnded(2);
  int LDR_BR = ads.readADC_SingleEnded(3);

  int avgTop = (LDR_TL + LDR_TR) / 2;
  int avgBottom = (LDR_BL + LDR_BR) / 2;
  int avgLeft = (LDR_TL + LDR_BL) / 2;
  int avgRight = (LDR_TR + LDR_BR) / 2;

  int verticalDiff = avgTop - avgBottom;
  int horizontalDiff = avgLeft - avgRight;

  Serial.printf("Top: %d | Bottom: %d | Left: %d | Right: %d\n", avgTop, avgBottom, avgLeft, avgRight);

  bool moved = false;

  if (abs(verticalDiff) > threshold) {
    verticalTrend = verticalDiff - prevVerticalDiff;
    prevVerticalDiff = verticalDiff;
    if (verticalDiff > 0 && posY < maxServo) posY++;
    else if (verticalDiff < 0 && posY > minServo) posY--;
    servoY.write(posY);
    moved = true;
  }

  if (abs(horizontalDiff) > threshold) {
    horizontalTrend = horizontalDiff - prevHorizontalDiff;
    prevHorizontalDiff = horizontalDiff;
    if (horizontalDiff > 0 && posX < maxServo) posX++;
    else if (horizontalDiff < 0 && posX > minServo) posX--;
    servoX.write(posX);
    moved = true;
  }

  if (moved) {
    lastMoveTime = millis();
  }

  if (!moved && (millis() - lastMoveTime > predictionInterval)) {
    if (horizontalTrend > 0 && posX < maxServo) posX++;
    else if (horizontalTrend < 0 && posX > minServo) posX--;
    if (verticalTrend > 0 && posY < maxServo) posY++;
    else if (verticalTrend < 0 && posY > minServo) posY--;
    servoX.write(posX);
    servoY.write(posY);
    Serial.println("Gerakan prediktif (AI)");
  }

  delay(100);
}
