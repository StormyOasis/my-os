#include "../include/cpp.h"
#include "../include/badtcpp.h"

extern "C" {
	
#include "../include/sched.h"	
#include "../include/util.h"
#include "../include/tss.h"

static Scheduler *sched = NULL;
extern Proc *currentProc;
extern Proc **pproc_tab;
extern ULONG MAX_PROCS;

ULONG InitIPCandScheduler()
{	
	debug_printf("\nCreating scheduler...");		
			
	sched = new Scheduler();
	if(sched == NULL)
		return ERC_ALLOCMEM_ERR;	
		
	debug_printf("\nScheduler created.");
	
	return 0;
}

void FixupReadyQueues()
{
	void *dqresult; 
	DequeueProc(sched->level4q[31]); //remove the first proc(the idle task) from the ready queue
	currentProc = (Proc*)getProccessWithPID(1);
	currentProc->tss->tick_last_run = GetCurrentTick();			
		
	if(!currentProc)
		panic("IPC Initialization Error");
}	

Scheduler::Scheduler()
{		
	UINT i;		
		
	debug_printf("\nZeroing ready queues...");
	
	level2q = NULL;
	NULL;
	
	level3q = NULL;
	NULL;
	
	
	for(i = 0; i < NUM_LEVEL_4_Q; i++)
	{
		level4q[i] = NULL;							
	}	
		
	debug_printf("Done.");
	
	debug_printf("\nCreating timer mutex...");
	//if(!(sched->timerMutex = CreateMutex(timerMutex)))	
	//	panic("Timer Mutex Creation Error.");	
	
	timerMutex = (Mutex *)kmalloc(sizeof(Mutex));
	
	//mutex = new Mutex;
		
	timerMutex->lock = 0;	
	
	debug_printf("Done.");
}

Proc * CG_GetNextProc(ULONG sel)
{
	Proc * res = NULL;
	
	res = sched->GetNextProc();
	
	//debug_printf("\ncg: %d", (addr_t)res);
	
	//__asm("mov %0, %%eax" : : "g"(res));
	//eax should already be set.
	__asm("leave");	
	__asm("lret");			
}

//This will return the proc that should be run next.
Proc * Scheduler::GetNextProc()
{
	//NOTE: By this time, currentProc should *always* have a valid entry.
	
	UINT i;
	ULONG pid = 0;		
	
	void *dqresult = NULL;
	
	//if the current proc is a level 2, then we don't need to do anything else, cuz it will run till finsihed
	if(currentProc->level <= 2)
		return NULL;
	
	if(level2q)
	{
		DequeueProc(level2q);//this is a  macro
		//debug_printf("\nLevel 2q.  pid: %d", (ULONG)dqresult);		
	}	
	else if(level3q)
	{
		//No waiting level 2 proc, so...
		//we want to round robin the level 3's, so we need to check to see if the current proc has run for a certain quanta.
		
		if(currentProc->level == 4) //go ahead and preempt the current(which is a lewel 4) proc
		{
			DequeueProc(level3q);
		}
		else
		{
			//The current proc is a level 3.
			
			//Check to see if it has run for a specified quanta.  If so, switch to the next l3 proc.  If not then just keep going.
			
			if((GetCurrentTick() - currentProc->tss->tick_last_run) > QUANTA)
			{
				DequeueProc(level3q);
			}
			else
			{
				return NULL; //don't switch..hasn't exceeded quanta
			}
		}
		
			
			
		//debug_printf("\nLevel 3q.  pid: %d", (ULONG)dqresult);
	}
	else if(currentProc->level == 4)
	{			
				
		//Since the high priorities will be tested first, they will run before the lower priorities, so no need to do anything special when testing quanta.
		//HOWEVER, this could lead to the lower prioriy tasks never running.
		if((GetCurrentTick() - currentProc->tss->tick_last_run) <= QUANTA)	
			return NULL; //don't switch..hasn't exceeded quanta							
				
		int pri = currentProc->tss->priority;
		
		pri++;
		
		if(pri >= NUM_LEVEL_4_Q)
			pri = 0;				
	
				
		for(i = pri; i < NUM_LEVEL_4_Q; i++)
		{						
			DequeueProc(level4q[i]);
			if(dqresult)					
				break;		
				
		//	debug_printf("\nAfter addr: %d",(addr_t)sched->level4q[i]);		
		}		
		
		if(!dqresult) //nothing was dqed
		{
			for(i = 0; i <= pri; i++)
			{
				DequeueProc(level4q[i]);
				if(dqresult)					
					break;						
			}
			
			//if nothing was dqed in the other q'a, try the current q			
			if(!dqresult)																
				return NULL;
	
		//debug_printf("\nLevel 4q.  pid: %d", (ULONG)dqresult);	
		}
	
	}
	else
		return NULL;
	
	
	//We should only make it to this point if a proc was dqed from the above code		
	
	pid = (ULONG)dqresult;
	
	if(pid == 0)					
		return NULL; //error	
		
	//debug_printf("\npid: %d", pid);		
	
		//if(pid == 2)
		//	halt();		
		
	return getProccessWithPID(pid);
	
}

ULONG CG_ScheduleProc(ULONG sel, Proc* proc)
{
	sched->ScheduleProc(proc);
	//eax should already be set
	__asm("leave");	
	__asm("lret");		
}

ULONG Scheduler::ScheduleProc(Proc * proc)
{
	if(!proc)	
		return 1;		
		
	proc->tss->tick_last_run = 0;	
		
	unsigned char level = proc->level;
	
	ULONG pid = proc->pid;
	
	if(level >= 5 || level <= 1)
		return 1;		
		
	if(level == 2)
	{
		if(!level2q)
			level2q = EnqueueProc(level2q, (void*)pid);
		else
			EnqueueProc(level2q, (void*)pid);
	}
	else if(level == 3)
	{
		if(!level3q)
			level3q = EnqueueProc(level3q, (void*)pid);
		else
			EnqueueProc(level3q, (void*)pid);
	}
	else
	{				
		if(proc->tss->priority > 31 || proc->tss->priority < 0)
			return 2;					
			
		//debug_printf("\nl4q S: %d", (addr_t)level4q);		
		
			
		if(!level4q[proc->tss->priority])
			level4q[proc->tss->priority] = EnqueueProc(level4q[proc->tss->priority], (void*)pid);
		else
			EnqueueProc(level4q[proc->tss->priority], (void*)pid);
						
			
		//debug_printf("\nl4q S: %d", (addr_t)level4q);
		
		//debug_printf("\nproc: %d, test: %d", (addr_t)pid, (ULONG)level4q[proc->tss->priority]->data);
	}	
	
	return 0;
}

Queue * Scheduler::GetFirstProcInQueue(char q, char pri)
{		
	if(q == 2)
		return level2q;
	else if(q == 3)
		return level3q;
	else if(q == 4)
	{
		if(pri > 31 || pri < 0)
			return NULL;
		
		return level4q[pri];
	}		
	return NULL;
}

void RemoveProcFromQueue(ULONG pid)
{
	sched->RemoveProcFromQueue(pid);
}

void Scheduler::RemoveProcFromQueue(ULONG pid)
{
	
	ListData  * node;
	UINT level = 2;
	int pri = -1;
	char found = 0;
	
searchstart:

	if(level == 2)
	{
		level++;
		node = level2q->head;
	}
	else if(level == 3)
	{
		level++;
		node = level3q->head;
	}	
	else if(level == 4)
	{
		if(pri < 32)
			pri++;
		else
			goto end;
			
		node = level4q[pri]->head;
	}
	
	while(node)
	{
		if((addr_t)node->data == pid)
		{
			found = 1;
			goto end;
		}	
		
		node = node->next;
	}	
	
	goto searchstart;
end:

	if(!found)
		return;
		
	ListData *c = node;
	
	//NOT SURE ABOUT THE FOLLOWING
	if(node->prev)
	{
		if(node->next)
			node->next->prev = c->prev;
		else
			node->next = NULL;
			
		node->prev->next = c->next;
	}
	else
	{		
		if(node->next)
			node->next->prev = NULL;
		else
			node->next = NULL;
		node->prev->next = NULL;
	}
		
	kfree(c);
	
	
	
/*	Queue *node;
	UINT level = 2;
	int i = -1;
	char found = 0;
	
searchstart:	
	if(level == 2)
	{
		level++;
		node = level2q;
	}
	else if(level == 3)
	{
		level++;
		node = level3q;
	}
	else	if(level == 4)
	{		
		if(i < 32)
			i++;
		else
			goto end;
			
		node = level4q[i];			
	}
		
	
	while(node)
	{
		//debug_printf("\nlevel: %d i %d node: %d", level, i, (addr_t)node);
		
		if((ULONG)node->data == pid)
		{					
			found = 1; //done
			goto end;
		}
		
		node = node->next;
	}
			
	goto searchstart;
end:		

	if(!found)
		return;
	//node is the one we want to remove from the list	
	
	//debug_printf("\nnode %d", (addr_t)node);
	
	if(node->prev)
		node->prev->next = node->next;
	
	if(node->next)
		node->next->prev = node->prev;
		
	node->next = node->prev = NULL;		
	
	kfree(node);	
*/
}

//debuggin purposes only
void SDG()
{
		currentProc = (Proc*)getProccessWithPID(1);
		
		//currentProc = (Proc*)GetNextProc();
		
		if(!currentProc)
			panic("Timer Initialization Error");	
	void * dqresult;
	int x = 0;
	ListData * node = sched->level4q[31]->head;
	debug_printf("\nQueuing test...\n");
	
		DequeueProc(sched->level4q[31]); //remove the first proc(the idle task) from the ready queue
		currentProc = (Proc*)getProccessWithPID(1);
		
		//currentProc = (Proc*)GetNextProc();				
		currentProc->tss->tick_last_run = GetCurrentTick();	
	
	while(x < 1500)
	{
		
		for(int i = 0; i < 3; i++)
		{
			debug_printf("%d", currentProc->pid);
		
			Proc * next_proc = (Proc*)GetNextProc();
		
			if(!next_proc) 
				debug_printf("...No Next Proc");
	
			if(ScheduleProc(currentProc))
				debug_printf("....SC ERROR");
			
			currentProc = next_proc;									
		}
		
		debug_printf("|");
		/*ListData * node = sched->level4q[31]->head;		
	
		debug_printf("\n--------------");
		debug_printf("\nPass: %d", x);
		while(node)
		{
			debug_printf("\nPID: %d", (ULONG)node->data);
			if((ULONG)node->data > 3 || (ULONG)node->data == 0)
			{
				//Error this is where we are gpfing
				debug_printf("Error! node->data = %d", (ULONG)node->data);
				while(1);
			}
			node = node->next;
		}*/		
		
				
		x++;
	}
	
	while(1);
}

ULONG intTimerHandler()
{	
	//return a pid to switch to or 0 if no switch.		
	
	if(nSwitches == 0) //System is not setup yet...
		return NULL;			
	
	//EnterCriticalSection(sched->timerMutex);			
			
	
	//debug_printf("\nBefore next_proc");
	Proc * next_proc = (Proc*)GetNextProc();				
	
	if(!next_proc)
	{
		//debug_printf("\nNo next proc");
		//currentProc->tss->tick_last_run = GetCurrentTick();
		
		//LeaveCriticalSection(sched->timerMutex);
		return NULL;	
	}
	
	if(next_proc->pid == currentProc->pid)
		panic("PID error");		
	
	//Queue up the old(currentProc) proc.
	if(ScheduleProc(currentProc))
		panic("Panic! Scheduler error.");	
		
	//next_proc->tss->tick_last_run = GetCurrentTick();
	
	currentProc = next_proc;
			
	
	if(next_proc->pid > 5)
	{
		//debug_printf("\nNext PID: %d", next_proc->pid);
		//return NULL;
		panic("PID Error 2");
	}
	
	//LeaveCriticalSection(sched->timerMutex);
	
	//debug_printf("%d", next_proc->pid);
	
	nSwitches++;
	
	//if(nSwitches > 3500)
	//	halt();
	
	return next_proc->pid;			
				
			
	//if(ScheduleProc(currentProc))
	//	return (MAX_PROCS+1);		
	
	//debug_printf("\ncurrentPID: %d nextPID %d next->selector %d", currentProc->pid, next_proc->pid, next_proc->tss->selector);
		
	//return next_proc->pid;		
	
	//debug_printf("\nAfter next_proc");
		
	//if(debug)	
	//debug_printf("\nnext_proc: %d currentProc: %d", (addr_t)next_proc, (addr_t)currentProc);
		
	//debug_printf("\n%d, %d", GetCurrentTick(), currentProc->tss->tick_last_run);
		
	/*if(currentProc->level == 4 && next_proc->level == 4)
	{
		//if(ScheduleProc(currentProc))
			//return  next_proc->pid;
		
		//This is a user proc(level 4), since it can't be preempted by another level 4, test
		//to see if it has run for the specified qunata.
		if((GetCurrentTick() - currentProc->tss->tick_last_run) >= QUANTA)
		{
			//time for a change								
		
			//compare next_proc's priority against the currently executing proc.  
			if(next_proc->tss->priority <= currentProc->tss->priority)
			{
				//switch to new proc.
				//queue the old(current) proc
				if(ScheduleProc(currentProc))
					panic("Panic! Scheduler error.");
			}
			else
			{
				//keep going
				currentProc->tss->tick_last_run = GetCurrentTick();
				ScheduleProc(next_proc);
				return (MAX_PROCS+1);
			}
		}
		else
		{
			//keep going
			currentProc->tss->tick_last_run = GetCurrentTick();
			ScheduleProc(next_proc);
			return (MAX_PROCS+1);
		}
	}	
	else
	{
		//The current task may or may not be a level 4, but does not matter.
		//Run which ever task, whether be currentProc or next_proc, that has 
		//the most privileged run level(lower number = higher level).		
		if(currentProc->level > next_proc->level)
		{
			//time to change...
			if(ScheduleProc(currentProc))
				panic("Panic! Scheduler error.");
		}
	}*/	
	
	//return next_proc->pid;		
}

char isNextProc(ULONG pid)
{
	UINT i;
	
	if(!currentProc)
		return 1;
			
	if(currentProc->level == 1 || currentProc->level == 2)
		return 0;
		
	Proc * proc = pproc_tab[pid];
	
	if(!proc)
		return 0;
		
	//First check all queues with a higher priority then that of pid.
	//If there is a waiting queue, then pid is not next.		
	
	switch(proc->level)
	{
		case 2:
			if(sched->GetFirstProcInQueue(2,0))
				return 0;		
		case 3:
			if(sched->GetFirstProcInQueue(2,0))
				return 0;			
			if(sched->GetFirstProcInQueue(3,0))
				return 0;
		case 4:
			if(sched->GetFirstProcInQueue(2,0))
				return 0;			
			if(sched->GetFirstProcInQueue(3,0))
				return 0;
			for(i = 0; i <= proc->tss->priority; i++)
			{
				if(sched->GetFirstProcInQueue(4,i))
					return 0;
			}
		default:
			return 0;						
	}
	
		
	if(currentProc->level >  proc->level)
		return 0;
	else if(currentProc->level < proc->level)
		return 1;
	else //same level either level 3 or 4
	{
		//a level 2 task can't interrupt another level 2
		//a level 3 can interrupt another level 3
		//we need to check quanta for both level 3 and level 4.
		//if level 4, check priority first, to save time
		if(currentProc->level == 4)
		{
			if(currentProc->tss->priority < proc->tss->priority)
				return 0;
			else if(currentProc->tss->priority > proc->tss->priority)
				return 1;
		}
		
		//At this point, proc and currentProc are the same level and
		//if level4, are the same priority. All we need to do is to check
		//the quanta.
		
		if((GetCurrentTick() - currentProc->tss->tick_last_run) > QUANTA)
			return 1;
	}
		
	return 0;	
		
}

} //extern "C"
