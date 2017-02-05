#ifndef _PROC_H
#define _PROC_H

#include "../include/defs.h"
#include "../include/tss.h"
#include "../include/ipc.h"
#include "../include/kernel.h"
#include "../include/kern_mem.h"
#include "../include/extern.h"

#define NULL_PROC ((Proc *) 0)

//By consolodating everything, we eliminate the need for a list for the tss, the td, the msg bin
//and whatever else is needed.

typedef struct Proc_s
{
	ULONG pid;
	char in_use;
	char level;
	
	TaskState * tss;
	TaskData * td;
	MsgBin * msgbin;
	
	AddressSpace * as;	
	
	struct Proc_s * next;
	struct Proc_s * prev;
	
} Proc;

void inline zeroProcList();
void inline copyProcToList(Proc * proc, UINT offset);

ULONG inline generatePID(ULONG);

ULONG createNewProc(ULONG);
void createNewProcTableEntry(ULONG pid);
void destroyProc(ULONG pid);
void destroyProcTableEntry(ULONG pid);

Proc * getProccessWithPID(ULONG pid);	
	
ULONG InitTaskingInfo();

void CG_TaskSwitch(ULONG, ULONG);


#endif
