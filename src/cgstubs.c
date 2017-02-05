#include "../include/cgstubs.h"

ULONG CreateProcess(char *name, char * path, char * parm, void * pEntry, DWORD pri, DWORD level, ULONG *result)
{			
	ULONG res = 0;
			
	__asm__ __volatile__ ("pushl %0" : : "g" (result));
	__asm__ __volatile__ ("pushl %0" : : "g" (level));
	__asm__ __volatile__ ("pushl %0" : : "g" (pri));
	__asm__ __volatile__ ("pushl %0" : : "g" (pEntry));	
	__asm__ __volatile__ ("pushl %0" : : "g" (parm));
	__asm__ __volatile__ ("pushl %0" : : "g" (path));	
	__asm__ __volatile__ ("pushl %0" : : "g" (name));	
				
	__asm__ __volatile__ ("lcall $0x68, $0x00" : "=a"(res));		
	__asm__ __volatile__("add $32, %esp");	
	
	return res;
}

void * GetNextProc()
{
	void *proc = NULL;
	
	__asm__ __volatile__("lcall $0x70, $0x00" : "=a"(proc));
	
	return proc;
}

void TaskSwitch(ULONG pid)
{
	__asm__ __volatile__("pushl %0" : : "g"(pid));
	__asm__ __volatile__("lcall $0x78, $0x00");
	__asm__ __volatile__("add $4, %esp");	
}

ULONG ScheduleProc(void* p)
{
	ULONG res = 1;
	
	if(p == 0)
		return 1;
	
	__asm__ __volatile__("pushl %0" : : "g"(p));
	__asm__ __volatile__("lcall $0x80, $0x00" : "=a"(res));
	__asm__ __volatile__("add $4, %esp");	
	
	return res;
}

ULONG GetCurrentTick()
{
	ULONG res = 0;
	
	__asm__ __volatile__("lcall $0x88, $0x00" : "=a"(res));
	
	return res;
}

ULONG SendMessage(ULONG pid, ULONG message, void* proc, void * data1, void *data2)
{
	ULONG res = 0;
			
	__asm__ __volatile__ ("pushl %0" : : "g" (data2));
	__asm__ __volatile__ ("pushl %0" : : "g" (data1));
	__asm__ __volatile__ ("pushl %0" : : "g" (proc));
	__asm__ __volatile__ ("pushl %0" : : "g" (message));
	__asm__ __volatile__ ("pushl %0" : : "g" (pid));
	
	__asm__ __volatile__("lcall $0x90, $0x00" : "=a"(res));
	__asm__ __volatile__("add $20, %esp");	
	
	return res;
}

ULONG WaitMessage(MSG *message)
{
	ULONG res = 0;
	
	__asm__ __volatile__("pushl %0" : : "g"(message));
	__asm__ __volatile__("lcall $0x98, $0x00" : "=a"(res));
	__asm__ __volatile__("add $4, %esp");
	
	return res;
	
}
