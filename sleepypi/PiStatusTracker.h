/*
 * PiStatusTracker.h
 *
 *  Created on: Mar 6, 2017
 *      Author: kervel
 */

#ifndef PISTATUSTRACKER_H_
#define PISTATUSTRACKER_H_

enum RaspiStatus {
	eBOOTING,
	eBOOTING_TOOLONG,
	eRUNNING,
	eHALTING,
	eHALTING_TOOLONG,
	eHALTED,
	eUNKNOWN,
	eOFF
};

class PiStatusTracker {
public:
	PiStatusTracker();

	enum RaspiStatus getCurrentStatus();
	void pollForChanges(bool autoManagePower);
	void startShutdownHandshake();
	void startPowercycleHandshake();
	bool isStableStatus();
	void changeStatus(enum RaspiStatus newStatus);
	void onPowerOff(void (*callback) (void));
	bool hasPowerOffCallback();

	~PiStatusTracker();


private:
	enum RaspiStatus theStatus;
	long timeSinceLastChange;
	bool powerOnAfterPowerOff;
	void (*powerOffCallback) (void);
};

extern PiStatusTracker piStatusTracker;

#endif /* PISTATUSTRACKER_H_ */
