#include "../include/cpp.h"

extern "C" {
#include "../include/kern_mem.h"
}

/* NOTE: BE SURE TO CALL CONSTRUCTORS AND DESTRUCTORS */

/*void *operator new (size_t size)
{
	return (void*)kmalloc(size);
}

void *operator new[] (size_t size)
{
	return (void*)kmalloc(size);
}

void operator delete (void *p)
{
	kfree(p);
}

void operator delete[] (void *p)
{
	kfree(p);
}*/
