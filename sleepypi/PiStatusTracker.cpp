/*
 * PiStatusTracker.cpp
 *
 *  Created on: Mar 6, 2017
 *      Author: kervel
 */

#include "PiStatusTracker.h"
#include <SleepyPi2.h>


PiStatusTracker::PiStatusTracker() {
	timeSinceLastChange = millis();
	if (SleepyPi.checkPiStatus(false)) {
		theStatus = eRUNNING;
	} else {
		theStatus = eOFF; // not right
	}
}

void PiStatusTracker::changeStatus(enum RaspiStatus newStatus) {
	if (newStatus == theStatus) {
		return;
	}
	theStatus = newStatus;
	timeSinceLastChange = millis();
}

void PiStatusTracker::startShutdownHandshake() {
	if (theStatus == eRUNNING) {
		changeStatus(eHALTING);
		SleepyPi.startPiShutdown();
	}
}

void PiStatusTracker::pollForChanges(bool cutPowerOnHalt) {
	bool stat = SleepyPi.checkPiStatus(false);
	bool pwr = SleepyPi.power_on;

	if ((theStatus != eOFF) && !pwr) {
		changeStatus(eOFF);
	} else {
		switch (theStatus) {
			case eOFF:
				if (pwr && !stat) {
					changeStatus(eBOOTING);
				}
				if (pwr && stat) {
					changeStatus(eRUNNING);
				}
				break;
			case eRUNNING:
				if (!stat) {
					changeStatus(eHALTED);
				}
				break;
			case eBOOTING:
				if (stat) {
					changeStatus(eRUNNING);
				}
				break;
			case eHALTING:
				long curTime = millis();
				if (!stat) {
					changeStatus(eHALTED);
				} else if (curTime - timeSinceLastChange > kFAILSAFETIME_MS) {
					changeStatus(eHALTING_TOOLONG);
				}
				break;
			case eHALTING_TOOLONG:
				if (!stat) {
					changeStatus(eHALTED);
				}
				if (cutPowerOnHalt) {
					SleepyPi.enablePiPower(false);
				}
				break;
			case eHALTED:
				if (stat) {
					changeStatus(eRUNNING);
				}
				if (cutPowerOnHalt) {
					SleepyPi.enablePiPower(false);
				}
				break;
		}
	}
}

PiStatusTracker::~PiStatusTracker() {
	// TODO Auto-generated destructor stub
}

PiStatusTracker piStatusTracker;
