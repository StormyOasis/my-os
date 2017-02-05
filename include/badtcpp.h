#ifndef __BADTCPP_H
#define __BADTCPP_H

#ifdef __cplusplus
extern "C" {
	#include "../include/util.h"
	#include "../include/basicadt.h"
	#include "../include/extern.h"
}
#else
	#include "../include/util.h"
	#include "../include/extern.h"
#endif

Queue * Enqueue(Queue *q, void* data);
Queue * EnqueueProc(Queue *q,  void* data);

void * Dq(Queue * q);

#define DequeueProc(q) 			\
{								\
	if(!q)						\
		dqresult = NULL;			\
	else							\
	{							\
		dqresult = q->head->data;			\
		ListData *qq = NULL;			\
		if(q->head->next)				\
		{						\
			qq = q->head->next;			\
			qq->prev = NULL;		\
		}						\
		else						\
			qq = NULL;			\
		kfree(q->head);					\
		q->head = qq;					\
		if(!q->head)					\
		{					\
			q->tail = NULL;				\
			q = NULL;			\
		}					\
		else						\
			q->head->prev = NULL;		\
	}							\
}






#endif
