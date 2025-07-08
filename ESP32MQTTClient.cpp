#include "ESP32MQTTClient.h"

ESP32MQTTClient::ESP32MQTTClient(const char* ssid, const char* password, const char* mqtt_server, int mqtt_port, const char* client_id)
  : _ssid(ssid), _password(password), _mqtt_server(mqtt_server), _mqtt_port(mqtt_port), _client_id(client_id), _client(_espClient), _lastMsg(0) {
  snprintf(_requestTopic, sizeof(_requestTopic), "device/AK/%s/request", client_id);
  snprintf(_responseTopic, sizeof(_responseTopic), "device/AK/%s/response", client_id);
}

void ESP32MQTTClient::begin() {
  connectWiFi();
  _client.setServer(_mqtt_server, _mqtt_port);
  connectMQTT();
}

void ESP32MQTTClient::connectWiFi() {
  WiFi.begin(_ssid, _password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void ESP32MQTTClient::connectMQTT() {
  while (!_client.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (_client.connect(_client_id)) {
      Serial.println("connected");
      _client.subscribe(_responseTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(_client.state());  // Print reason code
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void ESP32MQTTClient::loop() {
  if (!_client.connected()) {
    connectMQTT();
  }
  _client.loop();

  if (millis() - _lastMsg > 5000) {
    _lastMsg = millis();
  }
}

bool ESP32MQTTClient::loopUntilReceived() {
  _messageReceived = false;

  // Ensure MQTT is connected
  while (!_client.connected()) {
    connectMQTT();
  }

  // Wait until a message is received or timeout occurs
  Serial.println("loop until new message");

  unsigned long startTime = millis();
  const unsigned long timeout = 5000;  // 5 seconds

  while (!_messageReceived && (millis() - startTime < timeout)) {
    _client.loop();
    delay(10);  // Slight delay to avoid hogging CPU
  }

  if (_messageReceived) {
    Serial.println("new message is received");
  } else {
    Serial.println("timeout waiting for message");
  }

  return _messageReceived;
}

void ESP32MQTTClient::publish(const char* message) {
  unsigned long timeoutMs = 5000;
  unsigned long start = millis();
  bool isSuccess = false;

  while (millis() - start < timeoutMs) {
    if (!_client.connected()) {
      connectMQTT(); // Ensure we are connected
    }

    Serial.println(_requestTopic);

    isSuccess = _client.publish(_requestTopic, message);
    if (isSuccess) {
      Serial.println("MQTT publish success");
      return;
    }

    delay(100); // Small wait before retry
  }

  Serial.println("MQTT publish failed: timeout");
}

void ESP32MQTTClient::setCallback(MQTT_CALLBACK_SIGNATURE) {
  Serial.println("mqtt callback is generated");
  _client.setCallback([this, callback](char* topic, byte* payload, unsigned int length) {
    this->_handleIncomingMessage(callback, topic, payload, length);
  });
}

void ESP32MQTTClient::_handleIncomingMessage(MQTT_CALLBACK_SIGNATURE, char* topic, byte* payload, unsigned int length) {
  Serial.println("received message");
  _messageReceived = true;

  // Call the user's callback passed from the setCallback function
  if (callback) {
    callback(topic, payload, length);
  }
}
