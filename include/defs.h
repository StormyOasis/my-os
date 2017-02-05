#ifndef __DEFS_H_
#define __DEFS_H_

#include "../include/types.h"
#include "../include/cgstubs.h"

#define OsCodeSel 0x08
#define OsDataSel 0x10
#define UserCodeSel 0x18

//#define GDT_BASE 0x8830

#define OsStackTop 0x00000000

//#define SIZEOF_TSS		512 //this has changed...no hardcode

#define NUM_TSS 	101 //total number of tss(static + dynamic)
#define MAX_TSS 	100 //maximum dynamically allocatable tss's 

#define SIZEOF_EACH_RDYQ 8

#define COMPACTION_FRAG 30

#define STANDARD_STACK_SIZE 1024

#define USER_TASK 1
#define OS_TASK 0

#define LB_TYPE_MSG 0x00000001

#define SUCCESS 0
#define ERC_INVALIDTSS 5000
#define ERC_UNALLOCATED_MSGBIN 5001
#define ERC_INVALID_PRIORITY 5002
#define ERC_INVALID_MSGBIN 5003
#define ERC_CANT_ALLOCPD 5004
#define ERC_ALLOCMEM_ERR 5005
#define ERC_UNALLOCATED_TD 5006
#define ERC_BAD_TSS 5007
#define ERC_ALLOCMEM_BADSIZE 5008
#define ERC_PROC_NUM_EXCEEDED 5009
#define ERC_ADDR_SPACE_ALLOC_FAIL 5010
#define ERC_INVALID_PAGE 5011
#define ERC_MUTEX_ALLOC_ERROR 5012

#define debug(x) { debug_printf(x);}

#define true 1
#define false 0

#endif
