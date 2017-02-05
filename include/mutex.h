#ifndef _MUTEX_H_
#define _MUTEX_H_

#include "../include/defs.h"
#include "../include/util.h"

#ifdef __cplusplus
extern "C" {
	//#include "../include/sched.h"
	#include "../include/kern_mem.h"
}

#endif

//#include "../include/badtcpp.h"

typedef class Mutex_s 
{
	public:
		Mutex_s() {lock = 0; };
	
	char lock;	
	
	//Queue *queue;
	
} Mutex;


Mutex * CreateMutex(Mutex *);
void DestroyMutex(Mutex *);
void EnterCriticalSection(Mutex *);
void LeaveCriticalSection(Mutex *);
	

#endif
