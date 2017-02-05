#include "../include/ipc.h"

#ifdef __cplusplus
extern "C" {
	#include "../include/proc.h"
	#include "../include/kern_mem.h"
	#include "../include/util.h"
}
#endif

ULONG AddMessage(MsgBin *bin, ULONG pid, UINT msg, void * proc, void *data1, void *data2)
{
	LinkNode *newnode = (LinkNode*)kmalloc(sizeof(LinkNode));
	
	if(!newnode)
		return 1;
		
	newnode->LN_Msg = msg;
	newnode->LN_Data1 = data1;
	newnode->LN_Data2 = data2;
	newnode->msgbin = bin;
	newnode->proc = proc;
	
	//debug_printf("\nadd proc: %d %d", (addr_t)newnode->proc, (addr_t)proc);
	
	newnode->next = newnode->prev = NULL;
	
	//put it on the back of the list
	
	if(!bin->head)	
	{
		bin->head = bin->tail = newnode;
		bin->proc = getProccessWithPID(pid);
	}
	else
	{
		bin->tail->next = newnode;
		newnode->prev = bin->tail;
		bin->tail = newnode;
	}	
	
	return 0;
}

ULONG GetMessage(MsgBin * bin, MSG & msg)
{		
	if(!bin || !bin->head)
		return 1;
			
	//remove the head from the list.
	
	msg.LN_Msg = bin->head->LN_Msg;
	msg.LN_Data1 = bin->head->LN_Data1;
	msg.LN_Data2 = bin->head->LN_Data2;
	msg.proc = bin->head->proc;
	
	if(bin->head == bin->tail)
	{
		kfree(bin->head);
		
		//this is the last message
		bin->head = NULL;
		bin->tail = NULL;
	}
	else
	{				
		bin->head->next->prev = NULL;
		
		kfree(bin->head);
		
		bin->head = bin->head->next;
	}
	
	if(msg.proc)
		return 1;
	
	return 0;
}

LinkNode * PeekMessage(MsgBin * bin)
{
	if(!bin || !bin->head)
		return NULL;
	
	/*msg.LN_Msg = bin->head->LN_Msg;
	msg.LN_Data1 = bin->head->LN_Data1;
	msg.LN_Data2 = bin->head->LN_Data2;
	msg.proc = bin->head->proc;	*/

	return bin->head;
}
