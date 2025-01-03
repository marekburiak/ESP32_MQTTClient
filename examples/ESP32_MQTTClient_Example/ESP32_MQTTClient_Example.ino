/*
	Name:       ESP32_MQTTClient_Example.ino
	Created:	1/1/2025 6:43:43 PM
	Author:     Marek Buriak
	Description: In this example, ESP32 connects to wi-fi and then to MQTT broker.
				 It then publishes to lastWillTopic, subscribes to testTopic and publishes to testTopic.
				 That triggers onMqttMessageReceived handler. After a few seconds it unsubscribes from testTopic,
				 stops the client and disconnects from broker.
				 There are other event handlers triggered during the above flow. The expected Serial output 
				 is at the bottom of this file.
*/

#include "Arduino.h"
#include <WiFi.h>
#include <Network.h>
#include <mqtt_client.h>
#include <ESP32_MQTTClient.h>

const char* wifiSSID = "your-wifi-ssid";
const char* wifiPass = "your-wifi-pwd";

const char* mqttBrokerUri = "mqtt://broker.hivemq.com:1883";
//const char* mqttUser = "your-mqtt-username";
//const char* mqttPass = "your-mqtt-pwd";
const int qos = 1;	// publish and subscribe on QOS 1 so onMqttMessagePublishConfirmed is triggered and msgIds are created
const char* lastWillTopic = "ESP32_MQTTClient/status";
const char* testTopic = "ESP32_MQTTClient/testTopic";

bool executeLoop = false;

ESP32_MQTTClient _mqttClient;

void setup()
{
	Serial.begin(115200);

	// set params for MQTT connection
	_mqttClient.setBrokerUri(mqttBrokerUri);
	//_mqttClient.setCredentials(mqttUser, mqttPass);
	//_mqttClient.setKeepAlive(15);

	// set last will message to indicate that the esp is offline
	_mqttClient.setLastWillMessage(lastWillTopic, "offline", qos, true);

	// register MQTT event handlers
	_mqttClient.onMqttBeforeConnect(onMqttBeforeConnect);
	_mqttClient.onMqttConnected(onMqttConnected);
	_mqttClient.onMqttDisconnected(onMqttDisconnected);
	_mqttClient.onMqttTopicSubscribed(onMqttTopicSubscribed);
	_mqttClient.onMqttTopicUnsubscribed(onMqttTopicUnsubscribed);
	_mqttClient.onMqttMessageReceived(onMqttMessageReceived);
	_mqttClient.onMqttMessagePublishConfirmed(onMqttMessagePublishConfirmed);
	_mqttClient.onMqttError(onMqttError);

	// start wifi
	WiFi.onEvent(onWiFiEvent);
	WiFi.begin(wifiSSID, wifiPass);
}

// when wifi is connected, start MQTT client
void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
	switch (event) {
	case ARDUINO_EVENT_WIFI_STA_GOT_IP:
	{
		Serial.print("--- WiFi connected, IP address: ");
		Serial.println(WiFi.localIP());

		// wifi is connected, start MQTT client and connect to broker
		bool success = _mqttClient.start();

		if (success)
			Serial.println("--- MQTT Client start successful");
		else
			Serial.println("--- MQTT Client start failed");
		break;
	}
	case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
	{
		Serial.println("--- WiFi disconnected");
		break;
	}
	}
}

void onMqttBeforeConnect() {
	Serial.println("--- onMqttBeforeConnect");
	Serial.print("------ Connecting to MQTT broker: ");
	Serial.print(_mqttClient.getURI());
}

void onMqttConnected(int sessionPresent) {
	Serial.println("--- onMqttConnected");
	Serial.println("------ MQTT broker connected");

	// after connect, publish to lastWillTopic to indicate that esp is online
	Serial.print("------ Publishing to topic ");
	Serial.print(lastWillTopic);
	Serial.print(", qos: ");
	Serial.print(qos);
	Serial.print(", msgId: ");
	int msgId = _mqttClient.publish(lastWillTopic, "online", qos, true);
	Serial.println(msgId);

	// subscribe to testTopic
	Serial.print("------ Subscribing to topic ");
	Serial.print(testTopic);
	Serial.print(", qos: ");
	Serial.print(qos);
	Serial.print(", msgId: ");
	int msgId2 = _mqttClient.subscribe(testTopic, qos);
	Serial.println(msgId2);

	// publish to testTopic
	Serial.print("------ Publishing to topic ");
	Serial.print(testTopic);
	Serial.print(", qos: ");
	Serial.print(qos);
	Serial.print(", msgId: ");
	int msgId3 = _mqttClient.publish(testTopic, "hello", qos);
	Serial.println(msgId3);

	executeLoop = true;	// start what's in loop()
}

void onMqttDisconnected() {
	Serial.println("--- onMqttDisconnected");
	Serial.println("------ MQTT broker disconnected");
}

void onMqttTopicSubscribed(int msgId, esp_mqtt_error_type_t errorType, char* data, int dataLen) {
	Serial.println("--- onMqttTopicSubscribed");
	Serial.print("------ MQTT topic subscribed, msgId: ");
	Serial.println(msgId);
}

void onMqttTopicUnsubscribed(int msgId) {
	Serial.println("--- onMqttTopicUnsubscribed");
	Serial.print("------ MQTT topic unsubscribed, msgId: ");
	Serial.println(msgId);
}

void onMqttMessagePublishConfirmed(int msgId) {
	Serial.println("--- onMqttMessagePublishConfirmed");
	Serial.print("------ MQTT message publish confirmed by broker, msgId: ");
	Serial.println(msgId);
}

void onMqttError(esp_mqtt_error_codes_t* error) {
	Serial.println("--- onMqttError");
	Serial.println("------ MQTT error");
	_mqttClient.printError(error);
}

void onMqttMessageReceived(int msgId, char* topic, int topicLen, char* data, int dataLen, int currentDataOffset, int totalDataLen, bool retain, int qos, bool dup) {
	Serial.println("--- onMqttMessageReceived");
	Serial.print("------ MQTT message received, msgId: ");
	Serial.print(msgId);
	Serial.print(", topic: ");
	Serial.write(topic, topicLen);	// topic is not terminated by a null char!
	Serial.print(", data: ");
	Serial.write(data, dataLen);	// data is not terminated by a null char!
	Serial.print(", qos: ");
	Serial.println(qos);
}

void loop()
{
	if (executeLoop)
	{
		delay(5000);

		// unsubscribe from testTopic
		Serial.print("--- Unsubscribing from ");
		Serial.print(testTopic);
		Serial.print(", msgId: ");
		int msgId4 = _mqttClient.unsubscribe(testTopic);
		Serial.println(msgId4);

		delay(2000);

		Serial.println();
		Serial.print("--- If you power off your ESP now, it will trigger the last will by the broker after at least ");
		Serial.print(_mqttClient.getKeepAliveSeconds() * 1.5);
		Serial.println(" seconds");
		Serial.println();

		// wait 10 seconds before stopping the client and disconnecting from broker
		delay(10000);

		// stop MQTT client (otherwise it would try to reconnect to broker when disconnected below)
		Serial.println("--- Stopping MQTT Client");
		bool success = _mqttClient.stop();
		if (success)
			Serial.println("------ MQTT Client stop successful");
		else
			Serial.println("------ MQTT Client stop failed");

		// disconnect MQTT client from broker
		Serial.println("--- Disconnecting from MQTT broker");
		success = _mqttClient.disconnect();
		if (success)
			Serial.println("------ MQTT Client disconnect successful");
		else
			Serial.println("------ MQTT Client disconnect failed");

		executeLoop = false;	// do not execute this code anymore
	}
	
}


/*
Expected Serial output:

--- WiFi connected, IP address: 192.168.1.213
--- MQTT Client start successful
--- onMqttBeforeConnect
------ Connecting to MQTT broker: mqtt://broker.hivemq.com:1883
--- onMqttConnected
------ MQTT broker connected
------ Publishing to topic ESP32_MQTTClient/status, qos: 1, msgId: 13429
------ Subscribing to topic ESP32_MQTTClient/testTopic, qos: 1, msgId: 20688
------ Publishing to topic ESP32_MQTTClient/testTopic, qos: 1, msgId: 21702
--- onMqttMessagePublishConfirmed
------ MQTT message publish confirmed by broker, msgId: 13429
--- onMqttTopicSubscribed
------ MQTT topic subscribed, msgId: 20688
--- onMqttMessageReceived
------ MQTT message received, msgId: 51, topic: ESP32_MQTTClient/testTopic, data: hello, qos: 1
--- onMqttMessagePublishConfirmed
------ MQTT message publish confirmed by broker, msgId: 21702
--- Unsubscribing from ESP32_MQTTClient/testTopic, msgId: 58723
--- onMqttTopicUnsubscribed
------ MQTT topic unsubscribed, msgId: 58723

--- If you power off your ESP now, it will trigger the last will by the broker after at least 45.00 seconds

--- Stopping MQTT Client
------ MQTT Client stop successful
--- Disconnecting from MQTT broker
------ MQTT Client disconnect successful
*/