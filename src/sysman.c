#include "../include/kernel.h"
#include "../include/sysman.h"
#include "../include/types.h"
#include "../include/kern_mem.h"
#include "../include/proc.h"
#include "../include/extern.h"
#include "../include/util.h"

extern Proc ** pproc_tab;
extern Proc * currentProc;

static void IdleProc();
static void IdleProc2();
static void IdleProc3();
static void IdleProc4();
static void IdleProc5();


/*===================================================================

Task Scheduling:

Multilevel approach:
based on the MINIX operating system
(see pg. 94-95, 98 of "Operating Systems" by Tannenbaum)

------------------------------------------------
|          |           |           |           |
|   User   |    User   |    User   |    Idle   |		- Level 4
|          |           |           |           |
------------------------------------------------
------------------------------------------------
|          |           |           |           |
|  Service |  Service  |  Service  |  Service  |		- Level 3
|          |           |           |           |
------------------------------------------------
------------------------------------------------
|          |           |           |   System  |
|    I/O   |     I/O   |   I/O     |   task    |		- Level 2
|          |           |           |           |
------------------------------------------------
------------------------------------------------
|                                              |
|            OS Process Managment              |		- Level 1
|                                              |
------------------------------------------------

Level 4 are user programs.  Level 4 consists of an array of 32 queues.
1 for each priority level.  They are given very little access to the computer, aka
a low privalege level.

Level 3 are System services that are started at boot and run until the 
computer is shutdown.  They are similar to level 4 processes in the access
level, but are given more priority for the cpu.

Level 2 are interfaces between the kernel/hardware, and the user/system level tasks.
While most of these interface with a particular i/o device,
The system task serves as an interface between the kernel and everything else, so that
the kernel occasionally has access to certian info.

Level 1 is the kernel/scheduler.  It is run when an interrupt is fired or 
when a kernel primitive is called.  Highest access/privilege level.

Level 1 and 2 are compiled together and form the kernel.

Task Scheduling:

Level 1 is never scheduled, it only runs "on demand" via an interrupt or call to kernel primitive

Level 2 has the highest priority.  Tasks in this level run until they reach a wait state,
they cannot be preempted.  

Level 3 tasks all have the same priority.  It is higher than level 4 tasks, but lower than level 2.
Other than that, Level 3's run the same as level 2's except that they dont have the
same access rights.  Actually, I tink they should be run in a round-robin fashion with other Level 3's.  
It uses a single queue.

Level 4 user tasks use a 32 priority ready queue to determine what task is in a ready to
run state.  A user task will run for 30?ms unless there is a level 2 or 3 task that wants to run.


When the timer interrupt fires, the level 2 ready queue is evaluated.  If there is a waiting
task, then it is run until it is finished or enters into a waiting state.  If there is no level 2 
waiting, then the level 3 queue is evaluated.  If no level 3 is waiting, the level 4 user tasks
are finally given a chance to run.  Level 0 is the highest user level priority, 31 is the lowest.

Each interrupt, the entire process is repeated.  If a level 2 or 3 task was running, it continues 
to run until it finishes or waits.  If a level 4 was running, a test is done to see if it has
run for more than 30ms, if it has, it is moved to the end of its pri. level ready queue and the 
next task in that same level is run.  Each time the int fires, the level 2, then 3 queues are
evaluated first.

SHOULD I  MAKE IT SO THAT A LEVEL 2 TASK CAN INTERRUPT A LEVEL 3 TASK?!?!?!?!
since Level 3 tasks run until finished..hmmmmmm


--not sure if this will work or give a decent resolution.

see page 98

TSS's:

All tss's are stored in their appropriate linked list.  When a tss is that of a running task,
it is in the aTSS array.  When it is not running, it might still be in the aTSS array.  By not 
removing it from the aTSS array each time there is a task switch, we can save some cycles.  But,
the array may fill up eventually.
When a task is terminated, it's tss is removed from the aTSS array and the linked list.

The aTSS array is a sortof cache for the tss's.

When the aTSS array starts to fill up(say around 90%, but make that user/admin adjustable) we
can run a Least Recently Used Algorithm to move move inactive tss's out.


===================================================================*/

extern void SDG();

void System_Manager()
{			
	Proc *proc = NULL;
	ULONG res = 0;
	void *dqresult = NULL;

	char x;
	ULONG i = 0;
	
	debug("Here");
	__asm("hlt");
			
	DisableInterrupts(); //stay in single tasking mode until everything is setup	
	//EnableInterrupts();
	
	for(i = 0; i < 99000; i++)
	{		
		dqresult = kmalloc(32);
		if(!dqresult)
		{
			debug_printf("\nNull alloc: iteration %d", i);
			continue;			
		}
		
		debug_printf("\nIteration: %d, addr: %d", i, (ULONG)dqresult);
		debug_printf(" Writing data. ");	
		*(char*)dqresult = 5;
		if(*(char*)dqresult == 5)
			debug_printf("Success!");
		else
			debug_printf("Failed");
	}					

	while(1);
	
	debug_printf("\nCreating idle process...");
		
	ULONG pid1 = CreateProcess("System Idle", "/", "", IdleProc4, 31, 4, &res);		
	
	if(res)	
		panic("\n\nPanic! Error creating system idle process.");
	
	debug_printf("Done.\nSystem Idle process created with pid: %d", pid1);
	
	while(1);
	
	ULONG pid2 = CreateProcess("System Idle2", "/", "", IdleProc3, 30, 4, &res);		
	
	if(res)	
		panic("\n\nPanic! Error creating system idle process 2.");
	
	debug_printf("\nSystem Idle process 2 created with pid: %d", pid2);
	
	ULONG pid3 = CreateProcess("System Idle3", "/", "", IdleProc2, 29, 4, &res);		
	
	if(res)	
		panic("\n\nPanic! Error creating system idle process 3.");
	
	debug_printf("\nSystem Idle process 3 created with pid: %d", pid3);
	
	ULONG pid4 = CreateProcess("System Idle4", "/", "", IdleProc, 28, 4, &res);		
	
	if(res)	
		panic("\n\nPanic! Error creating system idle process 4.");
	
	debug_printf("\nSystem Idle process 4 created with pid: %d", pid4);
	
	ULONG pid5 = CreateProcess("System Idle5", "/", "", IdleProc5, 30, 4, &res);		
	
	if(res)	
		panic("\n\nPanic! Error creating system idle process 5.");
	
	debug_printf("\nSystem Idle process 5 created with pid: %d", pid5);								
	
	FixupReadyQueues();
	
	nSwitches = 1;	
	
	//SDG();
	
	//LET"S GO!!!!!!!!!!!!!!!!!
	EnableInterrupts();
	
	
	while(1);
	
	
//	end:;		
/*

Tasking model note:

the send/wait functions are the primary means to enqueue/dequeue task in the rdy q

lock the kbd if a task is in use by the user(non-background), 
but is in a waiting state(eg. waiting for kbd or other device)

*/				
	
	/*
	We have separated the IdleTask from here..What do I do with this task now?	
	
	I can just not queue it up, but the actual task data will waste memory.
	
	The monitor task's job is to watch for key hits, and to watch for msgs that 
	terminate tasks.
	
	Use the monitor to clear up everything.
	
	*/
	
	//should never get here, but just in case....
	__asm("hlt");	
}

static void IdleProc()
{	
	ULONG x = 0;		
		
	while(1)
	{
		
	//	debug_printf("1");	
	
		//if(x == 2)
		//	halt();		
		
		debug_printf("\n%d - Proc 1", x);
		x++;	
		
		//Sleep(9500);
	}		
}

static void IdleProc2()
{			
	ULONG x = 0;
	
	//debug_printf("2");
			
	while(1)
	{
		
	//	debug_printf("1");
	
	
		//if(x == 2)
		//	halt();		
		
		debug_printf("\n%d - Proc 2", x);
		x++;				
		
		//Sleep(9500);
	}		
}

static void IdleProc3()
{			
	ULONG x = 0;
			
			
	while(1)
	{
	//	debug_printf("1");		
	
		//if(x == 2)
		//	halt();		
		
		debug_printf("\n%d - Proc 3", x);
		x++;
		
		//Sleep(9500);
	}
}

static void IdleProc4()
{			
	ULONG x = 0;
			
			
	while(1)
	{
	//	debug_printf("1");		
	
		//if(x == 2)
		//	halt();		
		
		debug_printf("\n%d - Proc 4", x);
		x++;
		
		//Sleep(9500);
	}
}

static void IdleProc5()
{			
	ULONG x = 0;
			
			
	while(1)
	{
	//	debug_printf("1");		
	
		//if(x == 2)
		//	halt();		
		
		debug_printf("\n%d - Proc 5", x);
		x++;
		
		//Sleep(9500);
	}
}
