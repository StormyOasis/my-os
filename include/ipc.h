#ifndef __IPC_H_
#define __IPC_H_

#include "../include/types.h"

struct MessageBin_s;

typedef struct Message
{
	ULONG LN_Msg;
	void* LN_Data1;	
	void* LN_Data2;	
	
	void * proc;
	
} MSG;

typedef struct LinkNode_s
{		
	ULONG LN_Msg;
	void* LN_Data1;	
	void* LN_Data2;
		
	void * proc;
	
	ULONG reserved2;			
	
	struct MessageBin_s * msgbin;
	
	struct LinkNode_s *prev;
	struct LinkNode_s *next;
	
} LinkNode;

typedef struct MessageBin_s
{
	void *proc;	
	LinkNode *head;
	LinkNode *tail;
	
	ULONG reserved_for_future;
	
} MsgBin;

ULONG InitIPCSubSystem();
MsgBin * CreateMsgBin();

ULONG initMsgBin(MsgBin *, void *);

#ifdef __cplusplus
extern "C" {
	
ULONG AddMessage(MsgBin *, ULONG pid, UINT msg, void * proc, void *data1, void *data2);
ULONG GetMessage(MsgBin *, MSG &);
LinkNode * PeekMessage(MsgBin * bin);

}
#endif

#endif
