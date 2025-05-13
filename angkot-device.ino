#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include "ESP32MQTTClient.h"
#include "localgps.h"
#include <vector>

PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

ESP32MQTTClient mqttClient("angkot", "angkot12", "192.168.100.36", 1883, "6821ea7a1ea62f0c45c59f62");
ESP32GPS gps(16,17);

std::vector<String> daftarIdPenumpang;
int idxPenumpangDetected = -1, statusPenumpangNaik;
String idDetected;

void setup() {
  Serial.begin(115200);
  Serial.println("-------Peer to Peer HCE--------");

  mqttClient.begin();
  mqttClient.setCallback(processMqtt);
  gps.begin();
  nfc.begin();
  
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  //nfc.setPassiveActivationRetries(0xFF);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
}

void loop() {
  bool success;
  uint8_t responseLength = 32;

  Serial.println("Waiting for an ISO14443A card");

  success = nfc.inListPassiveTarget();

  if (success) {
    Serial.println("Found something!");

    // SELECT AID to initiate communication with Android HCE app
    uint8_t selectApdu[] = {
      0x00, 0xA4, 0x04, 0x00,
      0x07, 0xA0, 0x00, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA,
      0x00
    };

    uint8_t response[32];
    success = nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);

    if (success) {
      Serial.println("SELECT AID successful");

      // Now send "Hello" APDU and expect an ID or some response from HCE app
      uint8_t apdu[] = "Hello from Arduino";
      uint8_t back[32];
      uint8_t length = 32;

      success = nfc.inDataExchange(apdu, sizeof(apdu), back, &length);
      if (success) {
        Serial.print("APDU response: ");
        for (int i = 0; i < length; i++) {
          Serial.write(back[i]); // Print raw data
        }
        Serial.println();

        // Convert response to string (assume ASCII or UTF-8 ID)
        char idStr[33];
        memcpy(idStr, back, length);
        idStr[length] = '\0';

        idDetected = "RFID000001";

        processNfc();

        delay(1000); // optional pause
      } else {
        Serial.println("Failed to get data from APDU.");
      }

    } else {
      Serial.println("Failed SELECT AID.");
    }

  } else {
    Serial.println("No card found.");
  }

  delay(1000);
}

int cariPenumpang() {
  int i = 0;
  int frekDaftarIdPenumpang = daftarIdPenumpang.size();
  int status = 0;

  while(i < frekDaftarIdPenumpang && status == 0) {
    if (daftarIdPenumpang[i] == idDetected) {
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
      "naik_turun_angkot,%s,%d,%.6f,%.6f", idDetected, statusPenumpangNaik, lat, lon);

    mqttClient.publish(message);
    Serial.print("MQTT Message Sent: ");
    Serial.println(message);
    
    mqttClient.loopUntilReceived();
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
      daftarIdPenumpang.push_back(idDetected);
    }
  }
  // idxPenumpangDetected = -1;
  // idDetected = "";
}