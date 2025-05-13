#ifndef ESP32GPS_h
#define ESP32GPS_h

#include <TinyGPS++.h>
#include <HardwareSerial.h>

class ESP32GPS {
  public:
    ESP32GPS(int rxPin, int txPin, uint32_t baud = 9600);
    void begin();
    void update();
    bool locationAvailable();
    double getLatitude();
    double getLongitude();
    
  private:
    TinyGPSPlus _gps;
    HardwareSerial _serial;
    int _rxPin, _txPin;
    uint32_t _baud;
};

#endif
