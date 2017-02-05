#include "../include/defs.h"
#include "../include/kern_mem.h"
#include "../include/extern.h"
#include "../include/proc.h"
#include "../include/String.h"

/*
#ifdef __cplusplus
extern "C" {
	#include "..\include\mutex.h"
}
#else
	#include "..\include\mutex.h"
#endif*/

const addr_t KERNEL_START = (addr_t)scode;
const addr_t KERNEL_TEXT_END = (addr_t)etext;
const addr_t KERNEL_END = (addr_t)ebss;
const addr_t KERNEL_SIZE = 1024*1024; /* We are just going to assume that everything below 1 mb is the kernel */

UINT num_pages_on_system;
UINT num_pages_on_system_above_1_meg;
ULONG num_physical_page_allocs = 0;
ULONG num_virtual_page_allocs = 0;
ULONG max_procs = 0;
ULONG total_system_memory_bytes;
ULONG current_map_offset_low;
ULONG current_map_offset_high;
ULONG num_page_allocs = 0;
addr_t mmPagesAllocedInLastkmalloc[6];

char * physical_page_map = NULL;

ADDR_SPACE kernel_addr_space;


ULONG image_end_alloc_offset = 0;	//Offset used to find next addr in allocate_from_image_end

/****************************************************************************
 * Function: allocate_from_image_end
 * INPUT: amount of bytes to allocate
 * OUTPUT: pointer to allocated block
 * Allocates specified amount of bytes from the end of the kernel image.
 * This should prevent us from using the same address more than once.
 * NOTE: This function should only be used by the initialization process before
 * the kernel memory manager has been fully initialized.  This memory cannot
 * currently be freed.
 ****************************************************************************/
void * allocate_from_image_end(size_t size) {
	if((KERNEL_END + image_end_alloc_offset + size) >= total_system_memory_bytes) {
		panic("Insufficient memory available for allocation.");
	}

	char * buf = (char *)(KERNEL_END + image_end_alloc_offset);
	image_end_alloc_offset += size;
	return buf;
}


/****************************************************************************
 * Function: init_low_level_mem
 * INPUT: NONE
 * OUTPUT: 0 on success.
 * Initializes the kernel memory manager.  Upon completion, the kernel
 * memory allocation functions are available(eg. kmalloc).
 * NOTE: For now, we are preventing the use of any conventional memory.
*****************************************************************************/
int init_low_level_mem()
{
	ULONG i,j;

	if(conventionalMemory == 0 && extendedMem == 0)
		panic("No memory found in system!");
	else if(extendedMem == 0)
		panic("Currently cannot run in conventional memory.");

	/*==========================================================================
	*	Calculate the amount of memory on the system so that we can use that to
	*	determine the maximum number of procs that can run at one time.
	*===========================================================================*/
	total_system_memory_bytes = conventionalMemory * 1024 + extendedMem;
	num_pages_on_system = (total_system_memory_bytes) / (PAGE_SIZE);
	num_pages_on_system_above_1_meg = num_pages_on_system - 256;

	debug_printf("\n\t|--Amount of memory on system: %d KB, %d pages",
			(total_system_memory_bytes/1024), num_pages_on_system);
	debug_printf("\n\t|--Conventional %d KB, Extended: %d MB",
			conventionalMemory, extendedMem/1024/1024);

	//TODO:  Is there any reason to limit the number of processes?
	if(num_pages_on_system <= 1024)
		max_procs = num_pages_on_system;
	else if(num_pages_on_system > 1024 && num_pages_on_system <= 4096)
		max_procs = 16384;
	else if(num_pages_on_system > 4096 && num_pages_on_system <= 8192)
		max_procs = 65536;
	else
		max_procs = 131072;

	debug_printf("\n\t|--Maximum number of processes set at %d", max_procs);

	/*==========================================================================
	 * Initialize the physical page allocation map	by initializing all entries
	 * greater than the amount of memory on system to 1(marking them as used).
	 *========================================================================*/
	debug_printf("\n\t|--Initializing physical page map...");

	/*	We need to allocate enough pages for the map first. These pages
	 *  will be allocated at the end of the kernel image.
	 */
	ULONG ppm_size_bytes = num_pages_on_system_above_1_meg / 8; //1 byte per 8 pages
	physical_page_map = allocate_from_image_end(ppm_size_bytes);

	//Mark bitmap entries as unused
	for(i = 0; i < num_pages_on_system_above_1_meg; i++) {
		physical_page_map[i] = 0;
	}

	debug_printf("Done.");

	/*==============================================================================
	 * Now create and initialize the page directories and page tables for the kernel
	 * enabling paged mode.  This is a very naive setup right now...lazy...
	 *============================================================================*/
	debug_printf("\n\t|--Creating kernel address space...");

	j = 0;
	/* set the pdes and ptes to 0 so that the present bit is 0 */
	for(i = 0; i < 1024; i++)
		(page_dir[i]) = (page_table[i]) = (addr_t)0;

	/* fill in the actual pt */
	for(i = 0; i < KERNEL_SIZE; i += PAGE_SIZE, j++)
		(page_table[j]) = (addr_t)i | PRIV_WR | PRIV_KERN | PRIV_PRES;

	/* fill in the first page table into the first pde */
	page_dir[0] = (addr_t)page_table | PRIV_WR | PRIV_PRES | PRIV_KERN;

	/* page_dir and page_table are allocated in the ldscript */
	kernel_addr_space.page_dir = (addr_t*)page_dir;
	kernel_addr_space.os_page_table = (addr_t*)page_table;

	debug_printf("Done.");

	/*Let's try to flip the switch!*/
	debug_printf("\n\t|--Entering paged mode...");
	enablePaging();

	debug_printf("Done.");

	//TODO:  Maybe BUCKTES and PAGE_POOLS should be created here rather than as stack entries or hardcoded
	//into ldscript.

	//Let's test kmalloc here
	char * ptr = (char *)kmalloc(2);
	ptr[0] = 'A';
	ptr[1] = '\0';
	debug_printf("\nAddress: %x - value: %s", ptr, ptr);
	halt();
	//end test

	return 0;
}

/*********************************************************************************
 * Function: kmalloc
 * INPUT:	size to be allocated
 * OUTPUT:	returns address of allocated block
 *
 * Allocates memory(linear) from the kernel address space to be used by the os.
 * This will be mapped into every user address space.
*********************************************************************************/
void * kmalloc(size_t size) {
	if(size > MAX_ADDRESSABLE_BYTES) {
		return NULL;
	}

	return allocate(size);
}

/*********************************************************************************
 * Function: kfree
 * INPUT:	pointer to block of memory to be freed
 * OUTPUT:	NONE
 *
 * Frees memory previously allocated using kmalloc()
*********************************************************************************/
void kfree(void *buf) {
	if(buf == NULL) {
		return;
	}
}

/*********************************************************************************
 * Function: allocate
 * INPUT:	size to be allocated
 * OUTPUT:	returns address of allocated block
 *
 * Allocates memory(linear) from the kernel address space to be used by the os.
 * This will be mapped into every user address space.
*********************************************************************************/
void * allocate(size_t size) {
	void * buffer = NULL;






	return buffer;
}

#if 0
/*==========================================================================
Function: page_alloc
INPUT:	NONE
OUTPUT: returns address of allocated page

Allocates a physical page and marks it as used. Always tries to allocate
from the end of the physical memory
*=========================================================================*/
void * page_alloc()
{
	int i, pass;
	addr_t * page = NULL;
	ULONG map_offset, map_bit;
	char found = 0;
	ULONG page_number;

	map_offset = current_map_offset_high;

	pass = 1;
	while(!found)
	{
		for(i = 7; i >= 0; i--) /* Check each bit */
		{
			if(GetBitState(&physical_page_map[map_offset], i) == 0)
			{
				found = 1;
				break;
			}
		}

		if(found)
		{
			map_bit = i;
			break;
		}

		/* Test to see if we have gone all the way around */
		if(map_offset == 0)
		{
			if(pass == 1) /*If we have only gone around once, then we should try again from the very end */
			{
				map_offset = last_physical_page;
				pass++;
			}
			else /* No free space left.  We've tried twice, so give up. */
				break;
		}

		map_offset--;
	}

	/* 	A free page has been found.  We now need to convert the map_offset/map_bit
		pairing into the address of a physical page.  We should also mark the page
		as used.
	*/
	if(found)
	{
		SetBit(&physical_page_map[map_offset], map_bit);

		/*	Each offset represents a series of 8 pages.  The bit
			represents which of those 8 pages was returned.
		*/
		page_number = map_offset * 8 + map_bit;
		page = (addr_t*)((page_number + 256)*PAGE_SIZE); /* Don't forget that we are only using pages above 1 MB */

		current_map_offset_high = map_offset;

		num_physical_page_allocs++;
	}

	return page;
}

/*==========================================================================
Function: page_alloc_dma
INPUT:	count - number of contiguous pages to allocate
OUTPUT: returns address of first allocated page

Allocates 1 or more physical pages in contigous run and marks each as used.
Always tries to allocate memory below the physical 16MB mark.
No info will be stored about the run, so it is up to the application programmer
to free each page in the run.
*=========================================================================*/
void * page_alloc_dma(ULONG count)
{
	addr_t * page = NULL;
	ULONG map_offset, map_bit;
	ULONG i,j, running_count = 0;
	ULONG max_offset;
	ULONG num_bits = 0;
	ULONG page_number;
	char running = 0;
	char found = 0;

	if(count == 0 || count > NUM_LOW_PAGES)
		return NULL;

	/*  Find the last offset that is possible based on the count value */
	max_offset = (NUM_LOW_PAGES / 8) - (count / 8);

	/* For now, we are just going to use a horribly slow sequential search */
	for(i = 0; i < max_offset; i++)
	{
		for(j = 0; j < 8; j++)
		{
			if(GetBitState(&physical_page_map[i], j) == 0)
			{
				if(running == 0)
				{
					map_offset = i;
					map_bit = j;
					num_bits = 0;
				}

				running = 1;
				running_count++;
				num_bits++;
				if(running_count == count)
				{
					found = 1;
					goto end;
				}
			}
			else
			{
				running = 0;
				running_count = 0;
				num_bits = 0;
			}
		}
	}

	end:

	/* 	A free page has been found.  We now need to convert the map_offset/map_bit
		pairing into the address of a physical page.  We should also mark the page
		as used.
	*/
	if(found)
	{
		for(i = 0; i < num_bits; i++)
			SetBit(&physical_page_map[map_offset], map_bit + i);

		/*	Each offset represents a series of 8 pages.  The bit
			represents which of those 8 pages was returned.
		*/
		page_number = map_offset * 8 + map_bit;
		page = (addr_t*)((page_number + 256)*PAGE_SIZE); /* Don't forget that we are only using pages above 1 MB */

		num_physical_page_allocs++;
	}

	return page;
}

/*==========================================================================
Function: page_alloc_run
INPUT:	count - number of contiguous pages to allocate
OUTPUT: returns address of first allocated page

Allocates 1 or more physical pages in contigous run and marks each as used.
No info will be stored about the run, so it is up to the application programmer
to free each page in the run.  Will only check above 16MB and works
bottom-up.
*=========================================================================*/
void * page_alloc_run(ULONG count)
{
	addr_t * page = NULL;
	ULONG map_offset, map_bit;
	ULONG i,j, running_count = 0;
	ULONG max_offset;
	ULONG num_bits = 0;
	ULONG page_number;

	char running = 0;
	char found = 0;

	if(count == 0 || count > num_pages_on_system_above_1_meg)
		return NULL;

	/*  Find the last offset that is possible based on the count value */
	max_offset = num_pages_on_system_above_1_meg - (count / 8);

	/* For now, we are just going to use a horribly slow sequential search */
	for(i = (NUM_LOW_PAGES / 8); i < max_offset; i++)
	{
		for(j = 0; j < 8; j++)
		{
			if(GetBitState(&physical_page_map[i], j) == 0)
			{
				if(running == 0)
				{
					map_offset = i;
					map_bit = j;
					num_bits = 0;
				}

				running = 1;
				running_count++;
				num_bits++;
				if(running_count == count)
				{
					found = 1;
					goto end;
				}
			}
			else
			{
				running = 0;
				running_count = 0;
				num_bits = 0;
			}
		}
	}

	end:

	/* 	A free page has been found.  We now need to convert the map_offset/map_bit
		pairing into the address of a physical page.  We should also mark the page
		as used.
	*/
	if(found)
	{
		for(i = 0; i < num_bits; i++)
			SetBit(&physical_page_map[map_offset], map_bit + i);

		/*	Each offset represents a series of 8 pages.  The bit
			represents which of those 8 pages was returned.
		*/
		page_number = map_offset * 8 + map_bit;
		page = (addr_t*)((page_number + 256)*PAGE_SIZE); /* Don't forget that we are only using pages above 1 MB */

		num_physical_page_allocs++;
	}

	return page;
}

/*==========================================================================
Function: page_free
INPUT:	address of page to be returned
OUTPUT: returns address of allocated page

Frees a previously allocated page by clearing the bit in the physical_page_map
*=========================================================================*/
void page_free(addr_t * page)
{
	ULONG map_offset, map_bit;
	ULONG page_number;

	page_number = ((addr_t)page) / PAGE_SIZE;
	page_number -= 256; /* All mem is above 1 MB Only!*/

	map_bit = page_number % 8;
	map_offset = page_number / 8;

	ClearBit(&physical_page_map[map_offset], map_bit);
}


/*==========================================================================
Function: kmalloc
INPUT:	size to be allocated
OUTPUT: returns address of allocated block

Allocates memory(linear) from the kernel address space to be used by the os.
This will be mapped into every user address space.
*=========================================================================*/
void * kmalloc(size_t size)
{

}
#endif











#if 0
PAGE_POOL_INIT init;
PAGE_POOL_INIT low;
PAGE_POOL high;
Stack *low_stack = NULL;
Stack *high_stack = NULL;

BUCKET_DESC * desc = NULL;
BUCKET_FREE_BMP *bmp = NULL;

BUCKET_CHAIN buckets[] = {		//Bytes needed to track per page control info
	{ 8, 0,NULL,NULL, NULL, NULL }, //64
	{ 16, 0,NULL,NULL, NULL, NULL },//32
	{ 32, 0,NULL,NULL, NULL, NULL},//16
	{ 64, 0,NULL,NULL, NULL, NULL },//8
	{ 128, 0,NULL,NULL, NULL, NULL},//4
	{ 256, 0,NULL,NULL, NULL, NULL },//2
	{ 512, 0,NULL,NULL, NULL, NULL },//1
	{ 1024, 0,NULL,NULL, NULL, NULL },//1 (actually 4 bits)
	{ 4096, 0,NULL,NULL, NULL, NULL }};//1 (actually 1 bit)

	/*This makes the largest single allocatable amnt as 4096 bytes*/

	/*==========================================================================
		Make all physical memory(above 1 MB) 1:1 mapped at virtual address PHYSICAL_MAP_ADDRESS.
		Currently, Since mapping physical at PHYSICAL_MAP_ADDRESS if there is more than 1 gig of ram in the system,
		only 1 gig will be addressable.  Fix in the future by mapping page tables into page
		directories or whatever (Not 1:1).
	==========================================================================*/
	//ULONG res = create_mapping(&kernel_addr_space, (ULONG)PHYSICAL_MAP_ADDRESS, 1024*1024/* 1 MB*/,
	//				total_system_memory_bytes, PRIV_WR | PRIV_PRES | PRIV_KERN);

//	if(res)
	//	return (int) res;

/*==========================================================================
Function: page_alloc
INPUT:	stack designation(0 - low, 1 - high)
		location to place error info

OUTPUT: returns address of allocated page

Allocates a page from either the init/low arrays, or the high stack
*=========================================================================*/
void * page_alloc(int dma, unsigned long * ecode)
{
	/* LockMutex(&mutex); */

	char found = 0;
	int i = 0;
	void *s = NULL;
	addr_t addr;
	int off = -1;

	*ecode = 0;

	if(mem_ready == 0)
	{

		/*======================================================================
			Just do a sequential search through the init stack.
		======================================================================*/

		for(i = 0; i < init.num_pages; i++)
		{
			if(init.stack[i] == 0)
			{
				off = i;
				break;
			}
		}

		/* Nothing found */
		if(off == -1)
		{
			*ecode = ERC_ALLOCMEM_ERR;
			return NULL;
		}

		/* Convert the offset into an address */
		addr = off * PAGE_SIZE;
		addr += (PHYSICAL_MAP_ADDRESS);
		s = (void*)addr;

		num_page_allocs++;
		init.free_pages--;

		/*======================================================================
			If paging is not enabled then memory is an actual physical address
			but the init page stack contains virtual address, so we need to
			subtract the address where the memory is mapped. Remember,
			we are only using memory above 1 MB, so adjust accordingly.
		======================================================================*/
		if(!paging_enabled)
			return (void*)((void*)s - PHYSICAL_MAP_ADDRESS + USE_MEM_ABOVE);
		else
			return s;
	}
	else if(dma || !high.free_pages)
	{
		/*======================================================================
			mem_ready == 1, so we are no longer allocating from the init pool
			Here we want to allocate memory from the low pool page.  At this
			point, we are in paged mode.

			We are going to treat the low pool the same way we did the init.
		======================================================================*/

		for(i = 0; i < low.num_pages; i++)
		{
			if(low.stack[i] == 0)
			{
				off = i;
				break;
			}
		}

		/* Nothing found */
		if(off == -1)
		{
			*ecode = ERC_ALLOCMEM_ERR;
			return NULL;
		}

		/* Convert the offset into an address */
		addr = off * PAGE_SIZE;
		addr += (PHYSICAL_MAP_ADDRESS);
		s = (void*)addr;

		num_page_allocs++;
		low.free_pages--;

		return s;
	}
	else
	{
	//	s = pop_mem(high.stack);
		if(!s)
		{
			*ecode = ERC_INVALID_PAGE;
			return NULL;
		}

		high.free_pages--;
	}

	*ecode = 0;

	num_page_allocs++;

	//maybe we need to add into page tables????
	//lets try that.

	//!!!!NOTE!!!!

	//convert s into a virtual address

	//physical memory is mapped in at PHYSICAL_MAP_ADDRESS
	//since 1:1 mapping, PHYSICAL_MAP_ADDRESS + s will give virt addrress
	/*addr_t virt = (PHYSICAL_MAP_ADDRESS + (addr_t)s);

	//This WONT work because s is a PHYSICAL address not a VIRTUAL!!!
	ULONG pde, pte;

	pde = VIRT_TO_PDE((addr_t)virt);
	pte = VIRT_TO_PTE((addr_t)virt);

	//this should be more generalized(dont code in page_dir)
	addr_t * pt = PDE_TO_PT(page_dir[pde]);


//	pt[pte] = (addr_t)s | PRIV_WR | PRIV_KERN | PRIV_PRES;

	if(map)
	{
	//	debug_printf("\npde: %d pte: %d virt: %d phys: %d", pde, pte, (addr_t)virt, (addr_t)s);
		//while(1);

		//return (void*)virt;
	}*/

	/*if(paging_enabled && !mem_ready)
		return (void*)((PHYSICAL_MAP_ADDRESS + (addr_t)s));
	else*/

	return s;

}

/*==========================================================================
Function: page_free
INPUT:	address of page to deallocate
OUTPUT: None

Deallocates the page specified by addr
*=========================================================================*/
void page_free(addr_t *addr)
{
	ULONG off = 0;

	if(!mem_ready)
		return;


	//LockMutex(&mutex);

	addr = (addr_t*)PAGE_ALIGN(addr);

	if((addr_t)addr < (PHYSICAL_MAP_ADDRESS + (15 * 1025*1024)))
	{
		/* Low stack */
		low.free_pages++;
		/* Convert to offset */
		off = (addr_t)(addr);
		off -= PHYSICAL_MAP_ADDRESS;
		off = off / PAGE_SIZE;

		low.stack[off] = 0;
	}
	else
	{
		/* High stack */
	//	push_mem(high.stack, (addr_t)addr);
		high.free_pages++;
	}

	//UnlockMutex(&mutex);
}


/*==========================================================================
Function: push_mem
INPUT:	previously allocated destination stack
OUTPUT: -1 on failure, 0 success

push_mem is going to push the page addresses onto the stack
*=========================================================================*/
int push_mem(Stack *list, char low, addr_t addr)
{
	ListData *newnode;

	if(list == NULL)
		return -1;	/* Assume stack is already created */

	newnode = (ListData*)kmalloc(sizeof(ListData));
	newnode->next = NULL;
	newnode->prev = list->tail;
	newnode->data = (void*)addr;
    list->tail->next = newnode;
	list->count++;

	return 0;
}

unsigned long AllocatePages(PAGE_POOL * newpool, UINT num, int dma)
{
	if(!mem_ready)
		return ERC_ALLOCMEM_ERR;

	//LockMutex(&mutex);

	/*
	Traverse the page pools looking for multiple pages.
	This may span both page pools
	There is no guarentee that these pages will occupy back
	to back physical addresses.
	*/

	newpool->stack = NULL;
	newpool->num_pages = 0;
	newpool->free_pages = 0;

	UINT i = 0;
	ListData *s;

	UINT pages = (dma) ? PAGE_SIZE : num_pages_on_system;

	for(i = 0; i < pages; i++)
	{
		if(pages < PAGE_SIZE)
		{
			s = pop(low.stack);
			if(!s)
				break;
			low.free_pages--;
		}
		else
		{
			s = pop(high.stack);
			if(!s)
				break;
			high.free_pages--;
		}

		push(newpool->stack, (void*)(s->data));
		newpool->num_pages++;

		//At first I had this, but later I realized that if this routine
		//allocates pages, then it should not increase free_pages at all.
		//newpool->free_pages++;

		if(newpool->num_pages == num)
			break;
	}

	//UnlockMutex(&mutex);

	if(newpool->num_pages != num)
		return ERC_ALLOCMEM_ERR;

	return 0;
}

int SizetoBucketOffset(int size)
{
	if(size <= 0)
		return -1;
	else if (size <= 8)
		return 0;
	else if (size <= 16)
		return 1;
	else if (size <= 32)
		return 2;
	else if (size <= 64)
		return 3;
	else if (size <= 128)
		return 4;
	else if (size <= 256)
		return 5;
	else if (size <= 512)
		return 6;
	else if (size <= 1024)
		return 7;
	else if(size <= 4096)
		return 8;
	else
		return -1;
}

ULONG CreateNewBucket(int size, int low)
{
	ULONG error = 0;
	UINT i;
	int offset = SizetoBucketOffset(size);

	if(offset == -1)
		return ERC_ALLOCMEM_ERR;

	//Find the location on the bucket descriptor page(s) to place this
	//new descriptor.

	BUCKET_DESC * node = NULL;
	BUCKET_DESC * new_desc = NULL;

	//void * new_addr = NULL;

	//void *desc_addr = NULL;
	void *page = NULL;

	char found = 0;

	if(buckets[offset].list == NULL)
	{
		buckets[offset].list = (BUCKET_DESC*)page_alloc(low, &error);

		if(error)
		{
			buckets[offset].list = NULL;
			return error;
		}

		node = buckets[offset].list;

		for(i = 0; i < (PAGE_SIZE / sizeof(BUCKET_DESC)); i++)
		{
			//debug_printf("\nnode: %d", (addr_t)node);

			node->page = NULL;
			node->bmp_page_id = BMP_ID_NONE;
			node->bmp_offset = BMP_OFFSET_NONE;
			node->next = NULL;

			node = (BUCKET_DESC*)((void*)node + sizeof(BUCKET_DESC));
		}

		node = buckets[offset].list;

		mmPagesAllocedInLastkmalloc[0] = (addr_t)node;

		//new_addr = (ULONG)node;
		//desc_addr = (void*)new_addr;

		page = page_alloc(low, &error);
		if(error)
			return ERC_ALLOCMEM_ERR;

		//This is the actual allocated space. Dont put in in the
		//mmPagesAllocedInLastkmalloc array

		//mmPagesAllocedInLastkmalloc[1] = (addr_t)page;

		node->next = NULL;
		node->page = page;

		buckets[offset].expected_free_desc = node;

		return AssociateWithFreeList(node, offset, low);
	}
	else
	{
		//Find the location for the new bucket desc.
		//Find each page of Bucket descriptors.
		//

		node = (BUCKET_DESC*)buckets[offset].list;
		//BUCKET_DESC * last = NULL;
		BUCKET_DESC * t = NULL;

		while(node)
		{
			if(PAGE_ALIGN(node) == (ULONG)node)
			{
				//This is the start of the page
				//Loop through using pointer artihmatic
				//testing each entry to see if it is used.
				//If bmp_page_id == BMP_ID_NONE
				//then this location is available
				//use it. If this condition is never true,
				//then it is time to allocate a new page for
				//the buckets.  *NOTE* We must be SURE to set
				//bmp_page_id to BMP_ID_NONE when deallocating
				//that location.  Also, we must be sure to loop
				//through and set this value for each entry when
				//creating a new page for the descriptors.

				t = node;

				for(i = 0; i < (PAGE_SIZE / sizeof(BUCKET_DESC)); i++)
				{
					if(t->bmp_page_id == BMP_ID_NONE)
					{
						found = 1;
						break;
					}

					t = (BUCKET_DESC*)((void*)t + sizeof(BUCKET_DESC));
				}
			}

			if(found)
				break;

			node = node->next;
		}

		if(!found)
		{
			//We need to allocate a new page for the list
			//Since the above loop iterates until node == NULL, last should
			//be set to the very last node in the linked last.

			void * page = page_alloc(low, &error);
			if(error)
				return ERC_ALLOCMEM_ERR;

			mmPagesAllocedInLastkmalloc[0] = (addr_t)page;

			//Set each entry to BMP_ID_NONE
			BUCKET_DESC * temp = (BUCKET_DESC*)(page);

			for(i = 0; i < (PAGE_SIZE / sizeof(BUCKET_DESC)); i++)
			{
				temp->page = NULL;
				temp->bmp_page_id = BMP_ID_NONE;
				temp->bmp_offset = BMP_OFFSET_NONE;
				temp->next = NULL;

				temp = (BUCKET_DESC*)((void*)temp + sizeof(BUCKET_DESC));
			}

			new_desc = (BUCKET_DESC*)page;

		//debug_printf("\nfound: %d", found);
		//wait(450000);

		}
		else
			new_desc = t;

		/*
		//Find the last bucket descriptor
		while(node->next)
			node = node->next;

		new_addr = ((void*)node + sizeof(BUCKET_DESC));

		//For some reason, this is incrementing by 256 instead of 16 bytes!?!?!?!
		//Fixed...

		if(map)
		{
			debug_printf("\nsizeof(BUCKET_DESC): %d node: %d new_addr: %d", sizeof(BUCKET_DESC), (addr_t)node, (addr_t)new_addr);
			wait(2500000);
		}

		//If the new location is a page boundery, then we have to allocate a
		//new page for the descriptor table.  Since BUCKET_DESC is 16 bytes long
		//there are EXACTLY 256 per 4096 byte page.

		if(PAGE_ALIGN(new_addr) == (addr_t)new_addr)
		{
			ULONG error = 0;

			desc_addr = PageAlloc(0, &error);

			if(error)
				return ERC_ALLOCMEM_ERR;

			page = PageAlloc(0, &error);
			if(error)
				return ERC_ALLOCMEM_ERR;

			mmPagesAllocedInLastkmalloc[0] = (addr_t)desc_addr;
			//mmPagesAllocedInLastkmalloc[1] = (addr_t)page;
		}
		else
		{
			desc_addr = (void*)new_addr;
			page = node->page;
		}*/
	}
	//

	//now allocate the new data page

	page = page_alloc(low, &error);
	if(error)
		return ERC_ALLOCMEM_ERR;

	//this is the allocated page
	//mmPagesAllocedInLastkmalloc[1] = (addr_t)page;

	node = buckets[offset].list;

	while(node->next)
		node = node->next;

	new_desc->next = NULL;
	//new_node->size = buckets[offset].size;
	//new_node->refcnt = 0;
	new_desc->page = page;
	//new_node->bmp_page_id = (buckets[offset].last_bmp_id_used + 1);

	new_desc->bmp_page_id = BMP_ID_NONE;
	new_desc->bmp_offset = BMP_OFFSET_NONE;

	//new_node->bmp_offset = 0;
	//calculating the offset will require some work.  This will
	//be done later on.

	node->next = new_desc;

	buckets[offset].expected_free_desc = new_desc;

	//Most of the work is done in this func.
	return AssociateWithFreeList(new_desc, offset, low);
}

ULONG AssociateWithFreeList(BUCKET_DESC *node, int offset, int low)
{
	BUCKET_FREE_BMP * bmp_list = buckets[offset].bitmap_list;
	ULONG error = 0;
	UINT i;

	if(!bmp_list)
	{
		//We need to allocate a new free list for this bucket chain
		//This is the easy case.
		bmp_list = (BUCKET_FREE_BMP*)page_alloc(low, &error);
		if(error)
			return error;

		BUCKET_FREE_BMP * t = bmp_list;
		for(i = 0; i < (PAGE_SIZE / sizeof(BUCKET_FREE_BMP)); i++)
		{
			t->bmp_page = NULL; //use this to test presence
			t->refcnt = 0;
			t->next = NULL;
			t->bmp_id = BMP_ID_NONE;

			t = (BUCKET_FREE_BMP*)((void*)t + sizeof(BUCKET_FREE_BMP));
		}

		bmp_list->bmp_page = (void*)page_alloc(low, &error);
		if(error)
			return error;

		//if(debug)
		//	debug_printf("\n%d", (addr_t)bmp_list->bmp_page);

		mmPagesAllocedInLastkmalloc[2] = (addr_t)bmp_list;
		mmPagesAllocedInLastkmalloc[3] = (addr_t)bmp_list->bmp_page;

		buckets[offset].bitmap_list = bmp_list;
		bmp_list->next = NULL;

		buckets[offset].last_bmp_id_used = 0;

		bmp_list->bmp_id = 0;
		bmp_list->refcnt = 0;

		ClearBits(bmp_list->bmp_page, SizeToBmpBits(buckets[offset].size));

		node->bmp_page_id = 0;
		node->bmp_offset = 0;

		buckets[offset].expected_free_bmp = bmp_list;

		return 0;
	}

	//Every bitmap corrosponds 1:1 with a Bucket Desc / data page

	//First, test to see if there are any open bitmap offsets which
	//we can use.  If not, allocate a new BUCKET_FREE_BMP.

	bmp_list = buckets[offset].bitmap_list;

	UINT bmp_len = SizeToBmpBits(buckets[offset].size);
	if(bmp_len < 8)
		bmp_len = 8;

	BUCKET_DESC * list = buckets[offset].list;
	UINT coff, cid, bmp_per_page;

	//bmp_len is the number of bits per bitmap so / 8 for bytes
	bmp_per_page = (PAGE_SIZE / (bmp_len / 8));

	char found = 0;
	UINT j;

	char used[512];
	//512 is the maximum number of bitmaps per page(size being allocated is 8 bytes)


	cid = 0;

	while(list)
	{
		cid = list->bmp_page_id;

		BUCKET_DESC * d = buckets[offset].list;

		if(cid == BMP_ID_NONE)
		{
			list = list->next;
			continue;
		}

		for(i = 0; i < 512; i++)
			used[i] = 0;

		while(d)
		{
			if(d->bmp_page_id == cid)// && d->bmp_page_id != BMP_ID_NONE  && d->bmp_offset != BMP_OFFSET_NONE )
			{
			//	debug_printf("\nbp: %d bo: %d", d->bmp_page_id, d->bmp_offset);
				used[d->bmp_offset] = 1;
			}

			d = d->next;
		}

		//now test what was found

		for(i = 0; i < bmp_per_page; i++)
		{
			if(used[i] == 0)
			{
				//debug_printf("\ncid: %d used[%d]: %d", cid, i, used[i]);
				found = 1;
				coff = i;
				break;
			}
		}

		if(found)
			break;

		list = list->next;
	}

	//cid is the free page id and coff is the free offset
	if(found)
	{
		//the following two lines were list->...but I think I wanted node->...
		//not sure though
		node->bmp_page_id = cid;
		node->bmp_offset = coff;

		while(bmp_list)
		{
			if(bmp_list->bmp_id == cid)
				break;

			bmp_list = bmp_list->next;
		}

		ClearBits((void*)(bmp_list->bmp_page + (coff * (bmp_len / 8))), bmp_len);//SizeToBmpBits(buckets[offset].size));

		//The following is done in AllocateMemory()
		/*while(bmp_list)
		{
			if(cid == bmp_list->bmp_id)
				bmp_list->refcnt += 1;

			bmp_list = bmp_list->next;
		}*/

		buckets[offset].expected_free_bmp = bmp_list;

		return 0; //Is that all we have to do????
	}
	else
	{
		//we need to allocate a new BUCKET_FREE_BMP and therefore, a new bmp page

		//First, find where to put the new descriptor

		BUCKET_FREE_BMP * bmp_node = (BUCKET_FREE_BMP*)buckets[offset].bitmap_list;
		BUCKET_FREE_BMP * t = NULL;

		found = 0;
		while(bmp_node)
		{
			if(PAGE_ALIGN(bmp_node) == (ULONG)bmp_node)
			{
				t = bmp_node;

				for(i = 0; i < (PAGE_SIZE / sizeof(BUCKET_FREE_BMP)); i++)
				{
					if(t->bmp_id == BMP_ID_NONE)
					{
						found = 1;
						break;
					}

					t = (BUCKET_FREE_BMP*)((void*)t + sizeof(BUCKET_FREE_BMP));
				}
			}

			if(found)
				break;

			bmp_node = bmp_node->next;
		}

		BUCKET_FREE_BMP * new_bmp = NULL;

		if(found)
		{
			//we found a free spot for the descriptor
			new_bmp = t;
		}
		else
		{
			//we have to allocate a new page for the free descriptor
			new_bmp = (BUCKET_FREE_BMP*)page_alloc(low, &error);
			if(error)
				return error;

			mmPagesAllocedInLastkmalloc[2] = (addr_t)new_bmp;

			t = new_bmp;
			for(i = 0; i < (PAGE_SIZE / sizeof(BUCKET_FREE_BMP)); i++)
			{
				t->bmp_page = NULL; //use this to test availablity
				t->refcnt = 0;
				t->next = NULL;
				t->bmp_id = BMP_ID_NONE;

				t = (BUCKET_FREE_BMP*)((void*)t + sizeof(BUCKET_FREE_BMP));
			}
		}

		//the following lines may not be correct or in the correct location
		new_bmp->bmp_page = page_alloc(low, &error);
		if(error)
			return error;

		mmPagesAllocedInLastkmalloc[3] = (addr_t)new_bmp->bmp_page;

		buckets[offset].last_bmp_id_used+=1;
		new_bmp->bmp_id = buckets[offset].last_bmp_id_used;

		new_bmp->refcnt = 0;

		new_bmp->next = NULL;

		node->bmp_page_id = new_bmp->bmp_id;
		node->bmp_offset = 0;

		bmp_list = buckets[offset].bitmap_list;

		while(bmp_list->next)
			bmp_list = bmp_list->next;

		bmp_list->next = new_bmp;

		//clear every bit on the free list
		//this is easy, just set every byte on the bmp page to 0
		for(i = 0; i < PAGE_SIZE; i++)
			((char*)new_bmp->bmp_page)[i] = 0;

		//This is faster anyways...
		//ClearBits(new_bmp->bmp_page, SizeToBmpBits(buckets[offset].size));

		buckets[offset].expected_free_bmp = new_bmp;

		return 0;
	}

	/*

	//If we are here, then there already exists at least one
	//entry in the free bucket chain.  We have to do some work to
	//find out where to put the free descriptor and whether or not
	//we need to allocate a new page for the bitmap.

	while(list->next)
		list = list->next;

	//list is the final valid entry in this bucket chain

	//We have to first test if we need to allocate a new page
	//for the bitmap.  If we do, then we have to allocate a new
	//page for the free desc, and therefore a new bmp page.  We also
	//need to see if the free desc list needs to allocate a new
	//page.

	//We have the id of the last free bmp used in the Bucket_Dir
	//Search through the Bucket List for all entries that are on
	//that page, and find the last offset that is used among those
	//entries.

	//Here we are looking to see if we can use the info that is already there.
	ULONG last_offset = 0;
	ULONG max = (PAGE_SIZE / buckets[offset].size)-1;

	if(max == 0)
		max = 1;

	BUCKET_DESC * bn = buckets[offset].list;

	BUCKET_FREE_BMP *new_free = NULL;

	//this is not for the free bitmap, this is for the
	//actual data page.
	while(bn)
	{
		if(bn->bmp_page_id == buckets[offset].last_bmp_id_used)
		{
			//debug_printf("\nbp: %d", buckets[offset].last_bmp_id_used);
			if(bn->bmp_offset >= last_offset)
			{
				last_offset = bn->bmp_offset;
			//	debug_printf("\nlo: %d", last_offset);
			}
		}

		if(!bn->next)
			break;

		bn = bn->next;
	}

	if( !(last_offset == max))
	{
		//We can just use the info already there b/c there is still room.

		//The new bmp can go at (size*(last_offset+1)), but
		//we do not need to worry about this here
		node->bmp_page_id = buckets[offset].last_bmp_id_used;
		node->bmp_offset = last_offset++;

		void * bmp = NULL;

		//We have to up the refcnter for the page

		//Since back to back allocatins will most of the time
		//use the same page, testing for this will eliminate the
		//need for a loop every time there is an allocation.
		if(list->bmp_id == buckets[offset].last_bmp_id_used)
		{
			list->refcnt += 1;
			bmp = list->bmp_page;
		}
		else
		{
			BUCKET_FREE_BMP * n = buckets[offset].bitmap_list;

			while(n->next)
			{
				if(n->bmp_id == buckets[offset].last_bmp_id_used)
				{
					n->refcnt += 1;
					bmp = n->bmp_page;
					break;
				}
				if(!n->next)
					break;

				n = n->next;
			}
		}

		if(!bmp) //problem
			return ERC_ALLOCMEM_ERR;

		ClearBits((void*)(bmp + (node->bmp_offset * buckets[offset].size)),
					SizeToBmpBits(buckets[offset].size));

		//That shold be all that we have to do for this....???
	}
	else
	{
		//Allocate more free desc/pages, etc...

		//See if we can fit this new desc on an existing page or
		//if we have to allocate a new page for this new list node.

		if((ULONG)PAGE_ALIGN(list) == (ULONG)(list + sizeof(BUCKET_FREE_BMP)))
		{
			//if list + size of a new node falls on the page boundry of the list
			//then we have to allocate a new one.

			void *a = PageAlloc(0, &error);
			if(error)
				return error;

			mmPagesAllocedInLastkmalloc[4] = (addr_t)a;

			new_free = (BUCKET_FREE_BMP*)a;
		}
		else
			new_free = (BUCKET_FREE_BMP*)(list + sizeof(BUCKET_FREE_BMP));

		//First allocate a new page for the bmp

		void * bmp = PageAlloc(0, &error);
		if(error)
			return error;

		mmPagesAllocedInLastkmalloc[5] = (addr_t)bmp;

		buckets[offset].last_bmp_id_used += 1;
		node->bmp_page_id = buckets[offset].last_bmp_id_used;
		node->bmp_offset = 0;

		new_free->bmp_page = bmp;
		new_free->bmp_id = buckets[offset].last_bmp_id_used;
		new_free->refcnt = 0;

		list->next = new_free;
		new_free->next = NULL;

		ClearBits(bmp, SizeToBmpBits(buckets[offset].size));
	}		*/

	return 1; //unknown error
}

void * kmalloc(size_t size)
{
	void *buf = NULL;


	if(size <= 0 || size > PAGE_SIZE) //change later to the max amount of allocatable space
		return NULL;//ERC_ALLOCMEM_BADSIZE;
	else
	{

		mmPagesAllocedInLastkmalloc[0] = 0;
		mmPagesAllocedInLastkmalloc[1] = 0;
		mmPagesAllocedInLastkmalloc[2] = 0;
		mmPagesAllocedInLastkmalloc[3] = 0;
		mmPagesAllocedInLastkmalloc[4] = 0;
		mmPagesAllocedInLastkmalloc[5] = 0;


		/* get the proper bucket chain */
		int offset = SizetoBucketOffset(size);

		if(offset == -1)
			return NULL;

		BUCKET_DESC * node = buckets[offset].list;

		if(!node)
		{
			if(CreateNewBucket(buckets[offset].size, 0))
			{
				debug_printf("\nCreateNewBucket() error");
				return NULL;
			}
		}


		UINT free_page, free_offset, free_bit;

		BUCKET_DESC found_desc;
		BUCKET_FREE_BMP found_bmp;

		if(FindFreeLocation(offset, &free_page, &free_offset, &free_bit, &found_desc, &found_bmp))
		{
			//A free location was not found, make a new bucket

			if(CreateNewBucket(buckets[offset].size, 0))
			{
				debug_printf("\nCreateNewBucket() error");
				return NULL;
			}

			//This should find the location of the newly allocated bitmap page
			if(FindFreeLocation(offset, &free_page, &free_offset, &free_bit, &found_desc, &found_bmp))
			{
				debug_printf("\nFindFreeLocation() error");
				return NULL; //error
			}
		}


		//free_page is now the id of the free bmp page
		//free_offset is offset into the bmp page specified by free_page
		//free_offset refers to the group of bits. It does *NOT* refer to
		//a specific bit.
		//free_bit refers to the speNUM_LOW_PAGEScific bit
		//debug_printf("\n%d %d", (addr_t)found_desc.page, (addr_t)found_bmp.bmp_page);
		buf = AllocateMemory(offset, free_page, free_offset, free_bit, &found_desc, &found_bmp);

		//if(debug)
		//debug_printf("\nsize: %d free_page: %d free_offset: %d free_bit: %d buf: %d",
		//		buckets[offset].size, free_page, free_offset, free_bit, (addr_t)buf);


		/*
		UINT mult = SizeToBmpBits(buckets[offset].size);
		UINT i, j=0;
		UINT divisor = buckets[offset].size >= 1024 ? 1 : ((PAGE_SIZE / buckets[offset].size) / 8);

		UINT num_bmp_per_page = PAGE_SIZE / divisor;
		char found= 0;

		UINT bo = 0, bp = 0;

		//This is going to be horribly slow!!!

		//if(map)
		//	debug_printf("\nbuckets[offset].size: %d, mult: %d, divisor: %d, num_bmp_per_page: %d", buckets[offset].size, mult, divisor, num_bmp_per_page);

		//Loop through each free node.

		while(node && !found)
		{
			for(i = 0; i < num_bmp_per_page && !found; i++)
			{
				char coff = 0;

				BUCKET_DESC *l = buckets[offset].list;

				while(l->next)
				{
					if(l->bmp_offset == i)
					{
						coff = 1;
						break;
					}
					l = l->next;
				}

				if(!coff)
					continue;

				for(j = 0; j < mult && !found; j++)
				{
					if(!GetBmpStatus(node->bmp_page, buckets[offset].size, i,j))
					{
						//This one is free, so use it.
						found = 1;
						bo = i;
						bp = j;

						//if(map)
						///	debug_printf("\ni: %d j: %d", i, j);

						if(map)
						//{
							debug_printf("...Found");
						//	wait();
						//}

						//if(offset == 8)
						//debug_printf("     %d, %d", i, j);

						break;
					}
				}
				if(found)
					break;
			}
			if(!found)
				node = node->next;
			else
				break;
		}

		if(found)
		{
			//node should be the free bmp descriptor
			//i should be the bmp offset(picks which bitmap to use)
			//j should be the actual bit location inside that bitmap

			buf = AllocateMemory(node, bo, bp, offset);
		}
		else
		{

			//Time to create a new bucket...
			if(CreateNewBucket(buckets[offset].size))
				return NULL;

			goto retry_start;
		}

	*/
	}

	//debug_printf("\nbuf: %d", (addr_t)buf);

	return buf;
}

char FindFreeLocation(int offset, UINT * free_bmp_page, UINT * free_bmp_offset, UINT * free_bmp_bit, BUCKET_DESC * a, BUCKET_FREE_BMP *aa)
{
	//we should probably protect all of these functions with a mutex

	UINT free_page, free_offset, free_bit;
	char found = 0;
	char pass = 1;
	UINT j;

	UINT num_bits_per_bitmap = SizeToBmpBits(buckets[offset].size);

	UINT num_bmps_on_page;

	/*if(num_bits_per_bitmap <= 8)
		num_bmps_per_page = 4096;
	else
		num_bmps_per_page = (PAGE_SIZE / (num_bits_per_bitmap / 8));*/

	BUCKET_FREE_BMP * bmp_list = buckets[offset].bitmap_list;
	BUCKET_DESC * list = buckets[offset].list;

	if(!bmp_list || !list)
	{
		debug_printf("\nFindFreeLoc: error 1, free_bmp_list: %d bmp_list %d", (addr_t)bmp_list, (addr_t)list);
		return 1; //problem...
	}

	/*if(buckets[offset].expected_free_page != BMP_ID_NONE &&
				  buckets[offset].expected_free_offset != BMP_OFFSET_NONE &&
				  buckets[offset].expected_free_bit != BMP_BIT_NONE)
	{
		if(!GetBmpStatus(buckets[offset].expected_free_page, buckets[offset].size,
				  buckets[offset].expected_free_offset, buckets[offset].expected_free_bit))
		{
			found = 1;

			free_page = buckets[offset].expected_free_page;
			free_offset = buckets[offset].expected_free_offset;
			free_bit = buckets[offset].expected_free_bit;

			buckets[offset].expected_free_page = BMP_ID_NONE ;
			buckets[offset].expected_free_offset = BMP_OFFSET_NONE;
			buckets[offset].expected_free_bit = BMP_BIT_NONE;
		}
	}

	if(!found)
	{
		//The expected location is wrong
		//try searching through the last allocated bitmap
	}

	buckets[offset].expected_free_page = 0;
	buckets[offset].expected_free_offset = 0;
	buckets[offset].expected_free_bit = 0;	*/

	//For each page in the free bitmap list for this bucket size,
	//loop through each bitmap on the page and then each bit on that bitmap

	//HORRIBLY, HORRIBLY SLOW!!!!!!!!!!!!!!!!

	BUCKET_DESC * start_desc = NULL;

	start:

	if(pass == 1) //first try, try the last allocated bucket / free bmp
	{
		bmp_list = buckets[offset].expected_free_bmp;
		start_desc = list = buckets[offset].expected_free_desc;
	}
	else if(pass == 2) //not found, then try the whole list.
	{
		bmp_list = buckets[offset].bitmap_list;
		start_desc = list = buckets[offset].list;
	}

	while(bmp_list && !found)
	{
		list = start_desc;

		while(list && !found)
		{
			//debug_printf("\nbmp: %d, bmp->page: %d list: %d list->page: %d", (addr_t)bmp, (addr_t)bmp->bmp_page, (addr_t)list, (addr_t)list->page);

			if(list->bmp_page_id != bmp_list->bmp_id)
			{
				list = list->next;
				continue;
			}

			for(j = 0; j < num_bits_per_bitmap; j++)
			{
				//if(buckets[offset].size == 1024)
				//	debug_printf("\nbmp_id: %d bmp_page: %d, offset: %d bit: %d", list->bmp_page_id, (addr_t)bmp_list->bmp_page, list->bmp_offset, j);

				if(!GetBmpStatus(bmp_list->bmp_page, buckets[offset].size, list->bmp_offset, j))
				{
					found = 1;

					free_page = bmp_list->bmp_id;
					free_offset = list->bmp_offset;
					free_bit = j;

					desc = list;
					bmp = bmp_list;

					break;
				}
			}

			list = list->next;
		}

		bmp_list = bmp_list->next;
	}

	if(!found)
	{
		pass++;

		if(pass == 2)
			goto start;

		buckets[offset].expected_free_bmp = NULL;
		buckets[offset].expected_free_desc = NULL;


		return 1; //not found
	}

	if(free_bit == (num_bits_per_bitmap-1))
	{
		buckets[offset].expected_free_bmp = NULL;
		buckets[offset].expected_free_desc = NULL;
	}


	*free_bmp_page = free_page;
	*free_bmp_offset = free_offset;
	*free_bmp_bit = free_bit;

	return 0;
}

void * AllocateMemory(UINT offset, UINT free_page_id, UINT free_offset, UINT free_bit, BUCKET_DESC * a, BUCKET_FREE_BMP *aa)
{
	BUCKET_DESC * list = buckets[offset].list;
	BUCKET_FREE_BMP * bmp_list = buckets[offset].bitmap_list;

	void * buf = NULL;
	char found = 0;

	if(!list || !bmp_list || !desc || !bmp)
		return NULL; //uh oh! these should be valid by now....

	/*while(list)
	{
		if(list->bmp_page_id == free_page_id && list->bmp_offset == free_offset)
		{
			found = 1;
			break;
		}

		list = list->next;
	}

	if(!found)
		return NULL;

	found = 0;
	while(bmp_list)
	{
		if(bmp_list->bmp_id == free_page_id)
		{
			found = 1;
			break;
		}

		bmp_list = bmp_list->next;
	}

	if(!found)
		return NULL;*/

	list = desc;
	bmp_list = bmp;

	//list is the proper bucket descriptor.
	//bmp_list is the proper free bitmap descriptor.

	buf = (void*)(list->page + (buckets[offset].size * free_bit));

	SetBmpStatus(bmp_list->bmp_page, buckets[offset].size, free_offset, free_bit);

	bmp_list->refcnt += 1;

	return buf;


/*	BUCKET_DESC * b = buckets[bucket_num].list;

	while(b)
	{
			if(map)
			debug_printf("\nb->bmp_page_id: %d node->bmp_id: %d b->bmp_offset: %d bmp_offset: %d bit: %d",
				b->bmp_page_id, node->bmp_id, b->bmp_offset, bmp_offset, bit);

		if(b->bmp_page_id == node->bmp_id && b->bmp_offset == bmp_offset)
		{
			//if(map)
			//debug_printf("\nb->bmp_page_id: %d node->bmp_id: %d b->bmp_offset: %d bmp_offset: %d",
			//	b->bmp_page_id, node->bmp_id, b->bmp_offset, bmp_offset);

			break;
		}

		b = b->next;
	}

	if(!b)
	{
		if(map)
		{
			debug_printf("\nnode->bmp_id: %d bmp_offset: %d bit: %d", node->bmp_id, bmp_offset, bit);
		}
		return NULL;
	}

	//b is the right BUCKET_DESC

	void * data = (b->page + (buckets[bucket_num].size * bit));

	//debug_printf("b->page: %d, data: %d", (addr_t)b->page, (addr_t)data);

	//b->page is page that the data is on
	//We need to calculate the proper address on this page.

	//set the bit in the bitmap
	//debug_printf("\nnode->bmp_page: %d size: %d, bmp_offset: %d bit: %d",
	//			node->bmp_page, buckets[bucket_num].size, bmp_offset, bit);
	//debug_printf("\nbmp: %d", ((char*)node->bmp_page)[0]);
	SetBmpStatus(node->bmp_page, buckets[bucket_num].size, bmp_offset, bit);
	//debug_printf("\nbmp: %d", ((char*)node->bmp_page)[0]);

	node->refcnt += 1;

	return data;*/
}

void kfree(void *buf)
{
	//find the proper bucket_desc

	if(!buf)
		return;

	BUCKET_DESC *d = NULL, * desc = NULL, *list = NULL;
	BUCKET_FREE_BMP * bmp;
	BUCKET_FREE_BMP * prev = NULL;

	UINT offset, i = 0;
	char found = 0;
	UINT count = 0;

	for(i = 0; !found && i < NUM_DESC; i++)
	{
		d = buckets[i].list;

		while(d && !found)
		{
			if(PAGE_ALIGN(buf) == (addr_t)d->page)
			{
				found = 1;
				offset = i;
				desc = d;
				break;
			}

			d = d->next;
		}
	}

	if(!found || !desc) //problem
		return;

	//desc is the right bucket_desc

	bmp = buckets[offset].bitmap_list;
	if(!bmp) //problem
		return;

	while(bmp)
	{
		if(bmp->bmp_id == desc->bmp_page_id)
			break;

		bmp = bmp->next;
	}

	if(!bmp) //problem
		return;

	UINT j = 0;

	//possible bug
	//Find the proper bit, assign it to j

	j = (((addr_t)buf - (addr_t)desc->page) / buckets[offset].size);

	ClearBmpStatus(bmp->bmp_page, buckets[offset].size, desc->bmp_offset, j);

	//
	count = 0;
	ULONG bits = SizeToBmpBits(buckets[offset].size);
	for(i = 0; i < bits; i++)
	{
		if(GetBmpStatus(bmp->bmp_page, buckets[offset].size, desc->bmp_offset, i))
			count++;
	}

	if(count)
		return;

	bmp->refcnt--;

	if(buckets[offset].expected_free_bmp == bmp)
		buckets[offset].expected_free_bmp = NULL;

	if(bmp->refcnt == 0)
	{
		//We can free this free bmp now

		page_free(bmp->bmp_page);

		bmp->bmp_page = NULL;
		bmp->bmp_id = BMP_ID_NONE;

		//We can now take this descriptor out of the list
		//Problems arise while doing this because the taking of a
		//page out of the linked list will mess up the
		//list if not careful.

		BUCKET_FREE_BMP * bmp_list = buckets[offset].bitmap_list;

		UINT num_bmp = 0;
		UINT total_bmp = 0;

		while(bmp_list)
		{
			if(PAGE_ALIGN(bmp_list) == PAGE_ALIGN(bmp)) //if they are in the same page
				num_bmp++;

			if((addr_t)bmp_list->next == (addr_t)bmp)
				prev = bmp_list;

			bmp_list = bmp_list->next;
			total_bmp++;
		}

		if(total_bmp == 1) //this is the last one, so set the list to NULL
		{
			buckets[offset].expected_free_bmp = NULL;
			buckets[offset].bitmap_list = NULL;
		}

		if(num_bmp == 1) //if this is the only(last) bmp on this page, deallocate
		{
			if(bmp->next == NULL) //we can just go ahead and deallocate
			{
				page_free((void*)bmp);
				bmp = NULL;

				prev->next = NULL;

				//Is that all we need to do???
			}
			else
			{
				//the bmp is the last one on the bmp page, but is *NOT* the last one
				//in the bmp list so we need to fix up pointers

				prev->next = bmp->next;

				bmp->next = NULL;

				page_free((void*)bmp);

				bmp = NULL;
			}
		}
		else //we just want to remove it from the list(NO deallocation)
		{
			prev->next = bmp->next;
			bmp->next = NULL;
			bmp = NULL;

			//this will get deallocated when the last bmp on the page is freed.
		}
	}
	else
		return;//???????

	//The bmp list is done, now do the bucket desc list
	//This should work in a very similar way to the bitmap list

	UINT num_desc = 0;
	UINT total_desc = 0;

	list = buckets[offset].list;
	if(!list)
		return; //problem...

	BUCKET_DESC * prev_desc = NULL;

	if(buckets[offset].expected_free_desc == desc)
		buckets[offset].expected_free_desc = NULL;

	while(list)
	{
		if(PAGE_ALIGN(list) == PAGE_ALIGN(desc))
			num_desc++; //on page

		if((addr_t)list->next == (addr_t)desc)
			prev_desc = list;

		list = list->next;

		total_desc++;
	}

	if(total_desc == 1)
	{
		buckets[offset].expected_free_desc = NULL;
		buckets[offset].list = NULL;
	}

	page_free(desc->page);

	desc->bmp_page_id = BMP_ID_NONE;
	desc->bmp_offset = BMP_OFFSET_NONE;

	if(num_desc == 1) //This is the last on the page
	{
		if(desc->next == NULL) //This is the last one remaining on the page and in the list
		{
			prev_desc->next = NULL;

			page_free((void*)desc);
			desc = NULL;
		}
		else //This is the last one remaining on the page, but *NOT* in the list
		{
			prev_desc->next = desc->next;
			desc->next = NULL;

			page_free((void*)desc);

			desc = NULL;
		}
	}
	else
	{
		prev_desc->next = desc->next;
		desc->next = NULL;
		desc = NULL;
	}
}

char inline GetBmpStatus(void *t, UINT size, UINT offset, UINT loc)
{
	//t is the location of the bmp page
	//size is the size of this bucket
	//offset is the offset into the bmp page
	//loc is the bit location to test for

	UINT mult = SizeToBmpBits(size) / 8;
	if(mult == 0)
		mult = 1;

	//This means that each bmp is 'mult' bytes long.

	return GetBitState((void*)(t + (mult * offset)), loc);
}

void inline SetBmpStatus(void *t, UINT size, UINT offset, UINT loc)
{
	//t is the location of the bmp page
	//size is the size of this bucket
	//offset is the offset into the bmp page
	//loc is the bit location to test for

	UINT mult = SizeToBmpBits(size) / 8;
	if(mult == 0)
		mult = 1;

	//This means that each bmp is 'mult' bytes long.

	SetBit((void*)(t + (mult * offset)), loc);
}

void inline ClearBmpStatus(void *t, UINT size, UINT offset, UINT loc)
{
	//t is the location of the bmp page
	//size is the size of this bucket
	//offset is the offset into the bmp page
	//loc is the bit location to test for

	UINT mult = SizeToBmpBits(size) / 8;
	if(mult == 0)
		mult = 1;

	//This means that each bmp is 'mult' bytes long.

	ClearBit((void*)(t + (mult * offset)), loc);
}

UINT inline SizeToBmpBits(UINT size)
{
	if(size == 1024)
		return 4;
	else if(size == 4096)
		return 1;

	return (PAGE_SIZE / size);
}


void inline ClearBits(void *t, UINT size)
{
	if(!t)
		return;

	//size is the number of bits in the bucket
	char *data = (char *)t;
	UINT len = (size / 8);
	UINT i;

	if(len < 8)
		len = 8;

	for(i = 0; i < len; i++)
		data[i] = 0;
}



unsigned long CreateAddressSpace(ADDR_SPACE *addr, char os)
{
	if(!os) //because the os pd is already setup
	{
		ULONG res = CreatePageDir(addr);
		if(res)
			return res;

		//res = CreateMapping(addr, (addr_t)PHYSICAL_MAP_ADDRESS, 0,
		//				total_system_memory_bytes, PRIV_WR | PRIV_PRES | PRIV_KERN);

		addr->CodeSeg = 0x18; //for OS task...0x18 for user level tasks
		addr->sCodeSeg = 0xffffffff;
		addr->DataSeg = 0x10;
		addr->sDataSeg = 0xffffffff;
		//Is the size of the segments 0xffffffff (4 gigs) or is it 0xFFFFF like in the gdt?

		//0 for os.
		addr->AddrStack = NULL;
		addr->sStack = 0;

		return res;
	}
	else
	{
		addr->page_dir = (addr_t*)((void*)kernel_addr_space.page_dir + PHYSICAL_MAP_ADDRESS);
		addr->os_page_table = (addr_t*)((void*)kernel_addr_space.os_page_table + PHYSICAL_MAP_ADDRESS);

		addr->CodeSeg = 0x8; //for OS task...0x18 for user level tasks
		addr->sCodeSeg = 0xffffffff;
		addr->DataSeg = 0x10;
		addr->sDataSeg = 0xffffffff;
		//Is the size of the segments 0xffffffff (4 gigs) or is it 0xFFFFF like in the gdt?

		//0 for os.
		addr->AddrStack = NULL;
		addr->sStack = 0;
	}

	return 0;
}

ULONG initAddressSpace(ADDR_SPACE *addr, char os, void *pStack, TaskState * tss)
{


	return 0;
}

unsigned long CreatePageDir(ADDR_SPACE *as)
{
	ULONG res, i;

	void *pt, *pd;

	pd = kmalloc(4096);

	if(pd == NULL)
		return 1;

	pt = kmalloc(4096);

	if(pt == NULL)
		return res;

	as->page_dir = pd;
	as->os_page_table = pt;

	for(i = 0; i < 1024; i++)
	{
		as->page_dir[i] = 0;
		as->os_page_table[i] = 0;
	}

	as->page_dir[0] = (addr_t)pt | PRIV_USER | PRIV_PRES | PRIV_WR;

	return 0;
}

/*==========================================================================
Function: create_mapping
INPUT:	destination address space,
		virtual address to map to,
		physical address to be mapped,
		physical address to stop mapping at.
		privilage level
OUTPUT: 0 on success.
Maps a range of physical memory to virtual memory in a 1:1 fashion
*=========================================================================*/
unsigned long create_mapping(ADDR_SPACE *addr, addr_t virt, addr_t phys, addr_t phys_end, char priv)
{
	addr_t virt_end;
	addr_t pde = NULL;
	addr_t pte = NULL;
	ULONG err = 0;
	UINT j, i;

	virt = PAGE_ALIGN(virt);
	phys = PAGE_ALIGN(phys);
	virt_end = PAGE_ALIGN(virt+phys_end);

	//debug_printf("\nMapping Memory:\nphys: %d\nphys_end: %d\nvirt: %d\nvirt_end: %d\n", phys, phys_end,virt, virt_end);

	for(	; virt < virt_end; virt+=PAGE_SIZE, phys+=PAGE_SIZE)
	{
		pde = VIRT_TO_PDE(virt);
		pte = VIRT_TO_PTE(virt);

		//debug_printf("\nvirt %d phys %d pde: %d pte: %d", virt, phys, pde, pte);

		if((addr_t*)addr->page_dir[pde] == NULL)
		{
			//allocate a new page table
			//debug_printf("\n%d ", virt);
			void * pt = page_alloc(1, &err);//kmalloc(4096);

			if(pt == NULL)
				return 1;

			pt = (void*)(pt);

			//debug_printf("pt\n");

			//add the new page table into the page directory
			addr->page_dir[pde] = (addr_t)pt | PRIV_RD | PRIV_PRES | PRIV_USER;
		}

		addr_t * pt = NULL;

		pt = PDE_TO_PT(addr->page_dir[pde]);


		pt[pte] = (addr_t)phys | priv;
	}

	return 0;
}
#endif
