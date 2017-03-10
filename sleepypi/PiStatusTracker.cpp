/*
 * PiStatusTracker.cpp
 *
 *  Created on: Mar 6, 2017
 *      Author: kervel
 */

#include "PiStatusTracker.h"
#include <SleepyPi2.h>

#define CMD_PI_TO_SHDWN_PIN	17


PiStatusTracker::PiStatusTracker() {
	timeSinceLastChange = millis();
	powerOffCallback = 0;
	powerOnAfterPowerOff = false;
	theStatus = eUNKNOWN;
}

void PiStatusTracker::onPowerOff(void(*userfunc)(void)) {
	powerOffCallback = userfunc;
}

void PiStatusTracker::changeStatus(enum RaspiStatus newStatus) {
	if (newStatus == theStatus) {
		return;
	}
	theStatus = newStatus;
	timeSinceLastChange = millis();
}

void PiStatusTracker::startShutdownHandshake() {
	if (theStatus == eRUNNING || theStatus == eBOOTING) {
		changeStatus(eHALTING);
		digitalWrite(CMD_PI_TO_SHDWN_PIN,HIGH);
	}

}

void PiStatusTracker::startPowercycleHandshake() {
	powerOnAfterPowerOff = true;
	startShutdownHandshake();
}

bool PiStatusTracker::hasPowerOffCallback() {
	return powerOffCallback != 0;
}

void PiStatusTracker::pollForChanges(bool autoManagePower) {
	bool stat = SleepyPi.checkPiStatus(false);
	bool pwr = SleepyPi.power_on;

	if ((theStatus != eOFF) && !pwr) {
		changeStatus(eOFF);
		digitalWrite(CMD_PI_TO_SHDWN_PIN,LOW);
		if (powerOffCallback != 0) {
			powerOffCallback();
			powerOffCallback = 0;
			// the power off callback might have changed the status --> we need to poll the status again
			theStatus = eUNKNOWN;
		}
	} else {
		long curTime = millis();

		switch (theStatus) {
			case eUNKNOWN:
				digitalWrite(CMD_PI_TO_SHDWN_PIN,LOW);
				if (stat) {
					theStatus = eRUNNING;
				} else {
					if (pwr) {
						theStatus = eBOOTING;
					} else {
						theStatus = eOFF; // not right
					}
				}
				break;
			case eOFF:
				if (pwr && !stat) {
					changeStatus(eBOOTING);
				}
				if (pwr && stat) {
					changeStatus(eRUNNING);
				}
				if (!pwr && !stat) {
					if (powerOnAfterPowerOff) {
						SleepyPi.enablePiPower(true);
						powerOnAfterPowerOff = false;
						changeStatus(eBOOTING);
					}
				}
				if (!pwr && stat) {
					// bypass jumper apparently set ...
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
				} else if (curTime - timeSinceLastChange > kFAILSAFETIME_MS) {
					changeStatus(eBOOTING_TOOLONG);
				}
				break;
			case eBOOTING_TOOLONG:
				if (stat) {
					changeStatus(eRUNNING);
				}
				break;
			case eHALTING:
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
				if (autoManagePower) {
					SleepyPi.enablePiPower(false);
				}
				break;
			case eHALTED:
				// prevent reboot loop when bypass jumper is set
				digitalWrite(CMD_PI_TO_SHDWN_PIN,LOW);
				if (stat) {
					changeStatus(eRUNNING);
				}
				if (autoManagePower) {
					SleepyPi.enablePiPower(false);
				}
				break;
		}
	}
}

enum RaspiStatus PiStatusTracker::getCurrentStatus() {
	return theStatus;
}


bool PiStatusTracker::isStableStatus() {
	if (powerOnAfterPowerOff && (theStatus == eOFF)) {
		return false;
	}
	return ((theStatus == eRUNNING) || (theStatus == eOFF));
}

PiStatusTracker::~PiStatusTracker() {
}

PiStatusTracker piStatusTracker;
