// Do not remove the include below
#include "offgrid_sleepypi.h"

#include <Wire.h>
#include <SleepyPi2.h>
#include "PiStatusTracker.h"

#define DEBUG

#ifdef DEBUG
 #define debugpln(x)  Serial.println (x)
 #define debugp(x)  Serial.print (x)
#else
 #define debugpln(x)
 #define debugp(x)
#endif

#define SLAVE_ADDRESS 0x36
#define POWER_BUTTON_PIN	3
#define MAX_SENT_BYTES 5

union SleepyPiRegisters regmap;
bool newDataAvailable = 0;
bool written = 0;
bool isPowerSleep = false;
byte receivedCommands[MAX_SENT_BYTES];


void blinkDebug(unsigned char nblinks) {
	for (unsigned char x=0; x<nblinks; x++) {
		digitalWrite(LED_BUILTIN,HIGH);
		delay(50);
		digitalWrite(LED_BUILTIN,LOW);
		delay(50);
	}
}


//The setup function is called once at startup of the sketch
void setup()
{
#ifdef DEBUG
	Serial.begin(9600);
	debugpln("start");
#endif
	regmap.vars.fixedChar = 58;
	if (!SleepyPi.rtcInit(false)) {
		Serial.println("no RTC found");
	}
	SleepyPi.enablePiPower(true);
	/* rtcInit switches to I2C master mode for rtc setup.
	 * however, we want to be a slave for the raspberry pi, so switch back to slave mode.
	 * the wire library switches back to master mode when needed
	 */
	Wire.begin(SLAVE_ADDRESS);
	Wire.onRequest(request_event);
	Wire.onReceive(receive_event);
}


void dumpStatusSerial() {
    debugp("loop:");
    debugp(regmap.vars.inputVoltage);
    debugp(" - ");
    debugp(regmap.vars.rpiCurrent);
    debugp(" - ");
    debugp(regmap.vars.wakeupAlarmMinute);
    debugp(" - ");
    debugp(regmap.vars.command);
}

void dumpState() {
	switch(piStatusTracker.getCurrentStatus()) {
		case eOFF:
			debugpln('off');
			break;
		case eBOOTING:
			debugpln('booting');
			break;
		case eBOOTING_TOOLONG:
			debugpln('bootinglong');
			break;
		case eRUNNING:
			debugpln('running');
			break;
		case eHALTING:
			debugpln('halting');
			break;
		case eHALTING_TOOLONG:
			debugpln('haltinglong');
			break;
		case eHALTED:
			debugpln('halted');
			break;
		case eUNKNOWN:
			debugpln('unknown');
			break;

	}
}

// The loop function is called in an endless loop
void loop()
{
#ifdef DEBUG
	digitalWrite(LED_BUILTIN,HIGH);
	delay(50);
	digitalWrite(LED_BUILTIN,LOW);
#endif

	piStatusTracker.pollForChanges(true);
#ifdef DEBUG
	dumpState();
#endif
	regmap.vars.inputVoltage = SleepyPi.supplyVoltage();
	regmap.vars.rpiCurrent = SleepyPi.rpiCurrent();
	if (regmap.vars.inputVoltage > 0
			&& regmap.vars.inputVoltageStopPi > 0
			&& regmap.vars.inputVoltageResume > 0
			&& regmap.vars.inputVoltage < regmap.vars.inputVoltageStopPi) {
		// voltage dropped too low --> initiate shutdown sequence
		if (piStatusTracker.getCurrentStatus() != eOFF) {
			piStatusTracker.onPowerOff(0);
			piStatusTracker.startShutdownHandshake();
		}
		debugpln("pwrslp");
		blinkDebug(20);
		isPowerSleep = true;
	}
	if (piStatusTracker.getCurrentStatus() == eBOOTING ||
			piStatusTracker.getCurrentStatus() == eRUNNING) {
		if (digitalRead(POWER_BUTTON_PIN) == LOW) {
			piStatusTracker.startShutdownHandshake();
		}
	}
	if (isPowerSleep
			&& piStatusTracker.getCurrentStatus() == eOFF
			&& regmap.vars.inputVoltage >= regmap.vars.inputVoltageResume) {
		// voltage high enough --> start the rpi back up
		debugpln("resum");
		SleepyPi.enablePiPower(true);
		blinkDebug(3);
		isPowerSleep = false;
	}

	if (!isPowerSleep
			&& !piStatusTracker.hasPowerOffCallback()
			&& piStatusTracker.getCurrentStatus() == eOFF) {
		// spontaneous shutdown of rpi, wait 1 minute and power back up!
		debugpln("spont");
		wait_timershort();
	}

	if (piStatusTracker.isStableStatus()) {
		if (piStatusTracker.getCurrentStatus() == eOFF) {
			SleepyPi.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
		} else {
			delay(5000);
		}
	} else {
		// we cannot power down if we don't have a stable status because then we need millis()
		delay(700);
	}

	if (regmap.vars.command != CMD_NOTHING) {
		execute_command();
	}
	if (written) {
		dumpStatusSerial();

	}
	written = 0;
	newDataAvailable = 1;
}

void request_event()
{
	written = 1;
	Wire.write(regmap.regMapTemp + receivedCommands[0],REGMAP_SIZE - receivedCommands[0]);
}

void receive_event(int bytesReceived) {
	for (int a = 0; a < bytesReceived; a++)
	{
		if ( a < MAX_SENT_BYTES)
		{
			receivedCommands[a] = Wire.read();
		}
		else
		{
			Wire.read();  // if we receive more data then allowed just throw it away
		}
	}

	if(bytesReceived == 1 && (receivedCommands[0] < REGMAP_SIZE))
	{
		return;
	}
	if(bytesReceived == 1 && (receivedCommands[0] >= REGMAP_SIZE))
	{
		receivedCommands[0] = 0x00;
		return;
	}
	for (int i=1; i<bytesReceived; i++) {
		byte dataByte = receivedCommands[i];
		int offset = receivedCommands[0] + (i-1);
		if (offset < REGMAP_SIZE) {
			regmap.regMapTemp[offset] = dataByte;
		}
	}

}

void alarm_isr()
{
}


void execute_command() {
	unsigned char cmd = regmap.vars.command;
	regmap.vars.command = CMD_NOTHING;
	switch (cmd) {
	case CMD_WAIT_ALARM :
		piStatusTracker.onPowerOff(wait_alarm);
		piStatusTracker.startShutdownHandshake();
		break;
	case CMD_WAIT_TIMER:
		piStatusTracker.onPowerOff(wait_timer);
		piStatusTracker.startShutdownHandshake();
		break;
	case CMD_POWEROFF_EXT :
		SleepyPi.enableExtPower(false);
		break;
	case CMD_POWERON_EXT :
		SleepyPi.enableExtPower(true);
		break;
	}

}

void wait_timer() {
	wait_timer(regmap.vars.wakeupSeconds);
}

void wait_timershort() {
	blinkDebug(7);
	wait_timer(20);
}

void wait_timer(long wks) {
	blinkDebug(3);
	debugpln("waiting for timer...");
    attachInterrupt(0, alarm_isr, FALLING);    // Alarm pin
    eTIMER_TIMEBASE tb = eTB_SECOND;
    uint8_t tv = 0;
    if (wks < 250) {
    	tb = eTB_SECOND;
    	tv = wks;
    } else if (wks < 15300) {
    	tb = eTB_MINUTE;
    	tv = wks / 60;
    } else {
    	tb = eTB_HOUR;
    	tv = wks / 3600;
    }
    SleepyPi.setTimer1(tb, tv);
    delay(500);
    SleepyPi.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    detachInterrupt(0);
    SleepyPi.ackTimer1();
	debugpln("got timer interrupt...");
    blinkDebug(7);
    SleepyPi.enablePiPower(true);
}

void wait_alarm() {
	blinkDebug(3);
	debugpln("waiting for alarm...");
	attachInterrupt(0, alarm_isr, FALLING);		// Alarm pin
	SleepyPi.enableWakeupAlarm(true);
	SleepyPi.setAlarm(regmap.vars.wakeupAlarmHour, regmap.vars.wakeupAlarmMinute);
	delay(500);
	SleepyPi.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
	detachInterrupt(0);
	SleepyPi.ackAlarm();
	debugpln("got alarm...");
	blinkDebug(7);
	SleepyPi.enablePiPower(true);
}


