#pragma once

#include <Arduino.h>
#include <mqtt_client.h>

#define ESP32_MQTTCLIENT_LOGGING_ENABLED false

namespace ESP32_MQTTCallbacks
{
    typedef std::function<void()> OnMqttBeforeConnectCallback;
    typedef std::function<void(int sessionPresent)> OnMqttConnectedCallback;
    typedef std::function<void()> OnMqttDisconnectedCallback;
    typedef std::function<void(int msgId, esp_mqtt_error_type_t errorType, char* data, int dataLen)> OnMqttTopicSubscribedCallback;
    typedef std::function<void(int msgId)> OnMqttTopicUnsubscribedCallback;
    typedef std::function<void(int msgId, char* topic, int topicLen, char* data, int dataLen, int currentDataOffset, int totalDataLen, bool retain, int qos, bool dup)> OnMqttMessageReceivedCallback;
    typedef std::function<void(int msgId)> OnMqttMessagePublishConfirmedCallback;
    typedef std::function<void(int msgId)> OnMqttMessageDeletedCallback;
    typedef std::function<void(esp_mqtt_error_codes_t* error)> OnMqttErrorCallback;
    typedef std::function<void(const esp_mqtt_event_t* event)> OnMqttCustomEventCallback;
}


class ESP32_MQTTClient
{
public:
    ESP32_MQTTClient();
    ~ESP32_MQTTClient();

    void onMqttBeforeConnect(ESP32_MQTTCallbacks::OnMqttBeforeConnectCallback callback);
    void onMqttConnected(ESP32_MQTTCallbacks::OnMqttConnectedCallback callback);
    void onMqttDisconnected(ESP32_MQTTCallbacks::OnMqttDisconnectedCallback callback);
    void onMqttTopicSubscribed(ESP32_MQTTCallbacks::OnMqttTopicSubscribedCallback callback);
    void onMqttTopicUnsubscribed(ESP32_MQTTCallbacks::OnMqttTopicUnsubscribedCallback callback);
    void onMqttMessageReceived(ESP32_MQTTCallbacks::OnMqttMessageReceivedCallback callback);
    void onMqttMessagePublishConfirmed(ESP32_MQTTCallbacks::OnMqttMessagePublishConfirmedCallback callback); // works only for QOS 1 and 2
    void onMqttMessageDeleted(ESP32_MQTTCallbacks::OnMqttMessageDeletedCallback callback);
    void onMqttError(ESP32_MQTTCallbacks::OnMqttErrorCallback callback);
    void onMqttCustomEvent(ESP32_MQTTCallbacks::OnMqttCustomEventCallback callback);


    // three ways to set broker uri
    void setBrokerUri(const char* uri);   // setURI("mqtt://192.168.1.100:1883");
    void setBrokerUrl(const char* url, const int port = 1883, const char* scheme = "mqtt"); // setBrokerURL("192.168.1.100"); scheme can be mqtt, mqtts, ws, wss
    void setBrokerIp(const IPAddress ipAddress, const int port = 1883, const char* scheme = "mqtt"); // IPAddress mqttBrokerIP(192, 168, 1, 100); setBrokerIP(mqttBrokerIP); scheme can be mqtt, mqtts, ws, wss
    void setClientName(const char* name); // Allow to set client name manually (must be done in setup(), else it will not work.)
    void setCredentials(const char* username, const char* password);
    void setClientCert(const char* clientCert);
    void setCaCert(const char* caCert);
    void setAuthKey(const char* clientKey);
    void setTaskPriority(int priority);
    void setMaxPacketSize(const int size); // override the default value of 1024
    void setMaxInPacketSize(const int size); // override the default value of 1024
    void setMaxOutPacketSize(const int size); // override the default value of 1024
    void setKeepAlive(const int keepAliveSeconds); // Change the keepalive interval (30 seconds by default), when configuring this value, keep in mind that the client attempts to communicate with the broker at half the interval that is actually set. This conservative approach allows for more attempts before the broker's timeout occurs
    void setReconnectTimeout(int reconnectTimeoutMs);   // Reconnect to the broker after this value in miliseconds if auto reconnect is not disabled (defaults to 10s)
    void setNetowrkOperationTimeout(int networkOperationTimeoutMs);   // Abort network operation if it is not completed after this value, in milliseconds (defaults to 10s).
    void setLastWillMessage(const char *topic, const char *message, const int qos = 0, const bool retain = false); // Must be set before the first loop() call.
    void disableCleanSession();    //MQTT clean session, default clean_session is true
    void disableAutoReconnect();
  
    int publish(const char* topic, const char* payload, int qos = 0, bool retain = false);
    int enqueue(const char* topic, const char* payload, int qos = 0, bool retain = false, bool store = true);  // store - if true, all messages are enqueued; otherwise only QoS 1 and QoS 2 are enqueued

    int subscribe(const char* topic, int qos = 0);
    int unsubscribe(const char* topic);

    inline bool isConnected() { return _isConnected; };
    inline const char *getClientName() { return _mqttClientName; };
    inline const char *getURI() { return _mqttUri; };
    inline const int getOutboxBufferSize() { return _mqttClient != nullptr ? esp_mqtt_client_get_outbox_size(_mqttClient) : -1; }
    inline const int getKeepAliveSeconds() { return _mqttKeepAliveSeconds; }

    void printError(esp_mqtt_error_codes_t *error_handle);

    bool createClient(); // Creates MQTT client handle based on the configuration.
    bool start();        // Starts MQTT client. If client wasn't created with createClient() it will create the client first
    bool reconnect();    // This api is typically used to force reconnection upon a specific event.
    bool stop();         // Stops MQTT client tasks
    bool disconnect();   // This api is typically used to force disconnection from the broker.
    
private:
    esp_mqtt_client_config_t _mqttConfig;
    esp_mqtt_client_handle_t _mqttClient;

    bool _isConnected;
    unsigned long _nextMqttConnectionAttemptMillis;
    unsigned int _mqttReconnectionAttemptDelay;
    char* _uriBuf;
    const char* _mqttUri;
    const char* _mqttUsername;
    const char* _mqttClientName;
    int _mqttMaxInPacketSize;
    int _mqttMaxOutPacketSize;
    int _mqttKeepAliveSeconds;

    static void handleMqttEventStatic(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    void handleMqttEvent(esp_event_base_t event_base, int32_t event_id, void* event_data);

    ESP32_MQTTCallbacks::OnMqttBeforeConnectCallback _onMqttBeforeConnectCallback;
    ESP32_MQTTCallbacks::OnMqttConnectedCallback _onMqttConnectedCallback;
    ESP32_MQTTCallbacks::OnMqttDisconnectedCallback _onMqttDisconnectedCallback;
    ESP32_MQTTCallbacks::OnMqttTopicSubscribedCallback _onMqttTopicSubscribedCallback;
    ESP32_MQTTCallbacks::OnMqttTopicUnsubscribedCallback _onMqttTopicUnsubscribedCallback;
    ESP32_MQTTCallbacks::OnMqttMessageReceivedCallback _onMqttMessageReceivedCallback;
    ESP32_MQTTCallbacks::OnMqttMessagePublishConfirmedCallback _onMqttMessagePublishConfirmedCallback;
    ESP32_MQTTCallbacks::OnMqttMessageDeletedCallback _onMqttMessageDeletedCallback;
    ESP32_MQTTCallbacks::OnMqttErrorCallback _onMqttErrorCallback;
    ESP32_MQTTCallbacks::OnMqttCustomEventCallback _onMqttCustomEventCallback;
};
