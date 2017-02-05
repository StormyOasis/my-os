//These basic ADT's are used only in the Kernel

#include "../include/basicadt.h"
#include "../include/kern_mem.h"
#include "../include/extern.h"

void CreateList(List *list)
{
	list = (List*)kmalloc(sizeof(List));
	if(!list)
		return;

	list->head = NULL;
	list->tail = NULL;

	list->count = 0;
}

void DestroyList(List *list)
{
	//mutex ????

	ListData *c = list->head;
	ListData *n = list->head->next;

	while(c)
	{
		kfree(c);
		c = n;
        	if(n)
		  n = n->next;
	}

    	kfree(list);
}

void add_item(List *list, void *data)
{
	ListData *newnode = (ListData*)kmalloc(sizeof(ListData));
	newnode->next = NULL;
	newnode->prev = list->tail;
	newnode->data = data;
    list->tail->next = newnode;
	list->count++;
}

void RemoveItem(List *list, void *data)
{
	ListData *c = list->head;
    	ListData *n1;

	if(!list || !list->head)
    		return;

	while(c)
	{
		if(c->data == data)
		{
			n1 = c;

			if(c->prev)
				c->prev->next = c->next;
			if(n1->next)
				c->next->prev = n1->prev;

			kfree(n1);

			list->count--;
		}
		c = c->next;
	}

          /*

	List * l = list;
	List *nl;

	while(l)
	{
		if(l->data == data)
		{
			nl = l;
			if(l->prev)
				l->prev->next = l->next;
			if(nl->next)
				l->next->prev = nl->prev;

			//kfree(nl); //correct ????

			list->items--;

			break;
		}
	}        */
}

void CreateStack(Stack *stack)
{
	CreateList(stack);
}

void DestroyStack(Stack *stack)
{
	DestroyList(stack);
}

void push(Stack *stack, void *data)
{
	//AddItem puts it at the end of the list anyways, so use it
	add_item(stack, data);
}

void * pop(Stack *stack)
{
	ListData *pt;
    ListData *poped;
    void * data;

    if(!stack)
    	return NULL;

   	if(!stack->head || !stack->tail)
   		return NULL;

   	pt = stack->tail->prev;
   	poped = stack->tail;
   	poped->next = poped->prev = NULL;
   	pt->next = NULL;
   	stack->tail = pt;

   	data = poped->data;
   	kfree(poped);

  	return data;
}




//In order to avoid reentrancy problems, this func should either only be called
//when initializing(in single task mode), or we should implement some sort of
//mutex, semaphore, spinlock, etc...
void PushMem(Stack *list, addr_t addr)
{
	//almost the same as above.

	//Make sure that addr is a virtual address
	//Since this func is only used for pushing page address on the page stack,
	//the address should be greater than or == to 0xD0000000.  If it is less,
	//then we can just add  0xD0000000 to the address.

	if(addr < 0xD0000000)
		addr = (0xD0000000 + (addr_t)addr);

	ListData *newnode = (ListData*)kmalloc(sizeof(ListData));

	//debug_printf("\nPushMem: addr: %d newnode: %d PA newnode: %d", (addr_t)addr, (addr_t)newnode, PAGE_ALIGN(newnode));

	UINT i = 0;
	//for(i = 0; i < 6; i++)
	//	debug_printf("\nmmPagesAllocedInLastkmalloc[%d]: %d", i, mmPagesAllocedInLastkmalloc[i]);

	//be sure that none of the pages used in the kmalloc is the
	//one that we are trying to push.
	if(PAGE_ALIGN(newnode) == addr ||
		(PAGE_ALIGN(newnode) == mmPagesAllocedInLastkmalloc[0]) ||
		(PAGE_ALIGN(newnode) == mmPagesAllocedInLastkmalloc[1]) ||
		(PAGE_ALIGN(newnode) == mmPagesAllocedInLastkmalloc[2]) ||
		(PAGE_ALIGN(newnode) == mmPagesAllocedInLastkmalloc[3]) ||
		(PAGE_ALIGN(newnode) == mmPagesAllocedInLastkmalloc[4]) ||
		(PAGE_ALIGN(newnode) == mmPagesAllocedInLastkmalloc[5])  )
	{
		//problem!!! The page address we are pushing, is the same page
		//that was just allocated for newnode or was used in the allocation process(eg. bucket information).

		//for now, just halt.
		debug_printf("\nPushMem: addr: %d newnode: %d PA newnode: %d", (addr_t)addr, (addr_t)newnode, PAGE_ALIGN(newnode));

		for(i = 0; i < 6; i++)
			debug_printf("\nmmPagesAllocedInLastkmalloc[%d]: %d", i, mmPagesAllocedInLastkmalloc[i]);

		panic("Page stack error.");
	}

	if(!list->head)
	{
		//this is the first entry on the stack.
		newnode->prev = newnode->next = NULL;
		newnode->data = (void*)addr;
		list->head = list->tail = newnode;
	}
	else
	{
		newnode->next = NULL;
		newnode->prev = list->tail;
		newnode->data = (void*)addr;

		list->tail->next = newnode;
		list->tail = newnode;
	}

	/*List *l = list;

	while(l->next)
		l = l->next;

	// l is a pointer to the last element in the list

	newnode->next = NULL;
	newnode->prev = l;
	newnode->data = (void*)addr;

	l->next = newnode;
	*/
}

#if 0
void CreateList(List *list)
{
	list = (List*)kmalloc(sizeof(List));
	if(!list)
		return;

	list->data = NULL;
	list->next = NULL;
	list->prev = NULL;

	list->items = 0;
}

void DestroyList(List *list)
{
	//mutex ????

	List *c = list;
	List *n = list->next;

	while(c)
	{
		kfree(c);
		c = n;
		n = n->next;
	}
}

void AddItem(List *list, void *data)
{
	List *newnode = (List*)kmalloc(sizeof(List));

	List *l = list;

	while(l->next)
		l = l->next;

	// l is a pointer to the last element in the list

	newnode->next = NULL;
	newnode->prev = l;
	newnode->data = data;

	l->next = newnode;

	list->items++;
}

void RemoveItem(List *list, void *data)
{
	List * l = list;
	List *nl;

	while(l)
	{
		if(l->data == data)
		{
			nl = l;
			if(l->prev)
				l->prev->next = l->next;
			if(nl->next)
				l->next->prev = nl->prev;

			//kfree(nl); //correct ????

			list->items--;

			break;
		}
	}
}

void CreateStack(Stack *stack)
{
	CreateList(stack);
}

void DestroyStack(Stack *stack)
{
	DestroyList(stack);
}

void Push(Stack *stack, void *data)
{
	//AddItem puts it at the end of the list anyways, so use it
	AddItem(stack, data);
}

void * Pop(Stack *stack)
{
	List *list = stack;
	Stack *item = NULL;

	void * data = NULL;

	//List *l2;

	while(list)
	{
		//this should be the last element, if not, there is a prob...duh...

		//debug
		/*l2 = stack;

		while(l2)
		{
			if((l2 != list) && ((addr_t)list->data == (addr_t)l2->data))
				panic("Duplicate found!");
			l2 = l2->next;
		}
		*/
		//end debug

		if(list->next == NULL)
		{
			item = list;

			data = item->data;

			if(list->prev)
				list->prev->next = NULL;

			break;
		}

		list = list->next;
	}

	if(data)
	{
		list->items--;
		kfree(item);
	}

	return data;
}

#endif


void CreateQueue(Queue *q)
{
	CreateList(q);
}

void DestroyQueue(Queue *q)
{
	DestroyList(q);
}
