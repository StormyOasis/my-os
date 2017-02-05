#ifndef _BASICADTS_H
#define _BASICADTS_H

#include "../include/types.h"

typedef struct s_LIST List;
typedef struct s_LISTDATA ListData;

struct s_LISTDATA
{
	void * data;
	ListData *next;
	ListData *prev;
};

struct s_LIST
{
    ListData *head;
   	ListData *tail;
	ULONG count; /* Num of items in list? */
};

typedef List Stack;
typedef List Queue;

void CreateList(List *list);
void add_item(List* list, void *data);
void RemoveItem(List *l, void *data);
void DestroyList(List *list);

void CreateStack(List *list);

void push(Stack *stack, void *data);

void * pop(Stack *stack);
void DestroyStack(Stack*);

void CreateQueue(Queue *q);
void DestroyQueue(Queue *q);


#endif
