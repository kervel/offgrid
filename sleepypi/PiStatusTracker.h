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
	eRUNNING,
	eHALTING,
	eHALTING_TOOLONG,
	eHALTED,
	eOFF
};

class PiStatusTracker {
public:
	PiStatusTracker();

	enum RaspiStatus getCurrentStatus();
	void pollForChanges(bool cutPowerOnHalt);
	void startShutdownHandshake();
	void changeStatus(enum RaspiStatus newStatus);


	virtual ~PiStatusTracker();


private:
	enum RaspiStatus theStatus;
	long timeSinceLastChange;
};

extern PiStatusTracker piStatusTracker;

#endif /* PISTATUSTRACKER_H_ */
