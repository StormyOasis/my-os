#include "../include/tss.h"
#include "../include/extern.h"

TaskState **tss_cache = (TaskState**)((addr_t)static_tss_cache);

ULONG initTSS(TaskState *tss, ULONG pid, char os, char pri, char dpl, char dinfo, addr_t * page_dir)
{
	tss->priority = pri;
	tss->iomapbase = 0x0ffff;
	tss->cr3 = (addr_t)(page_dir);
	tss->inCache = 0;
	tss->dpl = dpl;
	tss->selector = 0; //not in gdt
	tss->eflags = 0x0202;
	tss->tick_created = GetCurrentTick();
	tss->tick_last_run = 0;
	tss->gdtDescriptorInfo = dinfo;

	if(os)
		tss->cs = 0x08;
	else //default to user code
		tss->cs = 0x18;

	tss->ds = 0x10;
	tss->es = 0x10;
	tss->fs = 0x10;
	tss->gs = 0x10;
	tss->ss = 0x10;
	tss->ss0 = 0x10;

	//tss info for non os tasks

	return 0;
}

void inline clearCache()
{
	UINT i;

	for(i = 0; i < TSS_CACHE_SIZE; i++)
		tss_cache[i] = NULL;
}

ULONG inline CG_GetCurrentTick()
{
	__asm("mov %0, %%eax" : : "g" (TickCount));
	__asm("leave");
	__asm("lret");

	//return TickCount;
}

ULONG inline copyTSS(TaskState *dest, TaskState * src)
{
	if(!dest || !src)
	{
		dest = NULL; //just to be sure, better safe than sorry
		return 1;
	}

	memcpy(*dest, *src, sizeof(TaskState));

	return 0;
}

void forceToCache(TaskState *tss, char loc)
{
	tss_cache[loc] = tss;

	//now add to gdt

	tss->inCache = 1;
	tss->selector = CACHE_TO_SEL(loc);

	AddTSStoGDT((addr_t)tss, tss->selector, sizeof(TaskState)-1, 0x10);//tss->gdtDescriptorInfo);
}

void addTSStoCache(TaskState *tss)
{
	UINT i;
	ULONG oldest_tick = 0;
	ULONG oldest_loc = 1;

	//This is an LRU search

	for(i = 0; i < TSS_CACHE_SIZE; i++)
	{
		if(tss_cache[i] == NULL)
		{
			forceToCache(tss, i);
			return;
		}
		else
		{
			if(tss_cache[i]->tick_last_run > oldest_tick)
			{
				oldest_tick = tss_cache[i]->tick_last_run;
				oldest_loc = i;
			}
		}
	}

	//possible error: Are we sure that tss_cache is pointing to the right
	//tss after each change????

	//make sure that the old task knows that it is no longer in the cache/gdt
	//before we add over it
	tss_cache[oldest_loc]->inCache = 0;
	tss_cache[oldest_loc]->selector = 0;

	forceToCache(tss, oldest_loc);
}

ULONG initTD(TaskData * td, char *name, char * path, char * cmdline)
{
	strcpy(td->TD_Name, name);
	strcpy(td->TD_Path, path);
	strcpy(td->TD_CmdLine, cmdline);

	td->TD_ExitCode = 0;

	td->TD_VideoMem = (void*)(0xD0000000 + 0xb8000); //???????????
	td->TD_VirtVid = (void*)(0xD0000000 + 0xb8000);

	td->TD_CurX = 0;
	td->TD_CurY = 0;
	td->TD_nColumns = 80;
	td->TD_nLines = 25;
	td->TD_VidMode = 0;
	td->TD_NormVid = 7;
	td->TD_Cursor = 0;
	td->TD_CursorType = 0;
	td->TD_SysIn[0] = '\0';
	td->TD_SysOut[0] = '\0';

	return 0;
}

ULONG inline SetBitLong(char bit, ULONG data)
{
	ULONG mask = 1 << bit;
	return (data | mask);
}


char inline GetBitStateLong(char bit, ULONG data)
{
	ULONG mask = 1 << bit;

	if(data & mask)
		return 1;

	return 0;
}


/*
Note:
	property values:

	NOTUSED - G - AVL - P - DPL(2) - SYS - BUSY

*/

void AddTSStoGDT(addr_t tssaddr, long selector, long limit, char properties)
{
	char i,j;
	ULONG res1 = 0;
	ULONG res2 = 0;
	addr_t *gdt_addr = (addr_t*)(GDTBase + selector);

	//forth byte of base address
	for(i = 24; i < 32; i++)
	{
		if(GetBitStateLong(i, tssaddr))
			res2 = SetBitLong(i, res2);
	}

	//Granularity
	if(GetBitStateLong(6, properties))
		res2 = SetBitLong(23, res2);
	//Available Bit
	if(GetBitStateLong(5, properties))
		res2 = SetBitLong(20, res2);

	//upper nibble of limit
	for(i = 16; i < 20; i++)
	{
		if(GetBitStateLong(i, limit))
			res2 = SetBitLong(i, res2);
	}

	//present bit
	if(GetBitStateLong(4, properties))
		res2 = SetBitLong(15, res2);

	//DPL
	if(GetBitStateLong(3, properties))
		res2 = SetBitLong(14, res2);

	//DPL
	if(GetBitStateLong(2, properties))
		res2 = SetBitLong(13, res2);

	//Sys bit(probably always 0)
	if(GetBitStateLong(1, properties))
		res2 = SetBitLong(12, res2);

	//will always be using 32 bit code so this bit should be 1
	res2 = SetBitLong(11, res2);

	//Busy Bit
	if(GetBitStateLong(0, properties))
		res2 = SetBitLong(9, res2);

	res2 = SetBitLong(8, res2); //always 1

	//third byte of base address
	for(i = 16, j = 0; i < 23; i++, j++)
	{
		if(GetBitStateLong(i, tssaddr))
			res2 = SetBitLong(j, res2);
	}

	//Res2 finished

	for(i = 0, j = 16; i < 16; i++, j++)
	{
		if(GetBitStateLong(i, tssaddr))
			res1 = SetBitLong(j, res1);
	}

	for(i = 0; i < 16; i++)
	{
		if(GetBitStateLong(i, limit))
			res1 = SetBitLong(i, res1);
	}

	//res1 finished

	//now add to gdt

	//debug_printf("\n\n1: %d 2: %d", gdt_addr[0], gdt_addr[1]);

	gdt_addr[0] = res1;
	gdt_addr[1] = res2;

	//debug_printf("\n\n1: %d 2: %d", gdt_addr[0], gdt_addr[1]);
}
