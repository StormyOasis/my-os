;idt.asm
;Defines idt and associated helper functions

[BITS 32]

%ifndef __IDT_ASM
%define __IDT_ASM

PIC1_PORT1 equ 0x20
PIC1_PORT2 equ 0x21
PIC2_PORT1 equ 0xa0
PIC2_PORT2 equ 0xa1


extern _MAX_PROCS

extern enQueueTSS
extern enQueueTSS
extern deQueueTSS
extern CheckReadyQueue
extern debug_printf
extern intTimerHandler
extern reboot

[SECTION .data]
pf_msg: db 10,"Page fault at address: %d",0
unhandled_msg: db 10, "Unhandled exception", 0
div0_msg: db 10, "Divide by 0 exception", 0
invalidopcode_msg: db 10, "Invalid opcode exception", 0
invalid_tss_msg: db 10, "Invalid tss exception", 0
double_ex_msg: db 10, "Double exception", 0
gpf_msg: db 10, "General protection fault",0
overflow_msg: db 10, "Overflow exception",0
stack_overflow_msg db 10, "Stack overflow exception",0
seg_not_present_msg: db 10, "Segment not present exception",0

[SECTION .text]
;========================================================================
;InitIDTSubsystem - NEAR
;
;IN: none
;OUT: none
;
;contains init calls for idt
;

InitIDTSubsystem:

	;get the idt base and limit
	;SIDT [IDTptr]	
	
	LIDT [idtr]
	
	;Do this so that the reset will work
;	jmp GAE	
	jmp GeneralAssignStart

	;first copy blank idt to base address
	;mov ecx,40
	;mov eax,10h
	;mov ds,ax
	;mov es,ax
	;mov esi,idt
	;mov edi,0	
	;rep movsd	
					
	
	
	;idt should now be at address 0h
	;can now begin adding call gates
	;NOTE IDT NO LONGER AT 0! HARDCODED into data.inc
	
	;first assign generic handler to each interrupt		
	MOV ECX, 255
	
GeneralAssignStart:

	PUSH ECX
	
	MOV EAX, 0x08F00	;DPL 3, Trap
	MOV EBX, 0x8		;code selector
	MOV ESI, IntQ		;generic handler
	
	CALL 0x48:0x00		;__AddIDTGate
	
	POP ECX
	LOOP GeneralAssignStart

GAE:
	
;begin individual assignments

	MOV ECX, 0		;divide by 0
	MOV EAX, 0x08F00	;DPL 3, Trap gate
	MOV EBX, 0x8
	MOV ESI, __IntDivBy0
	call 0x48:0x00
	
	MOV ECX, 1		;single step
	MOV EAX, 0x08F00	;DPL 3, Trap gate
	MOV EBX, 0x8
	MOV ESI, __IntSingleStep
	call 0x48:0x00	
	
	MOV ECX, 3		;breakpoint
	MOV EAX, 0x08E00	;DPL 3, Trap gate
	MOV EBX, 0x8
	MOV ESI, __IntBreakpoint
	call 0x48:0x00	
	
	MOV ECX, 4		;overflow
	MOV EAX, 0x08F00	;DPL 3, Trap gate
	MOV EBX, 0x8
	MOV ESI, __IntOverflow
	call 0x48:0x00		
	
	MOV ECX, 6		;invalid opcode
	MOV EAX, 0x08F00	;DPL 3, Trap gate
	MOV EBX, 0x8
	MOV ESI, __IntInvalidOpCode
	call 0x48:0x00	
	
	MOV ECX, 8		;double exception
	MOV EAX, 0x08F00	;DPL 3, Trap gate
	MOV EBX, 0x8
	MOV ESI, __IntDoubleException
	call 0x48:0x00		
	
	MOV ECX, 0xA		;invalid tss
	MOV EAX, 0x08F00	;DPL 3, Trap gate
	MOV EBX, 0x8
	MOV ESI, __IntInvalidTSS
	call 0x48:0x00		

	MOV ECX, 0xB		;segment not present
	MOV EAX, 0x08F00	;DPL 3, Trap gate
	MOV EBX, 0x8
	MOV ESI, __IntSegmentNotPresent
	call 0x48:0x00		
	
	MOV ECX, 0xC		;stack overflow
	MOV EAX, 0x08F00	;DPL 3, Trap gate
	MOV EBX, 0x8
	MOV ESI, __IntStackOverflow
	call 0x48:0x00	
	
	MOV ECX, 0xD		;general protection fault
	MOV EAX, 0x08F00	;DPL 3, Trap gate
	MOV EBX, 0x8
	MOV ESI, __IntGPF
	call 0x48:0x00	
	
	MOV ECX, 0xE		;page fault
	MOV EAX, 0x08F00	;DPL 3, Trap gate
	MOV EBX, 0x8
	MOV ESI, __IntPageFault
	call 0x48:0x00	
	
	MOV ECX, 0x20		;timer (IRQ 0)
	MOV EAX, 0x08F00	;DPL 3, Trap gate
	MOV EBX, 0x8
	MOV ESI, __IntTimer
	call 0x48:0x00	
	
	MOV ECX, 0x21		;keyboard (IRQ 1)
	MOV EAX, 0x08E00	;DPL 3, Trap gate
	MOV EBX, 0x8
	MOV ESI, __IntKeyboard
	call 0x48:0x00	
	
	MOV ECX, 0x22  		;picu2 from picu (IRQ 2)
	MOV EAX, 0x08E00	;DPL 3, Trap gate
	MOV EBX, 0x8
	MOV ESI, __IntPICU2
	call 0x48:0x00					
	
	;now add each basic idt entry to a generic handler
	
	MOV ECX, 0x23		;com2
	MOV EAX, 0x8E00	;dpl 3, int gate
	MOV EBX, 0x8
	MOV ESI, IntQ
	call 0x48:0x00										
	
	MOV ECX, 0x24		;com1
	MOV EAX, 0x8E00	;dpl 3, int gate
	MOV EBX, 0x8
	MOV ESI, IntQ
	call 0x48:0x00	
	
	MOV ECX, 0x25		;LPT2
	MOV EAX, 0x8E00	;dpl 3, int gate
	MOV EBX, 0x8
	MOV ESI, IntQ
	call 0x48:0x00
	
	MOV ECX, 0x26		;Floppy
	MOV EAX, 0x8E00	;dpl 3, int gate
	MOV EBX, 0x8
	MOV ESI, IntQ
	call 0x48:0x00	
	
	MOV ECX, 0x27		;LPT1
	MOV EAX, 0x8E00	;dpl 3, int gate
	MOV EBX, 0x8
	MOV ESI, IntQ
	call 0x48:0x00
	
	MOV ECX, 0x28		;general
	MOV EAX, 0x8E00	;dpl 3, int gate
	MOV EBX, 0x8
	MOV ESI, IntQ
	call 0x48:0x00		
	
	MOV ECX, 0x29		;general
	MOV EAX, 0x8E00	;dpl 3, int gate
	MOV EBX, 0x8
	MOV ESI, IntQ
	call 0x48:0x00
	
	MOV ECX, 0x2a		;general
	MOV EAX, 0x8E00	;dpl 3, int gate
	MOV EBX, 0x8
	MOV ESI, IntQ
	call 0x48:0x00
	
	MOV ECX, 0x2b		;general
	MOV EAX, 0x8E00	;dpl 3, int gate
	MOV EBX, 0x8
	MOV ESI, IntQ
	call 0x48:0x00	
	
	MOV ECX, 0x2c		;general
	MOV EAX, 0x8E00	;dpl 3, int gate
	MOV EBX, 0x8
	MOV ESI, IntQ
	call 0x48:0x00		
	
	MOV ECX, 0x2d		;general
	MOV EAX, 0x8E00	;dpl 3, int gate
	MOV EBX, 0x8
	MOV ESI, IntQ
	call 0x48:0x00				
	
	MOV ECX, 0x2e		;general
	MOV EAX, 0x8E00	;dpl 3, int gate
	MOV EBX, 0x8
	MOV ESI, IntQ
	call 0x48:0x00				
	
	MOV ECX, 0x2f		;general
	MOV EAX, 0x8E00	;dpl 3, int gate
	MOV EBX, 0x8
	MOV ESI, IntQ
	call 0x48:0x00										
	
	;that's it... for now

	retn
	
;========================================================================
;INTQ - FAR - int proc
;
;IN: NONE
;OUT: NONE
;
;generic interrupt handler.  places an I on screen.
;

IntQ:
	cli
	PUSH EAX		
	
	mov byte [es:dword 0xB8000],'I'
	
	PUSH word 0
	call 0x50:0x00			
	
	
	POP EAX
	sti
	IRETD	

;========================================================================
;__AddIDTGate - FAR
;
;Builds an IDT trap, interrupt, or task gate
;
;IN:	AX - word. specifies type of gate. 
;			Trap Gate with DPL 3	0x8F00
;			Int Gate with DPL 3	  	0x8E00
;			Task Gate with DPL 3	0x8500
;	BX - Gate selector number(OSCode or TSS selector)
;	CX - Word with Int Num
;	ESI - offset of entry point in os code(0 for task gates)
;
;OUT: none
;

__AddIDTGate:

	MOVZX EDX, CX
	SHL EDX, 3
	ADD EDX, [IDTBase]		;edx is now pointing to proper gate location
	
	MOV WORD [EDX+04], AX	;place gate id into idt
	
	MOV EAX, ESI
	MOV WORD [EDX], AX		;offset 0..15 into gate
	
	SHR EAX, 16
	MOV WORD [EDX+06], AX	;offset 16..31 into gate
	
	MOV WORD [EDX+02], BX    ;don't forget the selector!
	
	retf
	
;=========================================================================
;
;ExceptionRoot - Base exception handler
;
;not complete
;

__ExceptionRoot:

	cli

	push eax
	;push edx	
		
	mov eax, unhandled_msg	
	push eax
	call debug_printf
	add esp, 4	

	hlt
	IRETD
	
;=========================================================================
;__IntDivBy0 - Far - Trap
;
;IN: NONE
;OUT: NONE
;
;Not complete
;

__IntDivBy0:

	cli
	push eax		
	mov eax, div0_msg	
	push eax
	call debug_printf
	add esp, 4	

	hlt
	
D0_LOOP:
	jmp D0_LOOP		

	jmp __ExceptionRoot
	
;=========================================================================
;__IntSingleStep - Far - Trap
;
;IN: NONE
;OUT: NONE
;
;Not complete
;

__IntSingleStep:

	jmp __ExceptionRoot
	
;=========================================================================
;__IntBreakpoint - Far - Trap
;
;IN: NONE
;OUT: NONE
;
;Not complete
;

__IntBreakpoint:

	jmp __ExceptionRoot	
	
;=========================================================================
;__IntOverflow - Far - Trap
;
;IN: NONE
;OUT: NONE
;
;Not complete
;

__IntOverflow:

	cli
	push eax		
	mov eax, overflow_msg	
	push eax
	call debug_printf
	add esp, 4	

	hlt

	jmp __ExceptionRoot		
	
;=========================================================================
;__IntInvalidOpCode - Far - Trap
;
;IN: NONE
;OUT: NONE
;
;Not complete
;

__IntInvalidOpCode:

	cli
	push eax		
	mov eax, invalidopcode_msg
	push eax
	call debug_printf
	add esp, 4	

	hlt

	jmp __ExceptionRoot			
	
;=========================================================================
;__IntDoubleException - Far - Trap
;
;IN: NONE
;OUT: NONE
;
;Not complete
;

__IntDoubleException:
	cli
	push eax		
	mov eax, double_ex_msg
	push eax
	call debug_printf
	add esp, 4	

	hlt
	jmp __ExceptionRoot			
	
;=========================================================================
;__IntInvalidTSS - Far - Trap
;
;IN: NONE
;OUT: NONE
;
;Not complete
;

__IntInvalidTSS:
	cli
	push eax		
	mov eax, invalid_tss_msg	
	push eax
	call debug_printf
	add esp, 4	

	hlt
	jmp __ExceptionRoot			
	
;=========================================================================
;__IntSegmentNotPresent - Far - Trap
;
;IN: NONE
;OUT: NONE
;
;Not complete
;

__IntSegmentNotPresent:

	cli
	push eax		
	mov eax, seg_not_present_msg	
	push eax
	call debug_printf
	add esp, 4	

	hlt

	jmp __ExceptionRoot
	
;=========================================================================
;__IntStackOverflow - Far - Trap
;
;IN: NONE
;OUT: NONE
;
;Not complete
;

__IntStackOverflow:
	cli
	push eax		
	mov eax, stack_overflow_msg	
	push eax
	call debug_printf
	add esp, 4	

	hlt

	jmp __ExceptionRoot
	
;=========================================================================
;__IntGPF - Far - Trap
;
;IN: NONE
;OUT: NONE
;
;Not complete
;

;extern _SDB

__IntGPF:

	cli
	push eax		
	mov eax, gpf_msg	
	push eax
	call debug_printf
	add esp, 4	

	hlt

GPF_LOOP:
	jmp GPF_LOOP
	jmp __ExceptionRoot	
	
;=========================================================================
;__IntPageFault - Far - Trap
;
;IN: NONE
;OUT: NONE
;
;Not complete
;

__IntPageFault:

	cli

	push eax
	push edx
	
	mov edx, cr2
		
	mov eax, pf_msg
	
	push edx
	push eax
	call debug_printf
	add esp, 8

	;mov byte [es:dword 0xB8000],'P'	
	
	hlt		

PF_LOOP:
	jmp PF_LOOP	
	
	pop edx
	pop eax
	
	jmp __ExceptionRoot	

;=========================================================================
;__IntTimer - Far - Trap - IRQ 0
;
;IN: NONE
;OUT: NONE
;
;Not complete
;


__IntTimer:
				
	cli		
	PUSHAD
	
	inc dword [TickCount]
		
	
	call intTimerHandler	
	
	cmp eax, 0
	je TimerDone	;no task switch necessary		
		
	PUSH word 0	;send end of int before task switch
	call 0x50:0x00				
	;add esp, 4
	
	;eax is the pid to switch to.	
		
	push dword eax					
	call 0x78:0x00 ;task switch	
	add esp, 4
		
TimerDone:
	
	;send EOI

	PUSH word 0
	call 0x50:0x00			
	;add esp, 4

	
	POPAD		
	sti
	
	IRETD	

	
;=========================================================================
;__IntKeyboard - Far - Trap - IRQ 1
;
;IN: NONE
;OUT: NONE
;
;Not complete
;


kb dd 0


__IntKeyboard:
	
	PUSHAD
	
	cli
	
	XOR EAX, EAX
	in AL, 0x60 	;read byte
	

	;test stuff
	;mov eax, [kb]	
	;mov byte [es:0xb8002],'K'	
	;add eax, 0x2	
	;mov [kb], eax	
	;end test stuff	
	
	;send EOI
	
	;skip the first call to this handler.  Don't know what causes the INT, but it is not a keypress. Fix later.
	mov eax, [kb]
	cmp eax, 0
	je KBContinue
	
KBReset:	
	PUSH word 0
	call 0x50:0x00	
	
	POPAD

        call RestartSystem 
	
KBContinue:	

	inc eax
	mov [kb], eax
	
	PUSH word 0
	call 0x50:0x00	
	
	POPAD

	IRETD		
	
;=========================================================================
;__IntPICU2 - Far - Trap - IRQ 2
;
;IN: NONE
;OUT: NONE
;
;Not complete
;

__IntPICU2:

	PUSHAD
	
	;mov byte [es:0xB8004],'U'	

	PUSH word 0
	call 0x50:0x00
	
	POPAD

	IRETD						
	
	
;==========================================================================
;Init8259 - Near
;
;IN: NONE
;OUT: NONE
;
;Initializes the 8259 chips, I put in the push eax pop eax instructions because 
;I saw it in an example, but I dont know why they are needed. :<(
;

Init8259:

	push eax
	
	;ICW 1 master
	;level triggered, interval of 8, cascaded PIC's, need ICW 4
	mov al, 00010001b
	out PIC1_PORT1, al
	
	PUSH EAX
	POP EAX
	
	;ICW 1 slave
	;level triggered, interval of 8, cascaded PIC's, need ICW 4	
	out PIC2_PORT1, al
	
	PUSH EAX
	POP EAX	
	
	;ICW 2 - master		 
	mov al, 0x20			
	out PIC1_PORT2, al

	PUSH EAX
	POP EAX	
	
	;ICW 2 - slave
	
	MOV al,0x28
	out PIC2_PORT2, al
	
	PUSH EAX
	POP EAX		
	
	;ICW 3 - master
	;IR2 is connected to a Slave	
	mov al, 00000100b
	out PIC1_PORT2, al
	
	PUSH EAX
	POP EAX	
	
	;ICW 3 - slave	
	;SlaveID = Slave 2	
	mov al, 00000010b
	out PIC2_PORT2, al	
	
	PUSH EAX
	POP EAX	
	
	;ICW 4 - master Not Special Fully Nested Mode, Non - Buffered Mode, Auto EOI, 8086 mode
	mov al, 00000001b
	out PIC1_PORT2, al	
	
	PUSH EAX
	POP EAX	
		
	;ICW 4 - slave Not Special Fully Nested Mode, Non - Buffered Mode, Auto EOI, 8086 mode
	mov al, 00000001b
	out PIC2_PORT2, al	
	
	PUSH EAX
	POP EAX		
	
	;now mask all except cascade and the timer
	MOV AL,11111010b
	out PIC1_PORT2, al
	
	PUSH EAX
	POP EAX		
	
	;mask all on the slave pic
	MOV AL,11111111b	
	out PIC2_PORT2, al	
	PUSH EAX
	POP EAX	
	
	;I think we're done with the pic for now...for now!!!	
	
	pop eax

	retn	
	
;=============================================================================
;__EndOfIRQ - Far
;
;IN: NONE
;OUT: NONE
;
;sends an end of irq to the PIC's.  if irq is 0-7, sends it to pic1, if 8-15 send to pic
;2, then 1
;

__EndOfIRQ:		
	
	push EBP
	mov EBP, ESP
	PUSH EAX	
					
	mov eax, [EBP+0Ch]
	mov ah, al
	mov AL, 0x20							
	
	out PIC1_PORT1, al
	cmp AH, 7				;PICU1 only?
	JBE EOIDone			;Yes?
	OUT PIC2_PORT1, al       ;else Send to 2 also
EOIDone:				

	POP EAX       
	MOV ESP,EBP
	POP EBP			
	
	retf 2

;=============================================================================
;__MaskIRQ - Far
;
;IN: EBP is irq number
;OUT: NONE
;
;sends an end of irq to the PIC's.  if irq is 0-7, sends it to pic1, if 8-15 send to pic
;2, then 1
;

__MaskIRQ:
			
		PUSH EBP
		MOV EBP, ESP
		PUSHFD
		CLI						;Previous state is reset on POPFD
		PUSH EAX				;
		PUSH ECX
				
		MOV EAX, 1
		MOV ECX, [EBP+0Ch]		;Get IRQ number (0-15)
		AND ECX, 0Fh			;(0-15)
		SHL EAX, CL			;Set the bit for the IRQ (0-7) or (8-15)
		AND AL,AL
				
		JZ MaskIRQ2			
		MOV AH,AL
		IN AL, PIC1_PORT2
		PUSH EAX
		POP EAX
		OR AL, AH
		OUT PIC1_PORT2, AL
		JMP SHORT MaskIRQEnd
MaskIRQ2:	
		IN AL, PIC2_PORT2			;AH already has correct value
		PUSH EAX
		POP EAX
		OR AL, AH
		OUT PIC2_PORT2, AL
MaskIRQEnd:
		POP ECX                 
		POP EAX                 
		POPFD
		MOV ESP,EBP
		POP EBP 	
		
		retf 4
	
;=============================================================================
;__UnMaskIRQ - Far
;
;IN: ESP is irq number
;OUT: NONE
;
;sends an end of irq to the PIC's.  if irq is 0-7, sends it to pic1, if 8-15 send to pic
;2, then 1
;	

__UnMaskIRQ:
		
		PUSH EBP
		MOV EBP, ESP
		PUSHFD
		CLI						;Previous state is reset on POPFD
		PUSH EAX				;
		PUSH ECX
		MOV EAX,1
		MOV ECX,[EBP+0Ch]		;Get IRQ number (0-15)
		AND ECX, 0Fh			; (0-15 only)
		SHL EAX, CL				;Set the bit for the IRQ (0-7)
		AND AL,AL
		JZ UMIRQ2
		MOV AH, AL
		IN AL, PIC1_PORT2
		PUSH EAX
		POP EAX
		NOT AH
		AND AL, AH
		OUT PIC1_PORT2, AL
		JMP SHORT UMIRQEnd
UMIRQ2:
		IN AL, PIC2_PORT2
		PUSH EAX
		POP EAX
		NOT AH
		AND AL, AH
		OUT PIC2_PORT2, AL
UMIRQEnd:
		POP ECX                 
		POP EAX                 
		POPFD
		MOV ESP,EBP
		POP EBP
		RETF 4                
	
%endif
