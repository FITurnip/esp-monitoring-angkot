#ifndef ESP32MQTTClient_h
#define ESP32MQTTClient_h

#include <WiFi.h>
#include <PubSubClient.h>

class ESP32MQTTClient {
  public:
    ESP32MQTTClient(const char* ssid, const char* password, const char* mqtt_server, int mqtt_port, const char* client_id);
    void begin();
    void loop();
    void publish(const char* message);
    void setCallback(MQTT_CALLBACK_SIGNATURE);
    bool loopUntilReceived();
    
  private:
    void connectWiFi();
    void connectMQTT();

    const char* _ssid;
    const char* _password;
    const char* _mqtt_server;
    int _mqtt_port;
    const char* _client_id;
    char _requestTopic[50];
    char _responseTopic[50];

    WiFiClient _espClient;
    PubSubClient _client;
    unsigned long _lastMsg;

    bool _messageReceived;

    void _handleIncomingMessage(MQTT_CALLBACK_SIGNATURE, char* topic, byte* payload, unsigned int length);
};

#endif
