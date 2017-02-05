#ifndef __EXTERN_H
#define __EXTERN_H

//#include "../include/kernel.h"
//#include "../include/basicadt.h"
#include "../include/tss.h"

/*==================================================================
External Function Prototypes
==================================================================*/

extern unsigned long MemFind();
extern void enablePaging();

///extern void addTSStoGDT(addr_t addr);

//extern void AddTSSDesc(ULONG, ULONG, ULONG, ULONG);
extern void ltrOSTSS(ULONG sel);
extern void DoTaskSwitch(ULONG sel);



/*==================================================================
External Data Definitions
==================================================================*/

extern UINT NextServiceID;

extern addr_t page_dir[];
extern addr_t page_table[];
//extern void* page_stack_init[];
extern void* static_tss_cache[];

extern const ULONG etext[], bdata[], edata[], scode[], ebss[];
extern const ULONG tss_sel_loc;
extern ULONG max_procs;
extern ULONG TickCount;
extern ULONG nSwitches;
extern const char BootDrive;
extern const int GDTBase;

extern addr_t mmPagesAllocedInLastkmalloc[];

extern ULONG conventionalMemory;
extern ULONG extendedMem;

extern char intsEnabled;

#endif
