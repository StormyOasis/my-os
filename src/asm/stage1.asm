;stage1.asm
;First stage bootloader

;stage1 boot loader
;purpose: finds the second stage of the boot loader on the disk then runs it

;bios puts us at 0x7c00
[BITS 16]
;[ORG 0x7c00]
;Hmmmm....It seems later computers(pentiums and up) don't like
;the [ORG] command.  We need to manually set ds to 0x7c0,
;but it works with Bochs
[ORG 0x7c00]
loader_entry_point:
;    MOV AX,0x7c00
;	mov ds, ax

	JMP Start

version_string	DB 'V0.01', 13, 10, 0
bBootDrive		DB 0x00
loading:			DB 'Loading Second Stage...', 0
done_str			DB 'Done!', 13,10,0
WelcomeMsg:	DB 'Welcome to the OS!', 13, 10, 0
CheckMsg:      	DB 'Checking for 386+ cpu...', 0
FoundMsg:      	db 'Found!', 13,10,0
NotFoundMsg:	DB 'No 386+ cpu found', 13,10,0
ErrorMsg:			DB 'Press any key to reboot', 0

Reboot:
     MOV SI, ErrorMsg
     call PutChars
     call GetKey

     db 0EAh
     dw 0000h
     dw 0FFFFh

PutChars:
    	LODSB
    	OR   AL,AL
    	JZ   SHORT Done
    	MOV  AH, 0Eh
    	MOV  BX,0007
    	INT  10h
    	JMP  SHORT PutChars
Done:
	RETN

CheckCPU:
	MOV SI, CheckMsg
	Call PutChars

	pushf

	xor ah, ah
	push ax
	popf

	pushf
	pop ax
	and ah, 0f0h
	cmp ah, 0f0h

	je no386

	mov ah, 0f0h
	push ax
	popf

	pushf
	pop ax
	and ah, 0f0h
	jz no386
	popf

	mov SI, FoundMsg
	CALL PutChars

	retn
no386:
	mov SI, NotFoundMsg
	CALL PutChars
	call GetKey
	jmp Reboot

GetKey:
     mov ah, 0
     int 016h
     retn

;code
Start:
	; Setup the stack.
	; We can't allow interrupts while we setup the stack
	cli						; Disable interrupts
	mov AX, 0x9000 	; Put stack at 0x90000
	mov SS, AX			; SS = 9000
	mov SP, 0xfbff		;use all of the segment


	MOV [bBootDrive], DL


	MOV SI, WelcomeMsg
	Call PutChars
	MOV SI, version_string
	Call PutChars

	CALL CheckCPU

.386P

	MOV SI, loading
	CALL PutChars

LoadNextStage:
	sti

	;Reset the disk controller
    XOR AX, AX
	MOV DL, [bBootDrive]
    int 0x13
	JC Reboot       					; error

	MOV AX, 0x50					; segment
	MOV ES, AX
	MOV BX, 0						; offset
    MOV CX, 0x0002				; cylinder = 0, sector = 2 (1, not 0, based)
    MOV DH, 0                		; head = 0
    MOV DL, [bBootDrive]    	; disk booted from
	MOV AH, 0x02					; operation
	MOV AL, 0x02					; # sectors to read
   	int 0x13
	JC LoadNextStage       		;It sometimes takes a few tries to work.
											;NOTE: If the disk is bad, this will result in an
											;infinite loop

	;park the floppy motor
	MOV DX, 0x3F2
	XOR AL, AL
	OUT DX, AL

	MOV SI, done_str
	CALL PutChars

	cli
	mov DL, [bBootDrive]
	jmp 0x500

times 512-($-$$)-2 db 0
	dw 0AA55h
