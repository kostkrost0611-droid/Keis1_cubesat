#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SERVO_HORIZ_PIN 9
#define SERVO_VERT_PIN 10
#define LASER_PIN 7

#define CE_PIN 3
#define CSN_PIN 8

LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo servoHoriz;
Servo servoVert;
RF24 radio(CE_PIN, CSN_PIN);

const byte address[6] = "00001";

struct SensorData {
  int horizontalAngle;
  int verticalAngle;
  float voltage;
  char status[12];
  bool laserState;
  uint8_t sequence;
};

SensorData data;
uint8_t sequenceNumber = 0;

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  
  servoHoriz.attach(SERVO_HORIZ_PIN);
  servoVert.attach(SERVO_VERT_PIN);
  
  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, LOW);
  
  displayLCD("Setting zero", "position...", 0);
  moveServos(0, 0);
  delay(1000);
  
  displayLCD("Waiting", "5 seconds...", 0);
  for(int i = 5; i > 0; i--) {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(i);
    lcd.print(" seconds");
    delay(1000);
  }
  
  if (!radio.begin()) {
    displayLCD("NRF24 ERROR!", "Check wiring!", 3000);
    while (1);
  }
  
  radio.openWritingPipe(address);
  radio.setChannel(76);
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_1MBPS);
  radio.setRetries(0, 0);
  radio.stopListening();
  
  radio.setPALevel(RF24_PA_MAX);
  
  displayLCD("Transmitter", "READY", 2000);
}

void loop() {
  setLaser(true);
  displayLCD("Scanning...", "Laser: ON", 100);
  
  displayLCD("Horizontal", "scan", 100);
  for (int vert = -40; vert <= 40; vert += 10) {
    moveServos(0, vert);
    transmitData(0, vert, "HORIZ", true);
    updateDisplay(0, vert, "HORIZ");
  }
  
  displayLCD("Vertical", "scan", 100);
  for (int horiz = -40; horiz <= 40; horiz += 10) {
    moveServos(horiz, 0);
    transmitData(horiz, 0, "VERT", true);
    updateDisplay(horiz, 0, "VERT");
  }
  
  displayLCD("Diagonal 1", "scan", 100);
  for (int angle = -40; angle <= 40; angle += 10) {
    moveServos(angle, angle);
    transmitData(angle, angle, "DIAG1", true);
    updateDisplay(angle, angle, "DIAG1");
  }
  
  displayLCD("Diagonal 2", "scan", 100);
  for (int i = -40; i <= 40; i += 10) {
    moveServos(i, -i);
    transmitData(i, -i, "DIAG2", true);
    updateDisplay(i, -i, "DIAG2");
  }
  
  moveServos(0, 0);
  transmitData(0, 0, "RESET", true);
  displayLCD("Return to", "zero position", 500);
  
  setLaser(false);
  displayLCD("Scan complete", "Laser: OFF", 500);
  
  displayLCD("Waiting", "5 seconds", 0);
  for(int i = 5; i > 0; i--) {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(i);
    lcd.print(" seconds");
    delay(1000);
  }
}

void setLaser(bool state) {
  digitalWrite(LASER_PIN, state ? HIGH : LOW);
}

void moveServos(int horizAngle, int vertAngle) {
  int horizUs = map(horizAngle, -90, 90, 500, 2500);
  int vertUs = map(vertAngle, -90, 90, 500, 2500);
  
  servoHoriz.writeMicroseconds(horizUs);
  servoVert.writeMicroseconds(vertUs);
  delay(50);
}

void transmitData(int horiz, int vert, const char* status, bool laserState) {
  sequenceNumber++;
  data.horizontalAngle = horiz;
  data.verticalAngle = vert;
  data.voltage = 5.0;
  data.laserState = laserState;
  data.sequence = sequenceNumber;
  
  strncpy(data.status, status, sizeof(data.status) - 1);
  data.status[sizeof(data.status) - 1] = '\0';
  
  radio.write(&data, sizeof(data));
  
  delay(10);
}

void updateDisplay(int horiz, int vert, const char* mode) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("H:");
  lcd.print(horiz);
  lcd.print((horiz >= 0) ? " " : "");
  lcd.print(" V:");
  lcd.print(vert);
  lcd.print((vert >= 0) ? " " : "");
  
  lcd.setCursor(0, 1);
  lcd.print(mode);
  lcd.print(" #");
  lcd.print(sequenceNumber);
  lcd.print(" L:ON");
  
  delay(200);
}

void displayLCD(const char* line1, const char* line2, int delayTime) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
  
  if(delayTime > 0) {
    delay(delayTime);
  }
}