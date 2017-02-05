#ifndef __CG_STUBS_H
#define __CG_STUBS_H

#include "../include/types.h"
#include "../include/kernel.h"
#include "../include/ipc.h"
#include "../include/tss.h"

ULONG CreateProcess(char *, char *, char *, void * pEntry, DWORD pri, DWORD level, ULONG *result);

void * GetNextProc();

void TaskSwitch(ULONG);
ULONG ScheduleProc(void*);
ULONG GetCurrentTick();

ULONG SendMessage(ULONG pid, ULONG message, void* proc, void * data1, void *data2);
ULONG WaitMessage(MSG *message);

#endif
