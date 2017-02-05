#include "../include/extern.h"
#include "../include/kernel.h"
#include "../include/tss.h"
#include "../include/proc.h"
#include "../include/util.h"

#ifdef __cplusplus
extern "C" {
#include "../include/sched.h"
}
#endif

extern Proc ** pproc_tab;
extern Proc * currentProc;

static unsigned long NumActiveProcs = 0;
static unsigned long LastProcCreated = 0; //pid of last proc created.

ULONG CG_CreateProcess(ULONG codesel, void * name, void *path,  void * parm ,void * pEntry, DWORD pri, DWORD level, ULONG *result)
{
	void * stack = NULL;

	if(pri > 31 || pri < 0)
	{
		*result = ERC_INVALID_PRIORITY;

		__asm("mov $0, %eax");
		__asm("leave");
		__asm("lret");
	}

	//check to see if the number of active process is less than MAX_PROCS
	if(NumActiveProcs == max_procs)
	{
		*result = ERC_PROC_NUM_EXCEEDED;

		//EnableInterrupts();

		__asm("mov $0, %eax");
		__asm("leave");
		__asm("lret");
	}

	debug("1");
	ULONG pid = createNewProc(LastProcCreated+1);

	Proc * p = getProccessWithPID(pid);

	p->level = level;

	debug("2");
	//NOTE: Commented out pending rework of kmalloc()
//	if(CreateAddressSpace(p->as, 0))
	{
		*result = ERC_ADDR_SPACE_ALLOC_FAIL;

		destroyProc(pid);

		//EnableInterrupts();

		__asm("mov $0, %eax");
		__asm("leave");
		__asm("lret");
	}

	debug("3");
	if(initTSS(p->tss, pid, 1, (char)pri, 3, 0x10, p->as->page_dir))
	{
		*result = ERC_BAD_TSS;

		destroyProc(pid);

		//EnableInterrupts();

		__asm("mov $0, %eax");
		__asm("leave");
		__asm("lret");
	}


	debug("4");
	stack = kmalloc(STANDARD_STACK_SIZE);

	if(stack == NULL)
	{
		*result = ERC_ALLOCMEM_ERR;

		destroyProc(pid);

		__asm("mov $0, %eax");
		__asm("leave");
		__asm("lret");
	}

	p->tss->eip = (addr_t)(pEntry);
	p->tss->esp = (addr_t)(stack);
	p->tss->esp0 = (addr_t)(stack);

	debug("5");
	if(initTD(p->td, (char*)name, (char*)path, (char*)parm))
	{
		*result = ERC_UNALLOCATED_TD;

		destroyProc(pid);

		//EnableInterrupts();

		__asm("mov $0, %eax");
		__asm("leave");
		__asm("lret");
	}


	p->tss->selector = 0;
	p->pid = pid;

	debug("6");
	if(initMsgBin(p->msgbin, p))
	{
		*result = ERC_UNALLOCATED_MSGBIN;

		destroyProc(pid);

		//EnableInterrupts();

		__asm("mov $0, %eax");
		__asm("leave");
		__asm("lret");
	}

	//Since we just created this task, we assume that we want to run it(duh)...
	//Therefore, add it to the tss cache and enqueue it.

	debug("7");
	addTSStoCache(p->tss);

	p->tss->cr3 = (addr_t)(page_dir); //for testing purposes now...(everything is mapped in 1 addr space)

	debug("8");
	if(ScheduleProc(p))
		panic("Scheduler error.");

	debug("9");
	NumActiveProcs++;
	LastProcCreated = pid;

	*result = 0;

	//do the actual far(call gate) return
	__asm("mov %0, %%eax" : : "a"(pid));
	__asm("leave");
	__asm("lret");
}

ULONG CG_SendMessage(ULONG sel, ULONG pid, ULONG message, void * proc, void * data1, void *data2)
{
	DisableInterrupts();

	//debug_printf("\nmsg: %d, proc: %d pid: %d pri: %d", message, (addr_t)pproc_tab[pid], pid, pproc_tab[pid]->tss->priority);

	if(pproc_tab[pid] == NULL) //error
	{
		//debug_printf("\nHere2");
		EnableInterrupts();
		__asm("mov $1, %eax");
		__asm("leave");
		__asm("lret");
	}

	if(!pproc_tab[pid]->msgbin) //error
	{
		//debug_printf("\nHere3");
		EnableInterrupts();
		__asm("mov $1, %eax");
		__asm("leave");
		__asm("lret");
	}

	//debug_printf("\nBefore PM");

	//Add a new link node block...This needs to be treated as a Queue
	//But we can't use the kernel queue structure because we need the
	//LinkNode structure.  A C++ template would come in very handy here!

	LinkNode * msg = NULL;

	msg = (LinkNode*)PeekMessage(pproc_tab[pid]->msgbin);

	if(!msg)
	{
		//debug_printf("\nNothing waiting");
		//Nothing is waiting, just add this message
		if(AddMessage(pproc_tab[pid]->msgbin, pid, message, proc, data1, data2))
		{
			EnableInterrupts();
			__asm("mov $1, %eax");
			__asm("leave");
			__asm("lret");
		}
	}
	else
	{
		//something was waiting at the msgbin, either a message or a proc
		if(msg->proc) //proc
		{
			//debug_printf("\nProc waiting");

			//if a proc was waiting, add the message and switch to the proc.
			if(AddMessage(pproc_tab[pid]->msgbin, pid, message, proc, data1, data2))
			{
				EnableInterrupts();
				__asm("mov $1, %eax");
				__asm("leave");
				__asm("lret");
			}

			if(proc == NULL)
			{
				//We just added a message not another proc, so since
				//there was a waiting proc, we can run it if it has
				//the highest priority.

				/*if(isNextProc(((Proc*)msg->proc)->pid))
				{
					if(!ScheduleProc(currentProc))
						TaskSwitch(((Proc*)msg->proc)->pid);
				}
				else	*/
					ScheduleProc(((Proc*)msg->proc));
			}
		}
		else
		{
			//debug_printf("\nMessage waiting");
			//if a message is waiting, then queue the message but don't switch.
			if(AddMessage(pproc_tab[pid]->msgbin, pid, message, proc, data1, data2))
			{
				EnableInterrupts();
				__asm("mov $1, %eax");
				__asm("leave");
				__asm("lret");
			}
		}
	}

	EnableInterrupts();
	__asm("mov $0, %eax");		//success!
	__asm("leave");
	__asm("lret");
}


ULONG CG_WaitMessage(ULONG sel, MSG *message)
{
	MSG msg;
	Proc * next = NULL;
	Proc * proc = currentProc;

	if(!proc || !message)
	{
		__asm("mov $1, %eax");		//error
		__asm("leave");
		__asm("lret");
	}

	//First try to get a message off of the message bin.  If none is waiting, then
	//queue up the proc into the msg bin.

	DisableInterrupts();
	if(GetMessage(proc->msgbin, msg))
	{
		//no message is waiting
		//make this process wait as well

		msg.proc = proc;
		msg.LN_Msg = MSG_PROC;
		msg.LN_Data1 = NULL;
		msg.LN_Data2 = NULL;

		AddMessage(proc->msgbin, proc->pid, MSG_PROC, proc, NULL, NULL);

		debug_printf("\nwait1");

		//The current process is now waiting.  We need to take it off the ready queue.

		RemoveProcFromQueue(proc);

		debug_printf("\nwait2");

		//Since this task is now waiting, we have to force a task switch.

		while(1)
		{
			next = (Proc*)GetNextProc();
			if(!next)
			{
				//nothing is waiting to run. Guess we have nothing to do //Probably don't want to do this!!!!
				EnableInterrupts();
				__asm("hlt");
				//something has happened, so clear ints and resume.
				DisableInterrupts();
			}
			else
				break;
		}

		debug_printf("\nwait3");

		//There is now someone who wants to run, so let him.
		EnableInterrupts();
		TaskSwitch(next->pid);
		DisableInterrupts();

		debug_printf("\nwait4");

		//When we are back here, there should be a waiting message.
		//So process it.

		GetMessage(proc->msgbin, msg);
		//assume the call was successful for now
	}

	//At this point, one of two things has happened.  Either there was a waiting
	//message and we are processing it, or there wasn't a message, and we switched
	//tasks, then returned here.  At this point msg is what we want to process, so
	//return it through message.

	EnableInterrupts();

	message->proc = msg.proc;
	message->LN_Msg = msg.LN_Msg;
	message->LN_Data1 = msg.LN_Data1;
	message->LN_Data2 = msg.LN_Data2;

	__asm("mov $0, %eax");
	__asm("leave");
	__asm("lret");

}



/*UINT enQueueTSS()
{
	//place the tss pointed to by tss onto the ready queue

	//convert into tss struct

	TaskState *tss = pCurrentTSS;

	/*if(tss == NULL)
	{
		__asm("mov $5000, %eax");

		//do the actual far return
		__asm("leave");
		__asm("lret");
		//return ERC_INVALIDTSS;
	}* /

	//we tried to enqueue the os tss..dont error, just return
	if(tss.num == 0)
	{
		__asm("mov $5000, %eax");

		//do the actual far return
		__asm("leave");
		__asm("lret");
	}

	//	SysTD->TD_CurX = 0;
	//	SysTD->TD_CurY = 1;
	//printf("%u ", tss->num);

	//first, get the priority of the new tss

	if(tss.pri < 0 || tss.pri > 31)	//invalid priority
	{
		__asm("mov $5002, %eax");
		//do the actual far return
		__asm("leave");
		__asm("lret");
	}


	//we want to enqueue tss
	//add it to the tail
	//fixup the current tail's next ptr to the new tss
	//the new tss's next ptr = null

	//this is the last one in the queue
	tss.next = NULL;

	 //then this will be the first and only tss on this q
	if(rq.queue[tss.pri].tail == NULL || rq.queue[tss.pri].head == NULL)
		rq.queue[tss.pri].head = rq.queue[tss.pri].tail = &tss;
	else
	{
		//add to tail
		rq.queue[tss.pri].tail->next = &tss;
	}

	//TaskState *temp = (TaskState*)rq.queue[tss->pri].head;

		//SysTD->TD_CurX = 0;
		//SysTD->TD_CurY = 1;
	//printf("%u", temp->num);


	nRdyQ++;

	__asm("mov $0, %eax");

	//do the actual far return
	__asm("leave");
	__asm("lret");
}

//get the address of the highest priority level tss waiting at the q
TaskState * deQueueTSS()
{
	int i;

	TaskState *addr;
	int found = 0;

	for(i = 0; i < 32; i++)
	{
		if(rq.queue[i].head != NULL)
		{
		   found = 1;
		   break;
		}
	}

	if(!found) //return NULL
	{
		__asm("mov $0, %eax");

		//do the actual far return
		__asm("leave");
		__asm("lret");
	}

	//the highest level task is waiting at PQ number (i)
	//rq.queue[i].head is the highest priority tss

	addr = rq.queue[i].head;

	rq.queue[i].head = ((TaskState*)rq.queue[i].head)->next;

	//just a little housekeeping
	if(rq.queue[i].head == NULL)
		rq.queue[i].tail = NULL;

	nRdyQ--;

	addr->next = NULL;

	__asm("mov %0, %%eax" : : "a"((addr)));

	//do the actual far return
	__asm("leave");
	__asm("lret");

	//return addr;
}*/

/*
UINT SendMsg(MsgBin *to, ULONG msg, ULONG param1, ULONG param2)
{

	if(to == NULL)
	{
		__asm("mov $5001, %eax");
		__asm("leave");
		__asm("lret");
		//return ERC_UNALLOCATED_MSGBIN;
	}

	UINT res = 0;

	//allocate link node
	LinkNode link;
	link.LN_Handle = (addr_t*)(to);
	link.LN_Type = LB_TYPE_MSG;
	link.LN_Data1 = msg;
	link.LN_Data2 = param1;
	link.LN_Data3 = param2;
	link.LN_Next = NULL;
	link.LN_Prev = to->MB_Tail;

	//send and adjust the linked list/queue at the to bin
	to->MB_Tail = &link;

	//the msg has now been "sent"..perform a task switch if necessary

	//enqueue the task onto the ready queue
	//however, this new msg might not necessarily mean the task is ready to be
	//put in a ready state..hmmm...what to do...

	//reevaluate the queue
	TaskState * state = (TaskState*)CheckReadyQueue();

	if(state == NULL)
	{
		//no task are on the queue..put this one on it
		res =  enQueueTSS((addr_t*)&to->MB_Owner);
	}
	else
	{
		//addr is the tss of the msgbin
		//loop through the list of tss's until a match is found..if a match is found, then
		//just return.  otherwise, queue it up

		TaskState *tss = pTssHead;
		int found = 0;

		while(tss)
		{
			if(tss == (TaskState*)to->MB_Owner)
			{
				found = 1;
				break;
			}
			tss = tss->next;
		}

		if(found)
			res = 0;
		else
		{
			//queue it up...
			//res = enQueueTSS((addr_t*)tss);
		}
	}

	__asm("mov %0, %%eax" : : "g"(res));
	__asm("leave");
	__asm("lret");

	return res;
}

LinkNode* WaitMsg(MsgBin *bin)
{
	LinkNode *link;

	__asm ("cli");

	printf("\n\n%u, %u", bin, bin);

	if(bin == NULL)
	{
		__asm("mov $0, %eax");
		__asm("leave");
		__asm("lret");
	//	return NULL;
	}

	link = (LinkNode*)bin->MB_Head;
	//link is now the head of the msg queue

	if(link != NULL)
	{
		((LinkNode*)bin->MB_Head) = (LinkNode*)link->LN_Next;
		(LinkNode*)link->LN_Prev = NULL;

		//there is a message waiting get the msg and continue executing...
	}
	else
	{
		//there is no msg waiting.
		//reevealuate ReadyQueue by calling dequetss
		//this should get the highest priority level tss or null if none

		TaskState *tss;

		//should I call this through a call gate???

		//if there are no waiting tss's, then halt the system and wait for an interrupt
		while((tss = (TaskState*)deQueueTSS()) == NULL)
		{
			bHalted = 1;

			//halt with interrupts enabled
			EnableInterrupts();
			__asm("hlt");
			//an interrupt has occured and execution began here again

			DisableInterrupts();
			bHalted = 0;
		}

		//time for a task switch???

		if(tss != pCurrentTSS)
		{
			//task switch

			DisableInterrupts();

			pCurrentTSS = tss;

			DoTaskSwitch();

		}
	}

	EnableInterrupts();

	__asm("mov %0, %%eax" : : "g"(link));
	__asm("leave");
	__asm("lret");

	//return link;
}

LinkNode* PeekMsg(MsgBin *bin)
{
	//again, not sure about this...
	addr_t *addr = (addr_t*)bin->MB_Head;
	__asm("mov %0, %%eax" : : "g"(addr[0]));
	__asm("leave");
	__asm("lret");

	//return (LinkNode*)bin->MB_Head;
}


//returns a pointer to the addr of the highest priority tss struct but leaves it on the queue
addr_t * CheckReadyQueue()
{
	int i = 0;
	int found = 0;

	// be sure to null out the ready queue when empty
	for(i = 0; i < 32; i++)
	{
		if(rq.queue[i].head != NULL)
		{
			found = 1;
			break;
		}
	}

	if(!found) //return NULL
		__asm("mov $0, %eax");
	else //return the address
	{
		__asm("mov %0, %%eax" : : "a"(rq.queue[i].head));
	}

	//do the actual far return
	__asm("leave");
	__asm("lret");
}*/

//UINT CreateTask(void *pEntry, void *pStack, char pri, int os, char qlevel, MsgBin * ret_bin)
//{
	/*TaskData *td = NULL;
	TaskState *tss = NULL;
	addr_t *pd = NULL;
	//addr_t *pt = NULL;

	if(pri < 0 || pri > 31)
		return ERC_INVALID_PRIORITY;

	ret_bin = CreateMsgBin();
	if(ret_bin == NULL)
		return ERC_UNALLOCATED_MSGBIN;

	td = CreateNewTD("Task");
	if(td == NULL)
	{
		//free the msg bin
		DeallocMsgBin(ret_bin);

		return ERC_UNALLOCATED_TD;
	}

/*	if(MemAlloc(0, pd))
	{
		//free the td and msg bin
		DeallocMsgBin(ret_bin);
		return ERC_CANT_ALLOCPD;
	}*/

	/*if(MemAlloc(0, pt))
	{
		//free the td and msg bin
		DeallocMsgBin(ret_bin);
		MemFree((ULONG)pd);
		return ERC_CANT_ALLOCPD;
	}*/

	//tss = CreateNewTSS(qlevel);
	/*if(tss == NULL)
	{
		MemFree((ULONG)pd);
		DeallocMsgBin(ret_bin);

		return ERC_BAD_TSS;
	}*/

	/*td->TD_AddrPD = pd;
	td->TD_CurX = 0;
	td->TD_CurY = 0;
	td->TD_nColumns = 80;
	td->TD_nLines = 25;
	td->TD_VirtVid = (void*)0xb8000;
	td->TD_VidMode = 0;
	td->TD_NormVid = 7;

	tss->id = 9999;//selector number(changes as it is moved in and out off the gdt/aTSS)

	tss->num = td->TD_Num;
	tss->owner = td;
	tss->msgbin = ret_bin;
	tss->used = tss->active = 1;
	tss->tick = 9999999;
	tss->lasttime = 0;
	tss->incache = 0;

	tss->map_base = 0x0ffff;

	if(os)
		tss->cs = OsCodeSel;
	else
		tss->cs = UserCodeSel;

	tss->eip =  (ULONG)(pEntry);
	tss->esp0 = tss->esp = (ULONG)(pStack);

	tss->owner = td;

	//a new task needs a new page directory

	tss->cr3 = (ULONG)pd;

	tss->eflags = 0x0202;

	tss->pri = pri;

	//task is created...queue it up now????

	return 0;*/
//}

//TaskData* CreateNewTD(const char *str)
//{
	//are there any reentracy problems here????

/*	TaskData *td = NULL;
	TaskData *newtd = NULL;
	TaskData *temp = sched->td_head;
	char x;
	int i = -1, w,z;

	while(temp)
	{
		//all spots in this page are open, so use it. This is not likely to happen
		if(!temp->TD_Full)
		{
			i = 0;
			break;
		}
		else if(temp->TD_Full == 255) //all spots are taken, go to next page
		{
			td = temp;
			temp = temp->TD_NextPage;
			continue;
		}

		//if it makes it here, then there is an aloocated page with an empty slot available
		//so find the empty slot
		x = temp->TD_Full;

		for(i = 0; i < 8; i++)
		{
			if(!(x & 1))
				break;

			x = x >> 1;
		}

		break;

		//i is the number of the bit of the first 0(first open location in the page)
	}

	//if temp is NULL, then we did not find an open spot. we need to allocate a new page
	if(temp == NULL)
	{
		if(MemAlloc(0, temp))
			return NULL;

		//fix up the page pointers(temp is new page, td is the last page)
		temp->TD_PrevPage = td;
		td->TD_NextPage = temp;
		temp->TD_NextPage = NULL;
		temp->TD_Full = 0;
		temp->TD_HomePage = temp;

		newtd = temp;
	}
	else //we found an open spot at the page pointed to by temp. Dont forget to flip the bit in TD_Full
	{
		//td is the open spot, temp is the first spot on the page
		td = (TaskData*)(temp + i);

		z = 1;

		for(w = 0; w < i; w++)
			z = z * 2;

		z = (x ^ z);

		temp->TD_Full = z;

		td->TD_HomePage = temp;
		td->TD_NextPage = temp->TD_NextPage;
		td->TD_PrevPage = temp->TD_PrevPage;

		newtd = td;
	}

	newtd->TD_Num = sched->td_num++;

	strcpy(newtd->TD_Name, str);

	return newtd;	*/
//}

//TaskState * CreateNewTSS(char q)
//{
/*	TaskState *tss = NULL;
	TaskState *newtss = NULL;
	TaskState *temp = NULL;
	TaskState *head = NULL;
	char x;
	int i = -1, w,z;

	if(q == 2)
	{
		head = temp = sched->level2_head;

		while(temp)
		{
			//all spots in this page are open, so use it. This is not likely to happen
			if(!temp->full)
			{
				i = 0;
				break;
			}
			else if(temp->full == 255) //all spots are taken, go to next page
			{
				tss = temp;
				temp = temp->nextpage;
				continue;
			}

			//if it makes it here, then there is an aloocated page with an empty slot available
			//so find the empty slot
			x = temp->full;

			for(i = 0; i < 8; i++)
			{
				if(!(x & 1))
					break;

				x = x >> 1;
			}

			break;

			//i is the number of the bit of the first 0(first open location in the page)
		}

		//if temp is NULL, then we did not find an open spot. we need to allocate a new page
		if(temp == NULL)
		{
			if(MemAlloc(0, temp))
				return NULL;

			//fix up the page pointers(temp is new page, td is the last page)
			temp->prevpage = tss;
			tss->nextpage = temp;
			temp->nextpage = NULL;
			temp->full = 0;
			temp->homepage = temp;

			newtss = temp;
		}
		else //we found an open spot at the page pointed to by temp. Dont forget to flip the bit in TD_Full
		{
			//tss is the open spot, temp is the first spot on the page
			tss = (TaskState*)(temp + i);

			z = 1;

			for(w = 0; w < i; w++)
				z = z * 2;

			z = (x ^ z);

			temp->full = z;

			tss->homepage = temp;
			tss->nextpage = temp->nextpage;
			tss->prevpage = temp->prevpage;

			newtss = tss;
		}
	}
	else if(q == 3)
	{
		head = temp = sched->level3_head;

		while(temp)
		{
			//all spots in this page are open, so use it. This is not likely to happen
			if(!temp->full)
			{
				i = 0;
				break;
			}
			else if(temp->full == 255) //all spots are taken, go to next page
			{
				tss = temp;
				temp = temp->nextpage;
				continue;
			}

			//if it makes it here, then there is an aloocated page with an empty slot available
			//so find the empty slot
			x = temp->full;

			for(i = 0; i < 8; i++)
			{
				if(!(x & 1))
					break;

				x = x >> 1;
			}

			break;

			//i is the number of the bit of the first 0(first open location in the page)
		}

		//if temp is NULL, then we did not find an open spot. we need to allocate a new page
		if(temp == NULL)
		{
			if(MemAlloc(0, temp))
				return NULL;

			//fix up the page pointers(temp is new page, td is the last page)
			temp->prevpage = tss;
			tss->nextpage = temp;
			temp->nextpage = NULL;
			temp->full = 0;
			temp->homepage = temp;

			newtss = temp;
		}
		else //we found an open spot at the page pointed to by temp. Dont forget to flip the bit in TD_Full
		{
			//tss is the open spot, temp is the first spot on the page
			tss = (TaskState*)(temp + i);

			z = 1;

			for(w = 0; w < i; w++)
				z = z * 2;

			z = (x ^ z);

			temp->full = z;

			tss->homepage = temp;
			tss->nextpage = temp->nextpage;
			tss->prevpage = temp->prevpage;

			newtss = tss;
		}
	}
	else if(q == 4)
	{
		head = temp = sched->level4_head;

		while(temp)
		{
			//all spots in this page are open, so use it. This is not likely to happen
			if(!temp->full)
			{
				i = 0;
				break;
			}
			else if(temp->full == 255) //all spots are taken, go to next page
			{
				tss = temp;
				temp = temp->nextpage;
				continue;
			}

			//if it makes it here, then there is an allocated page with an empty slot available
			//so find the empty slot
			x = temp->full;

			for(i = 0; i < 8; i++)
			{
				if(!(x & 1))
					break;

				x = x >> 1;
			}

			break;

			//i is the number of the bit of the first 0(first open location in the page)
		}

		//if temp is NULL, then we did not find an open spot. we need to allocate a new page
		if(temp == NULL)
		{
			if(MemAlloc(0, temp))
				return NULL;

			//fix up the page pointers(temp is new page, td is the last page)
			temp->prevpage = tss;
			tss->nextpage = temp;
			temp->nextpage = NULL;
			temp->full = 0;
			temp->homepage = temp;

			newtss = temp;
		}
		else //we found an open spot at the page pointed to by temp. Dont forget to flip the bit in TD_Full
		{
			//tss is the open spot, temp is the first spot on the page

			tss = (TaskState*)(temp + i);

			z = 1;

			for(w = 0; w < i; w++)
				z = z * 2;

			z = (x ^ z);

			temp->full = z;

			tss->homepage = temp;
			tss->nextpage = temp->nextpage;
			tss->prevpage = temp->prevpage;

			newtss = tss;
		}
	}
	else
		return NULL;

	//now fixup the next and prev pointers

	temp = head;

	//just loop to the end
	while(temp)
	{
		if(temp->next == NULL)
			break;

		temp = temp->next;
	}

	if(temp == NULL)
	{
		debug("\ntemp == NULL\n");
		return NULL;
	}

	//debug("\n\ntemp->next = newtss");
	temp->next = newtss;
	//debug("\n\nnewtss->prev = temp");
	newtss->prev = temp;
	//debug("\n\nnewtss->next = NULL");
	newtss->next = NULL;

	return newtss;
	*/
//}

/*
UINT SpawnThread(addr_t *pEntry, addr_t *pStack, char Pri, int os, MsgBin * ret_bin)
{
	//invalid priority
	if(Pri < 0 || Pri > 31)
	{
		//__asm("mov $5002, %eax");
		//__asm("leave");
		//__asm("lret");
		return ERC_INVALID_PRIORITY;
	}

	//kill the ints to avoid rentrancy problems right now
	DisableInterrupts();

	MsgBin *msgbin = NULL;// = (addr_t*)call_gate((int) 0x78);
	__asm("lcall $0x78, $0x00" : "=a"(msgbin));
	//__asm("mov %%eax, %0" : :"a"(t));

	if(msgbin == NULL)
	{
		//unallocated msgbin
		//__asm("mov $5001, %eax");
		//__asm("leave");
		//__asm("lret");
		return ERC_UNALLOCATED_MSGBIN;
	}

	ret_bin = msgbin;

	if(nTSSNum+1 > MAX_TSS || pNextTSS == NULL)
	{
		//unallocate the msgbin and error
		//assuming it is still in eax
		//call_gate((int) 0x80);

		__asm("lcall $0x80, $0x00");

		EnableInterrupts();

		//__asm("mov $5000, %eax");
		//__asm("leave");
		//__asm("lret");

		return ERC_INVALIDTSS;
	}

	//ptr will serve as a pointer to the tss in the static tss array

	TaskState *ptr = pNextTSS;

	pNextTSS = NULL;

	//ptr should now point to the last allocated tss struct
	//create the new struct and fixup the next ptr

	UINT i;
	//when a tss is deallocated, the owner value MUST be zeroed

	//POSSIBLE OFF BY ONE ERROR BELOW..(SEE ARROWS)
	//we assume here that there are enough tss's
	for(i = 1; i < MAX_TSS; i++)
	{
		if(aTSS[i].owner == 0) //if it is 0, then the tss is not allocated
		{
			pNextTSS = &aTSS[i]; //   <---Maybe add SIZEOF_TSS also
			break;
		}
	}

	//ptr now points to the start of the new tss struct

	ptr->msgbin = (void*)(msgbin);

	if(os)
		ptr->cs = OsCodeSel;
	else
		ptr->cs = UserCodeSel;

	ptr->eip =  (addr_t)&(*(addr_t*)(pEntry));
	ptr->esp0 = ptr->esp = (addr_t)&(*(addr_t*)(pStack));

	ptr->owner = (void*)(((TaskState*)pCurrentTSS)->owner);

	ptr->cr3 = pCurrentTSS->cr3;

	ptr->eflags = 0x0202;

	ptr->pri = Pri;

	nTSSNum++;

	UINT res = 0;
	//tss is ready
	//check to see if new task is of higher priority..if so, then switch to it

	if(pCurrentTSS->pri < Pri)
	{
		//just enqueue the new task
		TaskState *t = pCurrentTSS;
		pCurrentTSS = ptr;

		__asm("lcall $0xa8, $0x00" : "=a"(res));
		pCurrentTSS = t;

		//UINT res = (UINT)call_gate((int) 0xa8);
		//res = enQueueTSS(ptr);
		if(res)
		{
			EnableInterrupts();
			//__asm("mov $-1, %eax");
			//__asm("leave");
			//__asm("lret");

			return res;
		}
	}
	else
	{
		//perform a task switch to the new task	and enqueue the current task
		res = 0;

		if(pCurrentTSS->num != 0)
			__asm("lcall $0xa8, $0x00" : "=a"(res));
		//UINT res = (UINT)call_gate((int) 0xa8);

		//res = enQueueTSS((addr_t*)pCurrentTSS);

		if(res)
		{
			EnableInterrupts();
			//__asm("mov $-1, %eax");
			//__asm("leave");
			//__asm("lret");

			return res;
		}

		pCurrentTSS = ptr;

		nSwitches++;

		SwitchTick = TickCount;

		DoTaskSwitch();
	}

	//EnableInterrupts();
	//__asm("mov $0, %eax");
	//__asm("leave");
	//__asm("lret");

	return 0;
}*/

/*
UINT CreateIdleTask(addr_t *pEntry, addr_t* pStack, char Pri, int os, MsgBin * bin)
{
	//invalid priority
	if(Pri < 0 || Pri > 31)
		return ERC_INVALID_PRIORITY;

	MsgBin *msgbin = NULL;
	//__asm("lcall $0x78, $0x00" : "=a"(msgbin));
	//__asm("mov %%eax, %0" : :"a"(t));

	msgbin = CreateMsgBin();

	if(msgbin == NULL)
		return ERC_UNALLOCATED_MSGBIN;

	TaskData *td = CreateNewTD(0,0,0);

	if(td == NULL)
		return ERC_UNALLOCATED_TD;

	bin = msgbin;

	TaskState *ptr = pIdleTSS;

	ptr->msgbin = (void*)(msgbin);

	if(os)
		ptr->cs = OsCodeSel;
	else
		ptr->cs = UserCodeSel;

	ptr->eip =  (addr_t)&(*(addr_t*)(pEntry));
	ptr->esp0 = ptr->esp = (addr_t)&(*(addr_t*)(pStack));

	ptr->owner = td;

	//a new task needs a new page directory

	addr_t *pd = (addr_t*)MemAlloc(0);
	if(pd == NULL)
		return ERC_CANT_ALLOCPD;

	ptr->cr3 = *(addr_t*)pd;

	ptr->eflags = 0x0202;

	ptr->pri = Pri;

	//since this is the os idle task, no need to enqueue it or to switch to it here. we are done

	return 0;
}*/


//returns the id of newly created service
/*UINT NewService(addr_t *pEntry, addr_t* pStack, MsgBin *bin)
{
	UINT id = NextServiceID;
	NextServiceID++;

	bin = CreateMsgBin();
	if(bin == NULL)
		return 0;

	TaskData *td = CreateNewTD(0,0);

	if(td == NULL)
		return 0;

	TaskState *ptr = NULL;

	ptr = (TaskState*)malloc(sizeof(TaskState));

	if(ptr == NULL)
		return 0;

	ptr->num = id;

	ptr->msgbin = (void*)(bin);

	ptr->cs = OsCodeSel;

	ptr->eip =  (ULONG)pEntry;
	ptr->esp0 = ptr->esp = (ULONG)(pStack);

	ptr->owner = td;

	//a new task needs a new page directory

	addr_t *pd;
	if(MemAlloc(0, pd))
		return ERC_INVALID_PAGE;

	ptr->cr3 = *(addr_t*)pd;

	ptr->eflags = 0x0202;

	//set the priority to a value that will specify that this is a level 3 service not user app
	ptr->pri = 50;

	//add this new task into the service linked_list and queue it up

	if(ServiceList_Add(ptr))
		return 0;

	if(Service_Enqueue(ptr));
		return 0;

	return id;
}*/
