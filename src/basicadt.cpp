#ifdef __cplusplus
extern "C" {
	#include "../include/kern_mem.h"
}
	#include "../include/badtcpp.h"
#else
	#include "../include/kern_mem.h"
	#include "../include/badtcpp.h"
#endif


Queue * Enqueue(Queue *q, void* data)
{
	if(!q)
	{		
		q = (Queue*)kmalloc(sizeof(Queue));

        if(!q)
        	panic("Enqueue error");

        q->head = (ListData*)kmalloc(sizeof(ListData));
        if(!q->head)
        	panic("Enqueue error");

        q->tail = q->head;

		q->head->next = NULL;
  		q->head->prev = NULL;
		q->head->data = data;
		
		//debug_printf("\ndata: %d", (addr_t)q->data);
		
		q->count = 1;
		
		return q;
	}

	ListData *new_node = (ListData*)kmalloc(sizeof(ListData));
	if(!new_node)
		panic("Enqueue error.");
		
	//Add new_node to the front of the list

	new_node->data = data;

	q->tail->next = new_node;
	new_node->prev = q->tail;
	new_node->next = NULL;

    q->tail = new_node;
	
	//debug_printf("\nnn: %d", (addr_t)new_node->data);
	
	q->count++;
	
	return q;		
}

Queue * EnqueueProc(Queue *q, void* data)
{
    //I don't remember why I separated these two funcs...
	return Enqueue(q, data);

	/*Queue * l = q;

	if(!q)
	{
		q = (Queue*)malloc(sizeof(Queue));

		q->next = NULL;
		q->prev = NULL;
		q->data = data;

		//debug_printf("\ndata: %d", (addr_t)q->data);

		q->items = 1;

		tail = q;

		return q;
	}

	//find the end	
	//while(l && l->next)
		//l = l->next;
		
	l = tail;
		
	Queue *new_node = (Queue*)malloc(sizeof(Queue));
	if(!new_node)
		panic("EnqueueProc memAlloc error.");
		
	//Add new_node to the front of the list
			
	new_node->data = data;			
	l->next = new_node;
	new_node->prev = l;
	new_node->next = NULL;	
	
	//debug_printf("\nnn: %d", (addr_t)new_node->data);
	
	q->items++;
	
	return q;	*/
}

void * Dq(Queue *q)
{		
	if(!q)	
	{
		//debug_printf("\nNull Queue");
		return NULL;	
	}						
	
	//q = q->next;
	//if(q->next)
	//	q->next->prev = NULL;		
	
	//Queue * qq = q->next;
	//qq->prev = NULL;						
	
	//q = qq;*/
	
   //	return q->data;
}


#if 0		
Queue * Enqueue(Queue *q, void* data)
{
	Queue * l = q;			
	
	if(!q)
	{		
		q = (Queue*)kmalloc(sizeof(Queue));							
		
		q->next = NULL;
		q->prev = NULL;
		q->data = data;
		
		//debug_printf("\ndata: %d", (addr_t)q->data);
		
		q->items = 1;
		
		return q;
	}
	
	//find the end	
	while(l && l->next)
		l = l->next;
		
	Queue *new_node = (Queue*)kmalloc(sizeof(Queue));
	if(!new_node)
		panic("Enqueue error.");
		
	//Add new_node to the front of the list
			
	new_node->data = data;			
	l->next = new_node;
	new_node->prev = l;
	new_node->next = NULL;	
	
	//debug_printf("\nnn: %d", (addr_t)new_node->data);
	
	q->items++;
	
	return q;		
}

Queue * EnqueueProc(Queue *q, Queue * tail, void* data)
{
	Queue * l = q;			
	
	if(!q)
	{		
		q = (Queue*)kmalloc(sizeof(Queue));							
		
		q->next = NULL;
		q->prev = NULL;
		q->data = data;
		
		//debug_printf("\ndata: %d", (addr_t)q->data);
		
		q->items = 1;
		
		tail = q;
		
		return q;
	}
	
	//find the end	
	//while(l && l->next)
		//l = l->next;
		
	l = tail;
		
	Queue *new_node = (Queue*)kmalloc(sizeof(Queue));
	if(!new_node)
		panic("Enqueue error.");
		
	//Add new_node to the front of the list
			
	new_node->data = data;			
	l->next = new_node;
	new_node->prev = l;
	new_node->next = NULL;	
	
	//debug_printf("\nnn: %d", (addr_t)new_node->data);
	
	q->items++;
	
	return q;		
}

void * Dq(Queue *q)
{		
	if(!q)	
	{
		//debug_printf("\nNull Queue");
		return NULL;	
	}						
	
	//q = q->next;
	//if(q->next)
	//	q->next->prev = NULL;		
	
	//Queue * qq = q->next;
	//qq->prev = NULL;						
	
	//q = qq;*/
	
	return q->data;
}


#endif
