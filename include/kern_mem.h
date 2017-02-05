#ifndef __KERN_MEM_H_
#define __KERN_MEM_H_

#include "../include/types.h"
//#include "..\include\extern.h"
#include "../include/basicadt.h"

#include "../include/tss.h"
#include "../include/util.h"

#define NUM_DESC 9
#define PRIV_PRES       0x001   /* present */
#define PRIV_RD         0x000   /* read-only */
#define PRIV_WR         0x002   /* writable */
#define PRIV_KERN       0x000   /* ring 0 */
#define PRIV_USER       0x004   /* ring 3 */
#define PRIV_ALL        0x007
#define PAGE_ACCESSED   0x020
#define PAGE_DIRTY	    0x040
#define PAGE_SIZE       4096
#define PHYSICAL_MAP_ADDRESS 0xC0000000
#define PAGE_ALIGN(X) 	((addr_t)(X) & -PAGE_SIZE)
#define PAGE_ALIGN_UP(X) (((addr_t)(X) + PAGE_SIZE - 1) & -PAGE_SIZE)
#define PDE_TO_PT(addr) ( (addr_t*) (addr & 0xFFFFF000) )
#define VIRT_TO_PDE(a) (a >> 22)
#define VIRT_TO_PTE(a) ((a << 10) >> 22)
#define VIRT_TO_OFFSET (a & 0x3FF)
#define PHYS_TO_VIRT(addr) (addr + PHYSICAL_MAP_ADDRESS)
#define BMP_ID_NONE 0xFFFFFFFF
#define BMP_OFFSET_NONE BMP_ID_NONE
#define BMP_BIT_NONE BMP_OFFSET_NONE
#define NUM_INIT_PAGES 4096
#define PAGE_SIZE 4096
#define USE_MEM_ABOVE 0x100000
#define NUM_LOW_PAGES 3840 /* 15 MB worth */
#define MAX_ADDRESSABLE_BYTES 0xFFFFFFFF
#define MAX_ADDRESSABLE_PAGES MAX_ADDRESSABLE_BYTES / PAGE_SIZE

typedef struct page_pool {

	Stack *stack;

	unsigned int num_pages;
	unsigned int free_pages;

} PAGE_POOL;

typedef struct page_pool_init {

	addr_t *stack;

	unsigned int num_pages;
	unsigned int free_pages;

} PAGE_POOL_INIT;

typedef struct address_space {

	addr_t *page_dir;
	addr_t *os_page_table;

	ULONG CodeSeg;		// ;Address of code segment
	ULONG DataSeg;		// ;Address of data segment
	ULONG sCodeSeg;		// ;sizeof code segment
	ULONG sDataSeg;		// ;sizeof data segment

	void* AddrStack;	// ;Address of stack
	ULONG sStack;		// ;sizeof stack

} ADDR_SPACE;

typedef struct address_space AddressSpace;


/* Every kernel allocated physical page has an associated bucket descriptor */
typedef struct bucket_desc {

	void *page;

	ULONG bmp_page_id;
	ULONG bmp_offset;

	struct bucket_desc *next;

} BUCKET_DESC;

typedef struct bucket_free_bitmap {

	ULONG bmp_id;
	ULONG refcnt; /* This is the number of bucket desc w/ bmp_page_id == bmp_id.
			    	When this is 0, we can deallocate this bucket free desc(and bmp page) */

	void *bmp_page;

	struct bucket_free_bitmap *next;

} BUCKET_FREE_BMP;

typedef struct bucket_chain {

	UINT size;
	ULONG last_bmp_id_used;

	struct bucket_desc *expected_free_desc;
	struct bucket_free_bitmap *expected_free_bmp;
	struct bucket_desc *list;
	struct bucket_free_bitmap *bitmap_list;

} BUCKET_CHAIN;

int init_low_level_mem();
void * allocate_from_image_end(size_t size);
void * kmalloc(size_t size);
void kfree(void *);
void * allocate(size_t size);


void init_pool(PAGE_POOL *pool, unsigned long start, unsigned long end);

ULONG CreateNewBucket(int size, int low);
ULONG AssociateWithFreeList(BUCKET_DESC *node, int offset, int low);
void * AllocateMemory(UINT, UINT, UINT, UINT, BUCKET_DESC * , BUCKET_FREE_BMP *);

UINT inline SizeToBmpBits(UINT size);
int inline SizetoBucketOffset(int size);

void inline ClearBits(void *t, UINT size);

char inline GetBmpStatus(void *t, UINT size, UINT offset, UINT loc);
void inline SetBmpStatus(void *t, UINT size, UINT offset, UINT loc);
void inline ClearBmpStatus(void *t, UINT size, UINT offset, UINT loc);

void * page_alloc();
void page_free(addr_t *addr);

unsigned long CreateAddressSpace(ADDR_SPACE *addr, char os);
ULONG initAddressSpace(ADDR_SPACE *addr, char os, void *pStack, TaskState * tss);

ULONG CreatePageDir(ADDR_SPACE *);

unsigned long AllocatePages(PAGE_POOL *, UINT num, int dma);
void * kmalloc_low(size_t size);
unsigned long create_mapping(ADDR_SPACE *, addr_t virt, addr_t phys, addr_t len, char priv);

char FindFreeLocation(int offset, UINT * free_bmp_page, UINT * free_bmp_offset, UINT *free_bmp_bit, BUCKET_DESC * desc, BUCKET_FREE_BMP *bmp);


#endif
