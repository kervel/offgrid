// Do not remove the include below
#include "offgrid_sleepypi.h"

#include <Wire.h>
#include <SleepyPi2.h>

#define DEBUG

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
#ifdef DEBUG
	Serial.begin(9600);
	Serial.println("start");
#endif
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
	Wire.write(regmap.regMapTemp + receivedCommands[0],REGMAP_SIZE);
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
#ifdef DEBUG
	Serial.print("Got ");
	Serial.print(bytesReceived);
	Serial.print(" bytes ");
	Serial.print(receivedCommands[0]);
	Serial.println("");
#endif
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
}


