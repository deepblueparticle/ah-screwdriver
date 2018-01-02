// ah-weather-01.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <thread>
#include <chrono>
#include "UtilsTimer.h"
#include "MqttComms.h"
#include <map>
#include <string>
#include <iostream>

#ifdef _WIN32

#else
	#define Sleep(x) usleep(x*1000)
#endif



class ServiceRegistration
{
public:
	void Initialize() {
		_serviceRegistrationValues["getLastOperation"] = "{\
				\"name\": \"getLastOperation\",\
				\"type\" : \"_getLastOperation._json._mqtt._tcp\",\
				\"port\" : \"1183\",\
				\"host\" : \"127.0.0.1\",\
				\"properties\" : {\
				\"property\": [{\
					\"name\": \"version\",\
						\"value\" : \"1.0\"\
				}, {\
					\"name\": \"path\",\
				\"value\" : \"screwdriver/sim/getLastOperation\"\
				}]\
			}\
		}";

		_serviceRegistrationValues["getOperationalState"] = "{\
				\"name\": \"getOperationalState\",\
				\"type\" : \"_getOperationalState._json._mqtt._tcp\",\
				\"port\" : \"1183\",\
				\"host\" : \"127.0.0.1\",\
				\"properties\" : {\
				\"property\": [{\
					\"name\": \"version\",\
						\"value\" : \"1.0\"\
				}, {\
					\"name\": \"path\",\
				\"value\" : \"screwdriver/sim/getOperationalState\"\
				}]\
			}\
		}";

		_serviceRegistrationValues["pushConfiguration"] = "{\
				\"name\": \"pushConfiguration\",\
				\"type\" : \"_pushConfiguration._json._mqtt._tcp\",\
				\"port\" : \"1183\",\
				\"host\" : \"127.0.0.1\",\
				\"properties\" : {\
				\"property\": [{\
					\"name\": \"version\",\
						\"value\" : \"1.0\"\
				}, {\
					\"name\": \"path\",\
				\"value\" : \"screwdriver/sim/pushConfiguration\"\
				}]\
			}\
		}";
	}

	void InjectCommunications(MqttComms *comms) {
		_comms = comms;
	}
	
	void Start(void) {

		//every10 seconds republish services
		_timer.Start(10, 10000, [this]()->void {
			const char* topic = "$ah/registry/service";
		//	std::string str = "{\
		//		\"name\": \"getLastOperation\",\
		//		\"type\" : \"_test._json._mqtt._tcp\",\
		//		\"port\" : \"1183\",\
		//		\"host\" : \"127.0.0.1\",\
		//		\"properties\" : {\
		//		\"property\": [{\
		//			\"name\": \"version\",\
		//				\"value\" : \"1.0\"\
		//		}, {\
		//			\"name\": \"path\",\
		//		\"value\" : \"screwdriver/sim/getLastOperation\"\
		//		}]\
		//	}\
		//}"; 
			for (auto service : _serviceRegistrationValues) {
				_comms->SendUpdate(topic, service.second);
				std::cout << topic << " : " << service.second << std::endl;
			}
		});
	}
	void Stop() {
		_timer.Stop();
	}
private:
	TestTimer _timer;
	MqttComms *_comms;
	std::map<std::string, std::string> _serviceRegistrationValues;
};




class ScrewDriver : public CallbackInterface {
public:
	int _torque = 0;
	int _angle = 0;
	void Initialize() {
		systemName = "screwdriver/sim";
		_cannedTopicsValues["getLastOperation"] = "{'id':'sim', 'time':'20170302224927', 'angle':'720', 'torque':'22', 'configuration':{'id':'sim','angle':'%a','torque':'%t'}}";
		_cannedTopicsValues["getOperationalState"] = "{'id':'sim','vbat':'24','lastcharge':'20170302183352'}";


		//subscribe
		_comms->RegisterHandlers("configure", this);
	}

	virtual void messageArrived_cb(std::string topic, std::string msg) {
		printf("*Message arrived\n");
		printf("*   topic: %s\n", topic.c_str());
		printf("*   message: %s\n", msg.c_str());

		
	}

	void InjectCommunications(MqttComms *comms) {
		_comms = comms;
	}

	void Start() {
		//every 10 seconds resend weather station topic updates
		_timer.Start(10, 10000, [this]()->void {
			for (auto topic : _cannedTopicsValues) {
				std::string::size_type f = topic.second.find("%a");
				std::string message = topic.second.replace(f, std::string("%a").length(), std::to_string(_angle));
				f = message.find("%t");
				message = message.replace(f, std::string("%t").length(), std::to_string(_torque));
				_comms->SendUpdate(systemName + "/" + topic.first, message);
				std::cout << topic.first << " : " << topic.second << std::endl;
			}
		});
	}
	void Stop() {
		_timer.Stop();
	}
private:
	MqttComms *_comms;
	TestTimer _timer;
	std::map<std::string, std::string> _cannedTopicsValues;
	std::string systemName;

};






std::map<std::string, CallbackInterface*> MqttComms::_topicObservers;
bool MqttComms::_connected;
int main()
{

	

	ServiceRegistration registration;
	MqttComms comms;
	ScrewDriver screwDriver;

	MqttConfiguration config;
	//config.ip = "192.168.0.111";
	config.ip = "127.0.0.1";
	config.port = 1883;
	config.uname = "ah-core-gw1";
	config.pword = "";

	registration.InjectCommunications(&comms);
	screwDriver.InjectCommunications(&comms);

	comms.initialize(config);
	registration.Initialize();
	screwDriver.Initialize();

	registration.Start();
	screwDriver.Start();

	while (1) {
		Sleep(60);
	};

	screwDriver.Stop();
	registration.Stop();
	comms.Close();

    return 0;
}

void initialize(void) {

}

