// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _offgrid_sleepypi_H_
#define _offgrid_sleepypi_H_
#include "Arduino.h"
//add your includes for the project offgrid_sleepypi here


//end of add your includes here


//add your function definitions for the project offgrid_sleepypi here
void request_event();
void receive_event(int bytes_received);
void wait_alarm();
void wait_timer();
void wait_timer(long secs);
void wait_timershort();
void wait_timer_nortc();
void execute_command();

struct SleepyPiRegisterMap {
	unsigned char fixedChar;
	float inputVoltage;
	float rpiCurrent;
	float inputVoltageStopPi;
	float inputVoltageResume;
	int wakeupAlarmHour;
	int wakeupAlarmMinute;
	unsigned long wakeupSeconds;
	unsigned char command;
	unsigned char watchdog_counter;
};

#define REGMAP_SIZE sizeof(SleepyPiRegisterMap)

union SleepyPiRegisters {
	struct SleepyPiRegisterMap vars;
	unsigned char regMapTemp[REGMAP_SIZE];
};

#define WD_DEFAULT 60

#define CMD_NOTHING 0
#define CMD_WAIT_ALARM 1
#define CMD_WAIT_TIMER 2
#define CMD_POWEROFF_EXT 4
#define CMD_POWERON_EXT 5

//Do not add code below this line
#endif /* _offgrid_sleepypi_H_ */
