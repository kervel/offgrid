// Do not remove the include below
#include "offgrid_sleepypi.h"

#include <Wire.h>
#include <SleepyPi2.h>

#define SLAVE_ADDRESS 0x36
#define MAX_SENT_BYTES 5

union SleepyPiRegisters regmap;
bool newDataAvailable = 0;

byte receivedCommands[MAX_SENT_BYTES];


//The setup function is called once at startup of the sketch
void setup()
{
	Wire.begin(SLAVE_ADDRESS);
	Wire.onRequest(request_event);
	Wire.onReceive(receive_event);
}

// The loop function is called in an endless loop
void loop()
{
	regmap.vars.inputVoltage = SleepyPi.supplyVoltage();
	regmap.vars.rpiCurrent = SleepyPi.rpiCurrent();
	if (regmap.vars.command != CMD_NOTHING) {
		execute_command();
	}
	newDataAvailable = 1;
}

void request_event()
{
	Wire.write(regmap.regMapTemp,REGMAP_SIZE);
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

}

void alarm_isr()
{
}


void execute_command() {
	unsigned char cmd = regmap.vars.command;
	regmap.vars.command = CMD_NOTHING;
	switch (cmd) {
		case CMD_WAIT_ALARM : wait_alarm();
							 break;
		case CMD_POWEROFF_EXT : SleepyPi.enableExtPower(false);
							 break;
		case CMD_POWERON_EXT : SleepyPi.enableExtPower(true);
							 break;
		case CMD_POWEROFF_RPI : SleepyPi.startPiShutdown();
							 break;

	}

}

void wait_alarm() {
    attachInterrupt(0, alarm_isr, FALLING);		// Alarm pin
    SleepyPi.enableWakeupAlarm(true);
	SleepyPi.setAlarm(regmap.vars.wakeupAlarmHour, regmap.vars.wakeupAlarmMinute);
    delay(500);
    SleepyPi.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    detachInterrupt(0);
    SleepyPi.ackAlarm();
}


