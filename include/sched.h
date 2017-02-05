#ifndef __SCHED_H
#define __SCHED_H

#include "../include/extern.h"
#include "../include/proc.h"
#include "../include/basicadt.h"
#include "../include/badtcpp.h"
#include "../include/mutex.h"

#define NUM_LEVEL_4_Q 32
#define QUANTA 100 //50 ticks

class Scheduler
{
	public:
		Scheduler();
		~Scheduler();
	
	public:
		Proc * GetNextProc();	
		ULONG ScheduleProc(Proc *);	
		
		Queue * GetFirstProcInQueue(char, char);
		
		void RemoveProcFromQueue(ULONG pid);	
		
	//private:	
		Queue * level2q;
		
		Queue * level3q;	

		Queue * level4q[NUM_LEVEL_4_Q];
		
		Mutex * timerMutex;

};

ULONG InitIPCandScheduler();
void FixupReadyQueues();

ULONG intTimerHandler();

char isNextProc(ULONG pid);
void RemoveProcFromQueue(ULONG pid);

#endif
