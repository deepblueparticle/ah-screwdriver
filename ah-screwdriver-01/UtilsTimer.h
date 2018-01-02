#pragma once


#include "Poco/Timer.h"
#include <functional>

class TestTimer
{
public:
	void Start(int delayMS, int intervalMS, std::function<void(void)> _callback) {
		_userCallback = _callback;
		_timer.setStartInterval(delayMS);
		_timer.setPeriodicInterval(intervalMS);
		Poco::TimerCallback<TestTimer> callback(*this, &TestTimer::wdCallback);
		_timer.start(callback);
	}
	void Stop() {
		_timer.stop();
	}
private:
	std::function<void(void)> _userCallback;

	Poco::Timer _timer;
	void wdCallback(Poco::Timer& t) {
		if (_userCallback) {
			_userCallback();
		}
	}
};