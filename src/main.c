#include "../include/kern_mem.h"
#include "../include/ipc.h"
#include "../include/sysman.h"
#include "../include/tss.h"
#include "../include/extern.h"

//main.c

//This is used for printing info to screen
TaskData pInitTD;

extern void RestartSystem();

int main(void)
{
	//DisableInterrupts();
	__asm("cli");

	/* Set up the some of the system TD so that we can use printf
	/ It will get fully setup in later
	/ This is just for now...
	*/
	pInitTD.TD_CurX = 0;
	pInitTD.TD_CurY = 0;
	pInitTD.TD_nColumns = 80;
	pInitTD.TD_nLines = 25;
	pInitTD.TD_VirtVid = (void*)0xb8000;
	pInitTD.TD_ExitCode = 0x9D;

	clrscr();

	debug_printf("-Initializing stage 1 and 2 memory manager...");

	//begin memory initialization
	if(init_low_level_mem())
		panic("\n\nInit panic! Cannot start system");

	debug_printf("\n-Kernel memory manager initialized.");
	debug_printf("\n\n-Initializing tasking information...");

	if(InitTaskingInfo())
		panic("\n\nInit panic! Cannot start system");

	debug_printf("\n-Tasking information initialized.");

	//begin IPC init

	debug_printf("\n\nInitializing interprocess communications system...");

	if(InitIPCandScheduler())
		panic("\n\nInit panic! Cannot start system");

	debug_printf("\n-Interprocess communications system initialized.");
	debug_printf("\n\n-System initialized.\n-Starting system manager...");

	System_Manager();

	return 0;
}
