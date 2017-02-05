#ifndef __TSS_H_
#define __TSS_H_

#include "../include/defs.h"

//#include "../include/extern.h"

typedef struct TSS_s TaskState;
typedef struct TaskData_s TaskData;

//#include "../include/ipc.h"

#define TSS_CACHE_SIZE 128

#define CACHE_TO_SEL(loc) (tss_sel_loc + (loc * 8))

struct TaskData_s
{
	char  TD_Name[32];//	EQU 8 ; resb 16			;Name of Task
	char TD_Path[160];//	EQU 52; resb 128			;Path(on Disk) of task
	char TD_CmdLine[160];//	EQU 180;resb 128			;Command Line Params
	int TD_ExitCode;// 		EQU 302;dw 0x0000			;error/success code returned to OS when task is ended
	void* TD_VideoMem;//	EQU 304;dd 0x00000000		;pointer to address of active video buffer
	void* TD_VirtVid;//		EQU 308;dd 0x00000000		;pointer to address of virtual vid buffer
	int TD_CurX;//			EQU 312;dw 0x0000			;cursor x position
	int TD_CurY;//			EQU 314;dw 0x0000			;cursor y position
	int TD_nColumns;//		EQU 316;dw 0x0000			;number of columns on screen
	int TD_nLines;//		EQU 318;dw 0x0000			;number of lines on screen
	char TD_VidMode;//		EQU 320;db 0x00			;task's video mode(0 - 80x25 VGA color text)
	char TD_NormVid;//		EQU 321;db 0x00			;Specifies what the normal vid mode is(7 - white on black)
	char TD_Cursor;//		EQU 322;db 0x00			;cursor on(1) / cursor off(0)
	char TD_CursorType;//    EQU 323;db 0x00			;0 = underline 1 = Block
	char TD_SysIn[50];//	EQU 324;resb 50			;std input
	char TD_SysOut[50];//	EQU 374;resb 50			;std output

	//char padding[64];

};

struct TSS_s
{
	USHORT backlink, _blh;
	ULONG esp0;	
	USHORT ss0, _ss0;
	ULONG esp1;		
	USHORT ss1, _ss1;
	ULONG esp2;
	USHORT ss2, _ss2;
	ULONG cr3;	
	ULONG eip;		
	ULONG eflags;			
	ULONG eax;			
	ULONG ecx;			
	ULONG edx;			
	ULONG ebx;			
	ULONG esp;			
	ULONG ebp;			
	ULONG esi;			
	ULONG edi;			
	USHORT es, _es;		
	USHORT cs, _cs;		
	USHORT ss, _ss;		
	USHORT ds, _ds;		
	USHORT fs, _fs;		
	USHORT gs, _gs;		
	USHORT ldt, _ldt;		
	USHORT bits;			
	USHORT iomapbase;


	//software state for TSS	
	ULONG selector;
	char dpl;	
	
	char priority;	
	char inCache;

	ULONG tick_created;				
	
	//Due to some lazy hardcoding in offsets in tss.asm, any additional info
	//should be placed after here.  If the above fields are changed in any way,
	//then tss.asm must be changed.
	
	ULONG tick_last_run;	
	char gdtDescriptorInfo;

	char padding[388]; //make the structure 512 bytes long

};

ULONG initTSS(TaskState *tss, ULONG pid, char os, char pri, char, char, addr_t * page_dir);
ULONG initTD(TaskData *td, char *, char *, char*);

void forceToCache(TaskState *tss, char loc);
void addTSStoCache(TaskState *tss);

void inline clearCache();
ULONG inline CG_GetCurrentTick();
ULONG inline copyTSS(TaskState *dest, TaskState * src);

ULONG inline SetBitLong(char bit, ULONG data);
char inline GetBitStateLong(char bit, ULONG data);

void AddTSStoGDT(addr_t tssaddr, long selector, long limit, char properties);

#endif
