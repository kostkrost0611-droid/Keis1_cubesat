#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define CE_PIN 9
#define CSN_PIN 10

const byte address[6] = "00001";

LiquidCrystal_I2C lcd(0x27, 16, 2);

RF24 radio(CE_PIN, CSN_PIN);

struct SensorData {
  int horizontalAngle;
  int verticalAngle;
  float voltage;
  char status[12];
  bool laserState;
  uint8_t sequence;
};

SensorData data;
unsigned long lastPacketTime = 0;
uint32_t packetCount = 0;
uint8_t lastSequence = 0;

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print("RF Scanner RX");
  lcd.setCursor(0, 1);
  lcd.print("Channel: 76");
  delay(2000);
  
  if (!radio.begin()) {
    displayError("NRF24 ERROR", "Check wiring!");
    while (1);
  }
  
  radio.openReadingPipe(0, address);
  radio.setChannel(76);
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_1MBPS);
  radio.setAutoAck(false);
  radio.setCRCLength(RF24_CRC_16);
  radio.startListening();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Receiver READY");
  lcd.setCursor(0, 1);
  lcd.print("Waiting data...");
  
  lastPacketTime = millis();
}

void loop() {
  if (radio.available()) {
    radio.read(&data, sizeof(data));
    
    packetCount++;
    lastPacketTime = millis();
    
    uint8_t seqDiff = 0;
    if (lastSequence > 0 && data.sequence > lastSequence) {
      seqDiff = data.sequence - lastSequence - 1;
    }
    lastSequence = data.sequence;
    
    displayData(seqDiff);
  }
  
  if (millis() - lastPacketTime > 3000) {
    displayNoSignal();
  }
}

void displayData(uint8_t lostPackets) {
  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print("H:");
  lcd.print(data.horizontalAngle);
  if (data.horizontalAngle >= 0) lcd.print(" ");
  
  lcd.setCursor(7, 0);
  lcd.print("V:");
  lcd.print(data.verticalAngle);
  if (data.verticalAngle >= 0) lcd.print(" ");
  
  lcd.setCursor(13, 0);
  lcd.print("#");
  lcd.print(data.sequence);
  
  lcd.setCursor(0, 1);
  lcd.print(data.status);
  
  lcd.setCursor(7, 1);
  lcd.print("L:");
  lcd.print(data.laserState ? "ON " : "OFF");
  
  if (lostPackets > 0) {
    lcd.setCursor(12, 1);
    lcd.print("-");
    lcd.print(lostPackets);
  }
  
  if (packetCount % 10 == 0) {
    delay(500);
    showStats();
  }
}

void showStats() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Packets: ");
  lcd.print(packetCount);
  
  lcd.setCursor(0, 1);
  lcd.print("Voltage: ");
  lcd.print(data.voltage, 1);
  lcd.print("V");
  
  delay(1000);
}

void displayNoSignal() {
  lcd.clear();
  
  static uint8_t blinkState = 0;
  blinkState = !blinkState;
  
  if (blinkState) {
    lcd.setCursor(0, 0);
    lcd.print("NO SIGNAL");
    lcd.setCursor(0, 1);
    lcd.print("Channel 76");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Packets: ");
    lcd.print(packetCount);
    lcd.setCursor(0, 1);
    lcd.print("Last: #");
    lcd.print(lastSequence);
  }
}

void displayError(const char* line1, const char* line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}