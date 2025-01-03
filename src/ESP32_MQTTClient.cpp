#include "ESP32_MQTTClient.h"

ESP32_MQTTClient::ESP32_MQTTClient()
{
	_isConnected = false;
	setKeepAlive(30);
	setMaxPacketSize(1024);
}

ESP32_MQTTClient::~ESP32_MQTTClient()
{
	esp_mqtt_client_destroy(_mqttClient);
	if (_uriBuf != nullptr)
		free(_uriBuf);
}

void ESP32_MQTTClient::onMqttBeforeConnect(ESP32_MQTTCallbacks::OnMqttBeforeConnectCallback callback) {
	_onMqttBeforeConnectCallback = callback;
}

void ESP32_MQTTClient::onMqttConnected(ESP32_MQTTCallbacks::OnMqttConnectedCallback callback) {
	_onMqttConnectedCallback = callback;
}

void ESP32_MQTTClient::onMqttDisconnected(ESP32_MQTTCallbacks::OnMqttDisconnectedCallback callback) {
	_onMqttDisconnectedCallback = callback;
}

void ESP32_MQTTClient::onMqttTopicSubscribed(ESP32_MQTTCallbacks::OnMqttTopicSubscribedCallback callback) {
	_onMqttTopicSubscribedCallback = callback;
}

void ESP32_MQTTClient::onMqttTopicUnsubscribed(ESP32_MQTTCallbacks::OnMqttTopicUnsubscribedCallback callback) {
	_onMqttTopicUnsubscribedCallback = callback;
}

void ESP32_MQTTClient::onMqttMessageReceived(ESP32_MQTTCallbacks::OnMqttMessageReceivedCallback callback) {
	_onMqttMessageReceivedCallback = callback;
}

void ESP32_MQTTClient::onMqttMessagePublishConfirmed(ESP32_MQTTCallbacks::OnMqttMessagePublishConfirmedCallback callback) {
	_onMqttMessagePublishConfirmedCallback = callback;
}

void ESP32_MQTTClient::onMqttMessageDeleted(ESP32_MQTTCallbacks::OnMqttMessageDeletedCallback callback) {
	_onMqttMessageDeletedCallback = callback;
}

void ESP32_MQTTClient::onMqttError(ESP32_MQTTCallbacks::OnMqttErrorCallback callback) {
	_onMqttErrorCallback = callback;
}

void ESP32_MQTTClient::onMqttCustomEvent(ESP32_MQTTCallbacks::OnMqttCustomEventCallback callback) {
	_onMqttCustomEventCallback = callback;
}

void ESP32_MQTTClient::setBrokerUri(const char* uri)
{
	if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
		log_d("MQTT uri %s\n", uri);
	_mqttUri = uri;
	_mqttConfig.broker.address.uri = _mqttUri;
};

void ESP32_MQTTClient::setBrokerUrl(const char* url, const int port, const char* scheme)
{
	if (_uriBuf != nullptr)
		free(_uriBuf);
	_uriBuf = (char*)malloc(strlen(scheme) + strlen(url) + 10);
	sprintf(_uriBuf, "%s://%s:%u", scheme, url, port);
	setBrokerUri(_uriBuf);
};

void ESP32_MQTTClient::setBrokerIp(const IPAddress ipAddress, const int port, const char* scheme)
{
	if (_uriBuf != nullptr)
		free(_uriBuf);
	_uriBuf = (char*)malloc(strlen(scheme) + 25);
	sprintf(_uriBuf, "%s://%d.%d.%d.%d:%d", scheme, ipAddress[0], ipAddress[1], ipAddress[2], ipAddress[3], port);
	setBrokerUri(_uriBuf);
};

void ESP32_MQTTClient::setClientName(const char* name)
{
	_mqttClientName = name;
	_mqttConfig.credentials.client_id = _mqttClientName;
}

void ESP32_MQTTClient::setCredentials(const char* username, const char* password)
{
	_mqttUsername = username;
	_mqttConfig.credentials.username = username;
	_mqttConfig.credentials.authentication.password = password;
};

void ESP32_MQTTClient::setClientCert(const char* clientCert)
{
	_mqttConfig.credentials.authentication.certificate = clientCert;
}

void ESP32_MQTTClient::setCaCert(const char* caCert)
{
	_mqttConfig.broker.verification.certificate = caCert;
}

void ESP32_MQTTClient::setAuthKey(const char* clientKey)
{
	_mqttConfig.credentials.authentication.key = clientKey;
}

void ESP32_MQTTClient::setTaskPriority(int priority)
{
	_mqttConfig.task.priority = priority;
}

void ESP32_MQTTClient::setMaxInPacketSize(const int size)
{
	_mqttMaxInPacketSize = size;
	_mqttConfig.buffer.size = _mqttMaxInPacketSize;
}

void ESP32_MQTTClient::setMaxOutPacketSize(const int size)
{
	_mqttMaxOutPacketSize = size;
	_mqttConfig.buffer.out_size = _mqttMaxOutPacketSize;
}

void ESP32_MQTTClient::setMaxPacketSize(const int size)
{
	setMaxInPacketSize(size);
	setMaxOutPacketSize(size);
}

void ESP32_MQTTClient::setKeepAlive(const int keepAliveSeconds)
{
	_mqttKeepAliveSeconds = keepAliveSeconds;
	_mqttConfig.session.keepalive = keepAliveSeconds;
}

void ESP32_MQTTClient::setReconnectTimeout(int reconnectTimeoutMs) {
	_mqttConfig.network.reconnect_timeout_ms = reconnectTimeoutMs;
}

void ESP32_MQTTClient::setNetowrkOperationTimeout(int networkOperationTimeoutMs) {
	_mqttConfig.network.timeout_ms = networkOperationTimeoutMs;
}

void ESP32_MQTTClient::setLastWillMessage(const char* topic, const char* message, const int qos, const bool retain)
{
	_mqttConfig.session.last_will.topic = topic;
	_mqttConfig.session.last_will.msg = message;
	_mqttConfig.session.last_will.qos = qos;
	_mqttConfig.session.last_will.retain = retain;
	_mqttConfig.session.last_will.msg_len = strlen(message);
}

void ESP32_MQTTClient::disableCleanSession()
{
	_mqttConfig.session.disable_clean_session = true;
}

void ESP32_MQTTClient::disableAutoReconnect()
{
	_mqttConfig.network.disable_auto_reconnect = true;
}

/// <summary>
/// Publishes message to broker
/// </summary>
/// <returns>message_id of the publish message (for QoS 0 message_id will always be zero) on success. -1 on failure, -2 in case of full outbox.</returns>
int ESP32_MQTTClient::publish(const char* topic, const char* payload, int qos, bool retain)
{
	if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
	{
		log_d("Publishing message with topic: %s, payload: %s, qos:%d, retain: %d", topic, payload == NULL ? "NULL" : payload, qos, retain);
	}

	if (!_isConnected)
	{
		log_w("MQTT client is not connected, the message won't publish");
	}

	int result = esp_mqtt_client_publish(_mqttClient, topic, payload, 0, qos, retain);

	if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
	{
		if (result >= 0)	// message_id
		{
			log_d("Publish successful");
		}
		else if (result == -1) {
			log_e("Publish failed");
		}
		else if (result == -2) {
			log_e("Publish failed, out buffer full");
		}
	}

	return result;
}

/// <summary>
/// Enqueue a message to the outbox, to be sent later. Typically used for messages with qos>0, but could be also used for qos=0 messages if store=true.
/// This API generatesand stores the publish message into the internal outboxand the actual sending to the network is performed in the mqtt - task 
/// context(in contrast to the esp_mqtt_client_publish() which sends the publish message immediately in the user task's context). Thus, it could be 
/// used as a non blocking version of esp_mqtt_client_publish().
/// </summary>
/// <returns></returns>
int ESP32_MQTTClient::enqueue(const char* topic, const char* payload, int qos, bool retain, bool store)
{
	if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
	{
		log_d("Enqueueing message with topic: %s, payload: %s, qos:%d, retain: %d", topic, payload == NULL ? "NULL" : payload, qos, retain);
	}

	int enqueueResult = esp_mqtt_client_enqueue(_mqttClient, topic, payload, 0, qos, retain, store);

	if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
	{
		if (enqueueResult >= 0)		// message_id
		{
			log_d("Enqueue successful");
		}
		else if (enqueueResult == -1) {
			log_e("Enqueue failed");
		}
		else if (enqueueResult == -2) {
			log_e("Enqueue failed, out buffer full");
		}
	}

	return enqueueResult;
}

int ESP32_MQTTClient::subscribe(const char* topic, int qos)
{
	if (_mqttClient == NULL) {
		if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
			log_e("MQTT client is null, can't subscribe, use createClient() to create a client first");
		return -1;
	}
	else if (ESP32_MQTTCLIENT_LOGGING_ENABLED) {
		log_d("Subscribing to topic '%s', qos: %d", topic, qos);
	}

	int result = esp_mqtt_client_subscribe(_mqttClient, topic, qos);
	
	if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
	{
		if (result >= 0)
			log_d("Subscribed to topic: %s, qos: %d", topic, qos);
		else if(result == -1)
			log_e("Subscribe failed");
		else if (result == -2)
			log_e("Subscribe failed, out buffer full");
	}

	return result;
}

int ESP32_MQTTClient::unsubscribe(const char* topic)
{
	if (_mqttClient == NULL) {
		if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
			log_e("MQTT client is null, can't unsubscribe, use createClient() to create a client first");
		return -1;
	}
	else if (ESP32_MQTTCLIENT_LOGGING_ENABLED) {
		log_d("Unsubscribing from topic '%s'", topic);
	}

	int result = esp_mqtt_client_unsubscribe(_mqttClient, topic);

	if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
	{
		if (result >= 0)
			log_d("Unsubscribed from topic: %s", topic);
		else if (result == -1)
			log_e("Unsubscribe failed");
		else if (result == -2)
			log_e("Unsubscribe failed, out buffer full");
	}

	return result;
}

void ESP32_MQTTClient::printError(esp_mqtt_error_codes_t* error_handle)
{
	switch (error_handle->error_type)
	{
	case MQTT_ERROR_TYPE_NONE:
		log_d("Error Type: MQTT_ERROR_TYPE_NONE");
		break;

	case MQTT_ERROR_TYPE_TCP_TRANSPORT:
	{
		auto espTransportSockErrNoToStr = [](int espTransportSockErrNo) -> const char* {
			switch (espTransportSockErrNo) {
			case MQTT_ERROR_TYPE_NONE:
				return "MQTT_ERROR_TYPE_NONE";
			case MQTT_ERROR_TYPE_TCP_TRANSPORT:
				return "MQTT_ERROR_TYPE_TCP_TRANSPORT";
			case MQTT_ERROR_TYPE_CONNECTION_REFUSED:
				return "MQTT_ERROR_TYPE_CONNECTION_REFUSED";
			default:
				return "Unknown esp transport sock err no";
			}
		};

		log_d("Error Type: MQTT_ERROR_TYPE_TCP_TRANSPORT");
		log_d("Last error code reported from esp-tls: %s (0x%x)", esp_err_to_name(error_handle->esp_tls_last_esp_err), error_handle->esp_tls_last_esp_err);
		log_d("Last tls stack error number: 0x%x (%s)", error_handle->esp_tls_stack_err, strerror(error_handle->esp_tls_stack_err));
		log_d("Last captured esp transport sock errno : %s (%d) (%s)", espTransportSockErrNoToStr(error_handle->esp_transport_sock_errno), error_handle->esp_transport_sock_errno, strerror(error_handle->esp_transport_sock_errno));
		break;
	}

	case MQTT_ERROR_TYPE_CONNECTION_REFUSED:
	{

		auto connectReturnCodeToStr = [](int connectReturnCode) -> const char* {
			switch (connectReturnCode)
			{
			case MQTT_CONNECTION_ACCEPTED:
				return "MQTT_CONNECTION_ACCEPTED";
			case MQTT_CONNECTION_REFUSE_PROTOCOL:
				return "MQTT_CONNECTION_REFUSE_PROTOCOL";
			case MQTT_CONNECTION_REFUSE_ID_REJECTED:
				return "MQTT_CONNECTION_REFUSE_ID_REJECTED";
			case MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE:
				return "MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE";
			case MQTT_CONNECTION_REFUSE_BAD_USERNAME:
				return "MQTT_CONNECTION_REFUSE_BAD_USERNAME";
			case MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED:
				return "MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED";
			default:
				return "Unknown connect return code";
			}
		};

		log_d("Error type: MQTT_ERROR_TYPE_CONNECTION_REFUSED");
		log_d("Connection refused error: %s (0x%x)", connectReturnCodeToStr(error_handle->connect_return_code), error_handle->connect_return_code);
		break;
	}

	default:
		log_d("Error type: Unknown (0x%x)", error_handle->error_type);
		break;
	}
}

bool ESP32_MQTTClient::createClient()
{
	if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
		log_d("MQTT client init");

	esp_err_t result;

	if (_mqttUri == nullptr)
	{
		if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
			log_e("MQTT Broker server URI is not set, aborting connect");
		return false;
	}

	// get client from IDF mqtt_client lib
	_mqttClient = esp_mqtt_client_init(&_mqttConfig);

	if (_mqttClient == NULL) {
		if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
			log_d("MQTT client init failed");
		return false;
	}

	if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
		log_d("MQTT client register events");

	// register callback to handle events from client
	result = esp_mqtt_client_register_event(_mqttClient, MQTT_EVENT_ANY, ESP32_MQTTClient::handleMqttEventStatic, this);
	if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
		log_d("MQTT client register events result: %d (%s)", result, esp_err_to_name(result));

	return result == ESP_OK;
}

bool ESP32_MQTTClient::start()
{
	if (_mqttClient == NULL) {
		if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
			log_d("MQTT client is null, creating new client");
		if (!createClient()) {
			return false;
		}
	}

	// start the client and it's loop
	esp_err_t result = esp_mqtt_client_start(_mqttClient);

	if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
		log_d("MQTT client start result: %d (%s)", result, esp_err_to_name(result));
	
	return result == ESP_OK;
}

bool ESP32_MQTTClient::reconnect() {
	if (_mqttClient == NULL) {
		log_e("MQTT client is null, can't reconnect, use createClient() to create a client first");
		return false;
	}

	esp_err_t result = esp_mqtt_client_reconnect(_mqttClient);

	if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
		log_d("MQTT client reconnect result: %d (%s)", result, esp_err_to_name(result));

	return result == ESP_OK;
}

bool ESP32_MQTTClient::stop() {
	if (_mqttClient == NULL) {
		log_e("MQTT client is null, can't stop, use createClient() to create a client first");
		return false;
	}

	esp_err_t result = esp_mqtt_client_stop(_mqttClient);
	if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
		log_d("MQTT client stop result: %d (%s)", result, esp_err_to_name(result));

	return result == ESP_OK;
}

bool ESP32_MQTTClient::disconnect() {
	if (_mqttClient == NULL) {
		log_e("MQTT client is null, can't disconnect, use createClient() to create a client first");
		return false;
	}

	esp_err_t result = esp_mqtt_client_disconnect(_mqttClient);
	
	if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
		log_d("MQTT client disconnect result: %d (%s)", result, esp_err_to_name(result));
	
	return result == ESP_OK;
}

void ESP32_MQTTClient::handleMqttEventStatic(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	static_cast<ESP32_MQTTClient*>(event_handler_arg)->handleMqttEvent(event_base, event_id, event_data);
}

void ESP32_MQTTClient::handleMqttEvent(esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	const esp_mqtt_event_t* event = esp_mqtt_event_handle_t(event_data);
	// your_context_t *context = event->context;
	if (event->client == _mqttClient)
	{
		switch (event_id) {
		case MQTT_EVENT_BEFORE_CONNECT:
			
			if (ESP32_MQTTCLIENT_LOGGING_ENABLED)
			{
				log_d("MQTT_EVENT_BEFORE_CONNECT");

				if (_mqttUsername)
					log_d("Connecting to MQTT broker '%s' with client name '%s' and username '%s'...", _mqttUri, _mqttClientName, _mqttUsername);
				else
					log_d("Connecting to MQTT broker '%s' with client name '%s'...", _mqttUri, _mqttClientName);
			}

			if (_onMqttBeforeConnectCallback) {
				_onMqttBeforeConnectCallback();
			}
			break;
		case MQTT_EVENT_CONNECTED:
			if (ESP32_MQTTCLIENT_LOGGING_ENABLED) {
				log_d("MQTT_EVENT_CONNECTED");
				log_d("MQTT broker connected");
			}
			_isConnected = true;
			if (_onMqttConnectedCallback) {
				_onMqttConnectedCallback(event->session_present);
			}
			break;
		case MQTT_EVENT_DISCONNECTED:
			if (ESP32_MQTTCLIENT_LOGGING_ENABLED) {
				log_d("MQTT_EVENT_DISCONNECTED");
				log_d("MQTT broker disconnected");
			}
			_isConnected = false;
			if (_onMqttDisconnectedCallback) {
				_onMqttDisconnectedCallback();
			}
			break;
		case MQTT_EVENT_SUBSCRIBED:
			if (ESP32_MQTTCLIENT_LOGGING_ENABLED) log_d("MQTT_EVENT_SUBSCRIBED");
			if (_onMqttTopicSubscribedCallback) {
				_onMqttTopicSubscribedCallback(event->msg_id, event->error_handle->error_type, event->data, event->data_len);
			}
			break;
		case MQTT_EVENT_UNSUBSCRIBED:
			if (ESP32_MQTTCLIENT_LOGGING_ENABLED) log_d("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
			if (_onMqttTopicUnsubscribedCallback) {
				_onMqttTopicUnsubscribedCallback(event->msg_id);
			}
			break;
		case MQTT_EVENT_PUBLISHED:
			if (ESP32_MQTTCLIENT_LOGGING_ENABLED) log_d("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
			if (_onMqttMessagePublishConfirmedCallback) {
				_onMqttMessagePublishConfirmedCallback(event->msg_id);
			}
			break;
		case MQTT_EVENT_DATA:
			if (ESP32_MQTTCLIENT_LOGGING_ENABLED) log_d("MQTT_EVENT_DATA, msg_id: %d, topic: %.*s, data: %.*s", event->msg_id, event->topic_len, event->topic, event->data_len, event->data);
			if (_onMqttMessageReceivedCallback) {
				_onMqttMessageReceivedCallback(event->msg_id, event->topic, event->topic_len, event->data, event->data_len, event->current_data_offset, event->total_data_len, event->retain, event->qos, event->dup);
			}
			break;
		case MQTT_EVENT_ERROR:
			if (ESP32_MQTTCLIENT_LOGGING_ENABLED) {
				log_d("MQTT_EVENT_ERROR");
				printError(event->error_handle);
			}
			if (_onMqttErrorCallback)
				_onMqttErrorCallback(event->error_handle);
			break;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
		case MQTT_EVENT_DELETED:
			if (ESP32_MQTTCLIENT_LOGGING_ENABLED) log_d("MQTT_EVENT_DELETED, msg_id=%d", event->msg_id);
			if (_onMqttMessageDeletedCallback)
				_onMqttMessageDeletedCallback(event->msg_id);
			break;
#endif
		default:
			if (ESP32_MQTTCLIENT_LOGGING_ENABLED) log_d("Other event id: %d", event_id);
			if (_onMqttCustomEventCallback)
				_onMqttCustomEventCallback(event);
			break;
		}
	}
}
