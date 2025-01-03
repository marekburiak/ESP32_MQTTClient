# ESP32_MQTTClient

MQTT client for Arduino ESP32.

- based on ESP32MQTTClient from cyijun: https://github.com/cyijun/ESP32MQTTClient
- it's a wrapper class for a thread-safe ESP-IDF mqtt-client: https://docs.espressif.com/projects/esp-idf/en/v5.3.2/esp32/api-reference/protocols/mqtt.html

### Version History
```
v.1.0.0 - initial release, developed on arduino-esp32 v3.1.0 (which is based on ESP-IDF v5.3.2)
```

### Usage

```c++
#include "Arduino.h"
#include <WiFi.h>
#include <Network.h>
#include <mqtt_client.h>
#include <ESP32_MQTTClient.h>

const char* wifiSSID = "your-wifi-ssid";
const char* wifiPass = "your-wifi-pwd";

const char* mqttBrokerUri = "mqtt://broker.hivemq.com:1883";
const char* testTopic = "ESP32_MQTTClient/testTopic";

ESP32_MQTTClient _mqttClient;

void setup()
{
	Serial.begin(115200);

	_mqttClient.setBrokerUri(mqttBrokerUri);
	_mqttClient.onMqttConnected(onMqttConnected);
	_mqttClient.onMqttMessageReceived(onMqttMessageReceived);

	Serial.println("Connecting to WiFi...");
	WiFi.onEvent(onWiFiEvent);
	WiFi.begin(wifiSSID, wifiPass);
}

void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
	if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP)
	{
		Serial.println("WiFi connected");

		Serial.println("Connecting to MQTT broker...");
		_mqttClient.start();
	}
}

void onMqttConnected(int sessionPresent) 
{
	Serial.println("MQTT broker connected");

	// subscribe to testTopic
	Serial.println("Subscribing to testTopic");
	_mqttClient.subscribe(testTopic);
	
	// publish to testTopic
	Serial.println("Publishing to testTopic");
	_mqttClient.publish(testTopic, "hello");
}

void onMqttMessageReceived(int msgId, char* topic, int topicLen, char* data, int dataLen, int currentDataOffset, int totalDataLen, bool retain, int qos, bool dup) 
{
	Serial.print("MQTT message received, topic: ");
	Serial.write(topic, topicLen);	// topic is not terminated by a null char!
	Serial.print(", data: ");
	Serial.write(data, dataLen);	// data is not terminated by a null char!
}

void loop()
{
}
```

Output:
```
Connecting to WiFi...
WiFi connected
Connecting to MQTT broker...
MQTT broker connected
Subscribing to testTopic
Publishing to testTopic
MQTT message received, topic: ESP32_MQTTClient/testTopic, data: hello
```
