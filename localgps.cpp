#include "localgps.h"

ESP32GPS::ESP32GPS(int rxPin, int txPin, uint32_t baud)
  : _serial(1), _rxPin(rxPin), _txPin(txPin), _baud(baud) {}

void ESP32GPS::begin() {
  _serial.begin(_baud, SERIAL_8N1, _rxPin, _txPin);
  Serial.println("Waiting for GPS fix...");

  while (!_gps.location.isValid()) {
    while (_serial.available()) {
      _gps.encode(_serial.read());
    }
    delay(100); // Small delay to avoid tight loop, optional
  }

  Serial.println("GPS fix acquired.");
}

void ESP32GPS::update() {
  while (_serial.available() > 0) {
    _gps.encode(_serial.read());
  }
}

bool ESP32GPS::locationAvailable() {
  return _gps.location.isValid() && _gps.location.age() < 2000;
}

double ESP32GPS::getLatitude() {
  return _gps.location.lat();
}

double ESP32GPS::getLongitude() {
  return _gps.location.lng();
}
