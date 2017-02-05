extern "C" {

#include "../include/mutex.h"


Mutex * CreateMutex(Mutex *mutex)
{
	mutex = (Mutex *)kmalloc(sizeof(Mutex));
	
	//mutex = new Mutex;
		
	
	if(mutex == NULL)
		return NULL;
		
	mutex->lock = 0;
	//mutex->queue = NULL;		
	
	return mutex;
}

void DestroyMutex(Mutex *mutex)
{
	DisableInterrupts();
	
	if(!mutex || mutex->lock)
	{
		EnableInterrupts();
		return;
	}	
		
	//delete mutex;
	
	kfree(mutex);
	
	EnableInterrupts();			
}

void EnterCriticalSection(Mutex *mutex)
{	
	if(!mutex)
		panic("Invalid Mutex Handle");
	
	DisableInterrupts();
	
	if(!mutex->lock)
	{
		//lock is not set...so set it and proceed into critical section.
		mutex->lock = 1;
		EnableInterrupts();
		return;
	}
	else
	{		
		//A lock is set, queue this proc
		/*if(!mutex->queue)
			mutex->queue = Enqueue(mutex->queue, (void*)currentProc->pid);
		else
			Enqueue(mutex->queue, (void*)currentProc->pid);*/
			
										
		while(1)
		{			
			DisableInterrupts();
			
			if(!mutex->lock)
			{
				mutex->lock = 1;
				break;
			}
			
			EnableInterrupts();
		}
	}	
	
	EnableInterrupts();
	
}

void LeaveCriticalSection(Mutex *mutex)
{
	DisableInterrupts();
	mutex->lock = 0;
	EnableInterrupts();
}

} //end extern "C"
