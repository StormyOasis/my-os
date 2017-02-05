#include "../include/extern.h"
#include "../include/ipc.h"
#include "../include/kernel.h"
#include "../include/proc.h"
#include "../include/kern_mem.h"

#define ERC_ADDMSG_ALLOCFAIL 3000

ULONG initMsgBin(MsgBin *msgbin, void *proc)
{
	msgbin->proc = proc;
	msgbin->head = NULL;
	msgbin->tail = NULL;
	
	return 0;
}

//Add a message onto the message queue

/*====================================================================
CreateMsgBin() - FAR

IN: none
OUT: returns pointer to the new MsgBin

because of the way that this func is written..alot of work will need to be done
in the deallocation of msgbins...That is where garbage collection will occur as well
as the maintaining of the linked list and deallocation of unused pages.
====================================================================*/

MsgBin* CreateMsgBin()
{
	//lock mutex	
	
	/*MsgBin * newbin = NULL;
	int res;			
	
	if((nMsgBins+1) > MAX_MSG_BINS)
	{
		//__asm("mov $0, %eax");
		//__asm("leave");
		//__asm("lret");		
		return 0;
	}				
	
	//if the following cmp doesnt work, then try...if PAGE_ALIGN(LastAddr) == PAGE_ALIGN_UP(LastAddr)
	if((nMsgBins+1) % nMBPagesAlloc)
	{
		//allocate new page and stick the msgbin on it
		newbin = kmalloc(sizeof(MsgBin));
		
		if(newbin == NULL)
		{			
			//__asm("mov $0, %eax");
			//__asm("leave");
			//__asm("lret");			
			return 0;
		}
			
		//create bin					
				
		newbin->MB_Owner = (void*)pCurrentTSS;	//set the addr of the task data struct
		newbin->MB_Head = NULL;				//addr of first lb in the queue(none yet)
		newbin->MB_Tail = NULL;				//addr of the last lb in queue(none yet)
		newbin->MB_Next = NULL;				//set the addr of the next msgbin		
										//(this is the last one) adjust the previous 
										//msgbin's next pointer
										
		LastAddr->MB_Next = newbin;	//set the addr of the next msgbin(this is the last one)
		LastAddr = newbin;
		
		
		nMsgBins++;
		nMBPagesAlloc++;		
	}
	else
	{
		//create bin					
		
		//adjust the previous msgbin's next pointer
		//set the addr of the next msgbin(this is the last one)
		LastAddr->MB_Next = LastAddr + sizeof(MsgBin);
		
		LastAddr += sizeof(MsgBin);		
		newbin = LastAddr;

				
		newbin->MB_Owner = (void*)pCurrentTSS;	//set the addr of the task data struct
		newbin->MB_Head = NULL;				//addr of first lb in the queue(none yet)
		newbin->MB_Tail = NULL;				//addr of the last lb in queue(none yet)
		newbin->MB_Next = NULL;				//set the addr of the next msgbin(this is the last one)						
		
		nMsgBins++;		
	}
	
	//unlock mutex
	
	//__asm("mov %0, %%eax" : : "g"(page));
		
	
	//do the actual far return
	//__asm("leave");	
	//__asm("lret");	
	
	return newbin;*/
}

/*====================================================================
DeallocMsgBin - FAR

IN: ptr to MsgBin to be deleted
OUT: none

====================================================================*/

void DeallocMsgBin(MsgBin * mb)
{
	//mb is the address of the msgbin to dealloc			
		
	/*MsgBin * next = OsMsgBin->MB_Next;
	MsgBin * prev = NULL;
	MsgBin *x = NULL;
	UINT i;
	int clean = 1;
	
	while(next != NULL)
	{				
		if(next == mb)
			break;
			
		prev = next;
		next = next->MB_Next;
	}
	
	//prev now points to previous msgbin in list	
	
	//remove it from the list
	
	prev->MB_Next = mb->MB_Next;	
	
	//mb is now out of the list
	//null out the next pointer...explaination follows
	mb->MB_Next = NULL;	
				
	//garbage collection time...free the page if there are no other allocated msgbins on this page
	for(i = PAGE_ALIGN(mb); i < PAGE_ALIGN_UP(mb); i+=sizeof(MsgBin))
	{
		x = (MsgBin*)(addr_t*)i;
		if(x->MB_Next != NULL)
		{
			clean = 0;
			break;
		}
	}
		
	//dealloc the page
	if(clean)
	{
		kfree(mb);		
		nMBPagesAlloc--;
	}
	
	//assign NULL to each next pointer when it is deallocated.  When the whole page is full of NULL
	//next pointers, then it is time to dealloc that page
			
	//watch out for lastaddr	
	//only worry about the last one if it was the one just deallocated 	
	//possible bug below....
	if(LastAddr == mb)
		LastAddr = prev;
		
	nMsgBins--;*/
}
