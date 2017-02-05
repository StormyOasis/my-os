;timer.asm
;timer related functions

[BITS 32]
[SECTION .text]

%ifndef __TIMER_ASM
%define __TIMER_ASM

;=======================================================================
;SetupTimer - Near
;
;IN: NONE
;OUT: NONE
;
;Just sets up the clock tick counter for 10ms.
;

SetupTimer:

	;sets the time counter divisor to 11932. 10 ms.
	;frequency is 1.191382 Mhz / 11932 = 100 per sec.(10ms)
	
	mov al, 0x9C
	out 0x40, al
	
	mov al, 0x02e
	out 0x40, al		

	retn

%endif
