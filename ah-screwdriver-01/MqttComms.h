#pragma once



#include "MQTTClient.h"
#include <string>
#include <map>
#include "Poco/Mutex.h"

class CallbackInterface {
public:
	virtual void messageArrived_cb(std::string topic, std::string msg) = 0;
};

struct MqttConfiguration {
	std::string ip;
	int port;
	std::string uname;
	std::string pword;
};

class MqttComms {
public:
	void initialize(MqttConfiguration config) {
		int rc = 0;
		_connected = false;
		//init
		//rc = MQTTClient_create(&client, "192.168.7.2:1883", "Hasan", MQTTCLIENT_PERSISTENCE_NONE, NULL);
		rc = MQTTClient_create(&client, std::string(config.ip + ":" + std::to_string(config.port)).c_str(), "Hasan", MQTTCLIENT_PERSISTENCE_NONE, NULL);
		rc = MQTTClient_setCallbacks(client, NULL, connectionLost_cb, messageArrived_cb, NULL);

		connectToBroker();
	}

	void SendUpdate(std::string topic, std::string payload) {
		//so multiple senders will await each other
		Poco::Mutex::ScopedLock lock(_sendupdatemutex);

		if (_connected == false) {
			connectToBroker();
		}

		if (_connected == true) {
			int rc = 0;
			int qos = 0;
			int retained = 0;
			MQTTClient_deliveryToken dt;

			std::string str = payload;
			char * c_payload = const_cast<char *>(str.c_str());

			rc = MQTTClient_publish(client, topic.c_str(), str.length(), c_payload, qos, 0, &dt);
		}
	}

	void RegisterHandlers(std::string topic, CallbackInterface *callback) {
		Poco::Mutex::ScopedLock lock(_registermutex);

		_topicObservers[topic] = callback;


		if (_connected == true) {
			//subscribe
			int rc = 0;
			int qos = 2;
			rc = MQTTClient_subscribe(client, topic.c_str(), qos);
		}
	}

	void Close() {
		MQTTClient_destroy(&client);
	}

private:
	//MQTTClient_messageArrived messageArrived_cb;
	static bool _connected;
	static std::map<std::string, CallbackInterface*> _topicObservers;
	MQTTClient client;
	Poco::Mutex _sendupdatemutex;
	Poco::Mutex _registermutex;

	int connectToBroker() {
		int rc = 0;
		//connect
		MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
		conn_opts.keepAliveInterval = 10;
		conn_opts.cleansession = 1;
		rc = MQTTClient_connect(client, &conn_opts);
		if (rc == 0) {
			_connected = true;
			//renew subscriptions
			for (auto topicObserver : _topicObservers) {
				const char* topic = topicObserver.first.c_str();
				int qos = 2;
				rc = MQTTClient_subscribe(client, topic, qos);
			}
		}
		return rc;
	}

	static int messageArrived_cb(void* context, char* topicName, int topicLen, MQTTClient_message* message) {
		printf("Message arrived\n");
		printf("   topic: %s\n", topicName);
		printf("   message: %.*s\n", message->payloadlen, static_cast<char *>(message->payload));
		for (auto topicObserver : _topicObservers) {
			if (topicObserver.first == topicName) {
				topicObserver.second->messageArrived_cb(topicName, std::string(static_cast<char *>(message->payload), message->payloadlen));
			}
		}
		return 1;
	}

	//void MQTTClient_connectionLost(void* context, char* cause);
	static void connectionLost_cb(void* context, char* cause) {
		_connected = false;
	}

};