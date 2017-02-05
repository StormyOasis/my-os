#include "../include/proc.h"
#include "../include/extern.h"

extern ULONG max_procs;
extern TaskData pInitTD;
extern ULONG nSwitches;

//actual process table
Proc *proc_head = NULL;
Proc *proc_tail = NULL;
Proc *currentProc = NULL;

//These are pointers to the entries (allows for quicker access, idea from Minix)
Proc **pproc_tab = NULL;

extern TaskState **tss_cache;

ULONG InitTaskingInfo()
{
	debug_printf("\n\t|--Creating process table...");

	pproc_tab = (Proc**)kmalloc(max_procs * sizeof(addr_t)); //(Proc**)allocate_from_image_end(max_procs * sizeof(addr_t));
	zeroProcList();
	debug_printf("Done.");

	debug_printf("\nCreating system process...");

	createNewProcTableEntry(0);

	//NOTE: Commented out pending rework of kmalloc()
//	if(CreateAddressSpace(pproc_tab[0]->as, 1))
	//	return 1;

	debug_printf("\n----: System address space initialized.");

	if(initTSS(pproc_tab[0]->tss, 0, 1, 25, 0, 0x10, pproc_tab[0]->as->page_dir))
		return 1;

	debug_printf("\n----: Kernel task state structure created.");
	pproc_tab[0]->tss->selector = tss_sel_loc;
	pproc_tab[0]->pid = 0;

	//tss_cache[0] = pproc_tab[0]->tss;
	pproc_tab[0]->tss->inCache = 1;

									//0x10 instead of 0x11 because LTR will set busy bit???
	AddTSStoGDT((addr_t)pproc_tab[0]->tss, pproc_tab[0]->tss->selector, sizeof(TaskState)-1, 0x10);
	debug_printf("\n----: Kernel task state structure added to gdt.");
	InitOSTSS(pproc_tab[0]->tss);
	debug_printf("\n----: Kernel task state structure initialized.");

	currentProc = pproc_tab[0];
	currentProc->in_use = 1;
	currentProc->level = 1;

	debug_printf("\n----: System task state info initialized.");

	//We don't need to do this since it is the os task.
	//All of this should already be setup?!?!?!?!??
	//if(initAddressSpace(pproc_tab[0]->as, 1, 0, pproc_tab[0]->tss))
	//	return 1;

	if(initTD(pproc_tab[0]->td, "init", "\0", "\0"))
		return 1;

	//sync it up with the initial td.
	pproc_tab[0]->td->TD_CurX = pInitTD.TD_CurX;
	pproc_tab[0]->td->TD_CurY = pInitTD.TD_CurY;
	pproc_tab[0]->td->TD_VirtVid = pInitTD.TD_VirtVid;

	pInitTD.TD_ExitCode = 0;

	//pInitTD should NO LONGER BE USED!!!!!!!

	debug_printf("\n----: System task data info initialized.");

	if(initMsgBin(pproc_tab[0]->msgbin, pproc_tab[0]))
		return 1;

	debug_printf("\n----: System message bin initialized.");

	debug_printf("\nSystem process created.");

	return 0;
}

void zeroProcList()
{
	ULONG i;

	for(i = 0; i < max_procs; i++)
		pproc_tab[i] = NULL;

	clearCache();
}

void inline copyProcToList(Proc * proc, UINT offset)
{
	memcpy(pproc_tab[offset], proc, sizeof(Proc));
}

ULONG inline generatePID(ULONG start)
{
	//mutex

	char found = 0;
	UINT i;

	for(i = start; i < max_procs; i++)
	{
		if(pproc_tab[i] == NULL)
		{
			found = 1;
			break;
		}
	}

	if(!found) //bad error
	{
		for(i = 1; i < start; i++)
		{
			if(pproc_tab[i] == NULL)
			{
				found = 1;
				break;
			}
		}
	}

	if(!found)
		panic("PID generation failure.");

	return i;
}

void createNewProcTableEntry(ULONG pid)
{
	//mutex

	if(!proc_head)
	{
		proc_head = kmalloc(sizeof(Proc));

		if(proc_head == NULL)
			panic("Proc table entry creation failure.");

		proc_head->next = NULL;
		proc_head->prev = NULL;
		proc_tail = proc_head;

		proc_head->pid = pid;
		pproc_tab[pid] = proc_head;

		return;
	}

	Proc * new_proc = kmalloc(sizeof(Proc));

	new_proc->next = NULL;
	new_proc->prev = proc_tail;
	proc_tail->next = new_proc;
	proc_tail = new_proc;

	new_proc->pid = pid;
	new_proc->level = 5;

	pproc_tab[pid] = new_proc;
}

ULONG createNewProc(ULONG last_pid)
{
	//mutex

	ULONG pid = generatePID(last_pid);

	createNewProcTableEntry(pid);

	pproc_tab[pid]->in_use = 1;

	pproc_tab[pid]->tss = kmalloc(sizeof(TaskState));
	if(!pproc_tab[pid]->tss)
		panic("TSS allocation failed");

	pproc_tab[pid]->td = kmalloc(sizeof(TaskData));
	if(!pproc_tab[pid]->td)
		panic("TD allocation failed");

	pproc_tab[pid]->msgbin = kmalloc(sizeof(MsgBin));
	if(!pproc_tab[pid]->msgbin)
		panic("Message bin allocation failed");

	pproc_tab[pid]->as = kmalloc(sizeof(AddressSpace));
	if(!pproc_tab[pid]->as)
		panic("Address space allocation failed");

	return pid;
}

void destroyProc(ULONG pid)
{
	//mutex

	pproc_tab[pid]->in_use = 0;

	kfree(pproc_tab[pid]->tss);
	kfree(pproc_tab[pid]->td);
	kfree(pproc_tab[pid]->msgbin);
	kfree(pproc_tab[pid]->as);

	//remove from list
	destroyProcTableEntry(pid);
}

void destroyProcTableEntry(ULONG pid)
{
	//mutex

	Proc * dest = pproc_tab[pid];
	Proc *p = dest->prev;
	Proc *n = dest->next;

	p->next = n;
	n->prev = p;

	if(dest == proc_tail)
		proc_tail = proc_tail->prev;

	kfree(dest);
	dest = NULL;

	pproc_tab[pid] = NULL;
}

Proc * getProccessWithPID(ULONG pid)
{
	return pproc_tab[pid];
}

void CG_TaskSwitch(ULONG sel, ULONG pid)
{
	//Proc * proc = getProccessWithPID(pid);
	/*if(currentProc->pid == pid)
	{
		currentProc->tss->tick_last_run = GetCurrentTick();
		//__asm("leave");
		//__asm("lret");
	}	*/

	nSwitches++;

	Proc * proc = pproc_tab[pid];

	currentProc->in_use = 0;

	//if(!proc->tss->selector) //This proc is not in the GDT, so add it
//		forceToCache(proc->tss);

	//debug_printf("\n\n%d\n\n", (addr_t)proc->tss);

	if(!proc->tss->inCache)
		addTSStoCache(proc->tss);

	proc->tss->tick_last_run = GetCurrentTick();
	proc->in_use = 1;

	//debug_printf("\ncurrent pid: %d next_pid: %d ns: %d", currentProc->pid, proc->pid, proc->tss->selector);

	currentProc = proc;

	DoTaskSwitch(proc->tss->selector);

	__asm("leave");
	__asm("lret");
}
