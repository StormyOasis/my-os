#ifndef __KERNEL_H
#define __KERNEL_H

#include "../include/defs.h"
#include "../include/stdarg.h"
#include "../include/ipc.h"

#define COMBINE(l, h) (((DWORD)h) << 16) & l)

#define LOWORD(l)  ((WORD)(DWORD)(l))
#define HIWORD(l)  ((WORD)((((DWORD)(l)) >> 16) & 0xFFFF))

#define MSG_PROC 4000
#define MSG_READY 5000
#define MSG_WAIT 5001

ULONG CG_CreateProcess(ULONG, void*, void*,void*,void * pEntry, DWORD, DWORD, ULONG *result);
ULONG CG_SendMessage(ULONG sel, ULONG pid, ULONG message, void* proc, void * data1, void *data2);
ULONG CG_WaitMessage(ULONG sel, MSG *);


#endif
