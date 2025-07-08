#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include "ESP32MQTTClient.h"
#include "localgps.h"
#include <vector>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // 0x27 is the I2C address, 16x2 LCD

PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);
volatile bool isNfcConnected = false;

ESP32MQTTClient mqttClient("angkot", "angkot12", "192.168.100.36", 1883, "6821ea7a1ea62f0c45c59f62");
ESP32GPS gps(16,17);

std::vector<String> daftarIdPenumpang;
int idxPenumpangDetected = -1, statusPenumpangNaik;
String uidDetected;

void setup() {
  Serial.begin(115200);
  Serial.println("-------Peer to Peer HCE--------");

  lcd.init();                      // Initialize the LCD
  lcd.backlight();                 // Turn on the backlight
  lcd.setCursor(0, 0);             
  lcd.print("Starting...");        // Show "Starting..." on the LCD
  lcd.setCursor(0, 1);             
  lcd.print("Broker");        // Show "Starting..." on the LCD

  mqttClient.begin();
  mqttClient.setCallback(processMqtt);

  delay(1000);

  lcd.clear();
  lcd.setCursor(0, 0);             
  lcd.print("Starting...");        // Show "Starting..." on the LCD
  lcd.setCursor(0, 1);             
  lcd.print("GPS");        // Show "Starting..." on the LCD
  gps.begin();
}

void loop() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cur: ");
  lcd.print(daftarIdPenumpang.size());
  lcd.setCursor(0, 1);
  lcd.print("Max: ");
  lcd.print(13);

  bool success;
  // Buffer to store the UID
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  // UID size (4 or 7 bytes depending on card type)
  uint8_t uidLength;

  while (!isNfcConnected) {
    isNfcConnected = connectNfc();
  }

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

  // If the card is detected, print the UID
  if (success)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Card Detected");
    lcd.setCursor(0, 1);
    lcd.print("Processing...");

    uidDetected = "";  // Create a string to store the UID

    Serial.println("Card Detected");
    Serial.print("Size of UID: "); Serial.print(uidLength, DEC);
    Serial.println(" bytes");

    // Convert UID to string
    for (uint8_t i = 0; i < uidLength; i++) {
      uidDetected += String(uid[i], HEX);  // Append each byte of UID as a hex string
    }

    // Print the UID string (optional)
    Serial.print("UID as String: ");
    Serial.println(uidDetected);

    processNfc();

    gps.update();
    delay(1000);
    isNfcConnected = connectNfc();
  }
  else
  {
    // PN532 probably timed out waiting for a card
    Serial.println("Timed out waiting for a card");
  }
}

bool connectNfc() {
  
  nfc.begin();

  // isNfcConnected, show version
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata)
  {
    Serial.println("PN53x card not found!");
    return false;
  }

  //port
  // Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  // Serial.print("Firmware version: "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  // Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(0xFF);

  // configure board to read RFID tags
  nfc.SAMConfig();

  Serial.println("Waiting for card (ISO14443A Mifare)...");
  Serial.println("");

  return true;
}

int cariPenumpang() {
  int i = 0;
  int frekDaftarIdPenumpang = daftarIdPenumpang.size();
  int status = 0;

  while(i < frekDaftarIdPenumpang && status == 0) {
    if (daftarIdPenumpang[i] == uidDetected) {
      status = 1;
    } else {
      i++;
    }
  }
  
  return (i < frekDaftarIdPenumpang) ? i : -1;
}

void processNfc() {
  gps.update();
  if (gps.locationAvailable()) {
    double lat = gps.getLatitude();
    double lon = gps.getLongitude();

    // save data to server
    char message[150];
    idxPenumpangDetected = cariPenumpang();
    statusPenumpangNaik = (idxPenumpangDetected == -1);
    snprintf(message, sizeof(message),
      "naik_turun_angkot,%s,%d,%.6f,%.6f", uidDetected, statusPenumpangNaik, lat, lon);

    mqttClient.publish(message);
    Serial.print("MQTT Message Sent: ");
    Serial.println(message);

    bool isReceived = mqttClient.loopUntilReceived();
    
    if(!isReceived) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Failed");
    }

    gps.update();
    delay(1000);
  } else {
    Serial.println("GPS not available.");
  }
}

void processMqtt(char* topic, byte* payload, unsigned int length) {
  // Convert payload to String
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Split the message by comma
  int startIndex = 0, idxPart = 0;
  int commaIndex;

  bool isOk = false;
  
  while ((commaIndex = message.indexOf(',', startIndex)) != -1) {
    String part = message.substring(startIndex, commaIndex); // Extract substring before comma
    Serial.print("Part: ");
    Serial.println(part); // Print each part
    startIndex = commaIndex + 1; // Move past the comma

    if(idxPart == 1) {
      isOk = (part == "OK");
    }
    idxPart++;
  }

  if(isOk) {
    Serial.print("statusPenumpangNaik :");
    Serial.println(statusPenumpangNaik);
    
    if(statusPenumpangNaik == 0) {
      daftarIdPenumpang.erase(daftarIdPenumpang.begin() + idxPenumpangDetected);
    } else {
      daftarIdPenumpang.push_back(uidDetected);
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Success");
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Failed");
  }
}