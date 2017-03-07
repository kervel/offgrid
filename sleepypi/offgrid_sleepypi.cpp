// Do not remove the include below
#include "offgrid_sleepypi.h"

#include <Wire.h>
#include <SleepyPi2.h>
#include <PiStatusTracker.h>

#define DEBUG

#define SLAVE_ADDRESS 0x36
#define MAX_SENT_BYTES 5

union SleepyPiRegisters regmap;
bool newDataAvailable = 0;
bool written = 0;
bool isPowerSleep = false;
byte receivedCommands[MAX_SENT_BYTES];


//The setup function is called once at startup of the sketch
void setup()
{
#ifdef DEBUG
	Serial.begin(9600);
	Serial.println("start");
#endif
	regmap.vars.fixedChar = 58;
	if (!SleepyPi.rtcInit(false)) {
		Serial.println("no RTC found");
	}
	/* rtcInit sets up RTC in master mode, but we want to be a slave most of the time
	 * normally, the wire library switches to master mode when needed
	 */
	Wire.begin(SLAVE_ADDRESS);
	Wire.onRequest(request_event);
	Wire.onReceive(receive_event);
}


void dumpStatusSerial() {
    Serial.print("loop:");
    Serial.print(regmap.vars.inputVoltage);
    Serial.print(" - ");
    Serial.print(regmap.vars.rpiCurrent);
    Serial.print(" - ");
    Serial.print(regmap.vars.wakeupAlarmMinute);
    Serial.print(" - ");
    Serial.println(regmap.vars.command);
}

// The loop function is called in an endless loop
void loop()
{
	piStatusTracker.pollForChanges(true);
	regmap.vars.inputVoltage = SleepyPi.supplyVoltage();
	regmap.vars.rpiCurrent = SleepyPi.rpiCurrent();
	if (regmap.vars.inputVoltage > 0 && regmap.vars.inputVoltage < regmap.vars.inputVoltageStopPi) {
		piStatusTracker.onPowerOff(0);
		piStatusTracker.startShutdownHandshake();
		isPowerSleep = true;
	}
	if (isPowerSleep && piStatusTracker.getCurrentStatus() == eOFF && regmap.vars.inputVoltage >= regmap.vars.inputVoltageResume) {
		SleepyPi.enablePiPower(true);
		isPowerSleep = false;
	}

	if (piStatusTracker.isStableStatus()) {
		SleepyPi.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
	} else {
		delay(500);
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
		wait_alarm();
		break;
	case CMD_POWEROFF_EXT :
		SleepyPi.enableExtPower(false);
		break;
	case CMD_POWERON_EXT :
		SleepyPi.enableExtPower(true);
		break;
	case CMD_POWEROFF_RPI :
		SleepyPi.startPiShutdown();
		break;
	}

}


void wait_alarm() {
#ifdef DEBUG
	Serial.println("waiting for alarm...");
#endif
	attachInterrupt(0, alarm_isr, FALLING);		// Alarm pin
	SleepyPi.enableWakeupAlarm(true);
	SleepyPi.setAlarm(regmap.vars.wakeupAlarmHour, regmap.vars.wakeupAlarmMinute);
	delay(500);
	SleepyPi.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
#ifdef DEBUG
	Serial.println("got alarm...");
#endif
	detachInterrupt(0);
	SleepyPi.ackAlarm();
	SleepyPi.enablePiPower(true);
}


