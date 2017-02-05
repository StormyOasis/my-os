#ifndef _CPP_SUPPORT_H
#define _CPP_SUPPORT_H

extern "C" {
#include "../include/util.h"
#include "../include/types.h"
}

/*void * operator new (size_t size);
void * operator new[] (size_t size);

void operator delete(void *p);
void operator delete[] (void *);

inline void* operator new(size_t, void* p)   { return p; }
inline void* operator new[](size_t, void* p) { return p; }

inline void  operator delete  (void*, void*) { };
inline void  operator delete[](void*, void*) { };
*/
#endif
