;stage2.asm
;Second stage bootloader

;placed here after the first stage

;The sector reading code was stolen from somewhere, but I can't remember where

[BITS 16]

[ORG 0x500]

start:
.386P

	mov [bBootDrive], DL

	call EnableA20	
	
	sti
	
	;assuming sucess
	MOV SI, A20msg
	CALL PutChars	
	
	call get_conv_mem_amount
	call get_ext_mem_amount
	mov [ext_mem + 0],ax
	mov [ext_mem + 2],dx	
	
	MOV SI, KLoadingMsg
	CALL PutChars
	
    mov cx,0x100        ; <- Number of sectors (Increase as the kernel size increases)
    mov ax,0x3        ; <- Input location start
    mov bx,0x1000   ; <- Output location es:bx
	mov es,bx
	xor bx,bx
    call ReadSectors      
	
	MOV SI, done_msg
	CALL PutChars	
	
	;park the floppy motor
	MOV DX, 0x3F2
	XOR AL, AL
	OUT DX, AL	
			 	
	MOV SI, Pmode
	CALL PutChars
	
	cli	
		
o32	lgdt [gdtr]	
	
	mov eax,cr0
	or al,1	
	mov cr0,eax
	jmp prefetch_clear                 ; clear the prefetch queue
	nop
	nop
prefetch_clear:	

	jmp 0x8:pm
	
	hlt ;should never hit this
	
[BITS 32]

pm:

	;store memory amounts
	mov ebx, [con_mem]
	mov ecx, [ext_mem]

	jmp 0x08:0x10000
		
[BITS 16]
	
GetKey:
	 sti
     mov ah, 0
     int 016h
     retn	
	
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
	
EnableA20:
;Enable the a20 address line.  There are many different ways to try to do this.
	CLI
	XOR CX, CX
IBEmm0:
	IN AL, 0x64
	TEST AL, 0x02
	LOOPNZ IBEmm0
	MOV AL, 0x0D1
	OUT 0x64, AL
	XOR CX, CX
IBEmm1:
	IN AL, 0x64
	TEST AL, 0x02
	LOOPNZ IBEmm1
	MOV AL, 0x0DF
	OUT 0x60, AL
	XOR CX, CX
IBEmm2:
	IN AL, 0x64
	TEST AL, 0x02
	LOOPNZ IBEmm2

    in al, 0x92
    or al, 2
    out 0x92, al

	retn	
	
		
; Procedure to change LBA address system into CHS address system
;  Description: Gets absolute values from logical values. (Logical = Consecutive sectors; Absolute = Sector:Track:Head)
;  Calculations done:
;   absolute sector = (LBA mod SPT) + 1
;   absolute head = (LBA / SPT) MOD Heads
;   absolute cylinder = (LBA / SPT) / Heads
;  Input: AX - LBA value (And the FAT table of course)
;  Output: AX - Sector; BX - Head; CX - Cylinder

LBAtoCHS:
	PUSH dx			; Save the value in dx
	XOR dx,dx			; Zero dx
	MOV bx, [SectorsPerTrack]	; Move into place STP (LBA all ready in place)
 	DIV bx				; Make the divide (ax/bx -> ax,dx)
 	inc dx				; Add one to the remainder (sector value)
 	push dx			; Save the sector value on the stack

 	XOR dx,dx			; Zero dx
 	MOV bx, [NumHeads]		; Move NumHeads into place (NumTracks all ready in place)
 	DIV bx				; Make the divide (ax/bx -> ax,dx)

 	MOV cx,ax			; Move ax to cx (Cylinder)
 	MOV bx,dx			; Move dx to bx (Head)
 	POP ax				; Take the last value entered on the stack off.
 	POP dx				; Restore dx, in case something was there to start with
 	RETN				; Return to the main function
	
; Procedure ReadSectors - Reads sectors from the disk.
;  Input: cx - Number of sectors; ax - Start position
;  Output: Loaded file into: es:bx (bx initial value must be set)

ReadSectors:
.MAIN:                          ; Main Label
        mov di, 5               ; Loop 5 times max!!!
.SECTORLOOP:
        push ax                 ; Save register values on the stack
        push bx
        push cx
        push bx
	; AX must have the LBA sector value. (Correct)
		call LBAtoCHS             ; Change the LBA addressing to CHS addressing
	; AX=Sector, CX=Cylinder, BX=Head
        ; The code to read a sector from the floppy drive
        mov ch, cl                      ; Cylinder to read
        mov cl, al                      ; Sector to read
        mov dh, bl                      ; Head to read
        mov dl, BYTE [bBootDrive]          ; Drive to read (Could be setup before loop to optimise)
        mov ah, 02              	; BIOS read sector function
        mov al, 01              	; read one sector
        pop bx
        int 0x13                	; Make the BIOS call
	; Loop round stuff
        jnc .SUCCESS
        dec di                  ; Decrease the error counter
        pop cx                  ; Restore the register values
        pop bx
        pop ax
        jnz .SECTORLOOP         ; Try the command again incase the floppy drive is being annoying
        call ReadError          ; Call the error command in case all else fails
.SUCCESS
     	MOV SI, dot
     	call PutChars

        pop cx                  ; Restore the register values
        pop bx
        pop ax
        add bx, WORD 512   ; Queue next buffer (Adjust output location so as to not over write the same area again with the next set of data)
        inc ax                          ; Queue next sector (Start at the next sector along from last time)
        ; I think I may add a status bar thing also. A # for each sector loaded or something.
        ; Shouldn't a test for CX go in here???
        dec cx                          ; One less sector left to read
        jz .ENDREAD                     ; Jump to the end of the precedure
        loop .MAIN                      ; Read next sector (Back to the start)
.ENDREAD:                       ; End of the read procedure
        ret                     ; Return to main program
		
ReadError:
	call Reboot
	
;===================================
; get_conv_mem_amount
; Find the amount of conventional memory on the system
;IN: NONE
;OUT: NONE
;

get_conv_mem_amount:
	int 0x12	
	mov [con_mem], ax
	ret	

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:			get_ext_mem_amount
; action:		gets extended memory size using BIOS calls
; in:			(nothing)
; out:			32-bit extended memory size in DX:AX
; modifies:		DX, AX
; minimum CPU:		8088 (386+ for INT 15h AX=E820h)
;"Borrowed" from somewhere
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

get_ext_mem_amount:
	push es
	push di
	push cx
	push bx

; try INT 15h AX=E820
		push ds
		pop es
		mov di,buffer_e820
		xor esi,esi
		mov edx,534D4150h	; "SMAP"
		xor ebx,ebx
mem1:
		mov ecx,20h
		mov eax,0000E820h
		int 15h
		jc mem3
		cmp dword [es:di + 16],1 ; type 1 memory (available to OS)
		jne mem2
		cmp dword [es:di],100000h ; check only extended memory
		jb mem2
		add esi,[es:di + 8]
mem2:
		or ebx,ebx
		jne mem1

		mov eax,esi
		mov edx,esi
		shr edx,16
		jmp short mem7		; DX:AX=RAM top
mem3:
; try INT 15h AX=E801h
		mov ax,0E801h
		int 15h
		jc mem4
		mov dx,bx		; BX=ext mem >=16M, in 64K
					; move to DX multiples by 64K
		xor bx,bx
		mov bl,ah		; AX=ext mem 1M-16M, in K
		mov ah,al		; multiply by 256
		xor al,al


		add dx,bx
		jmp short mem7		; DX:AX=RAM top
mem4:
; try INT 15h AH=88h
		mov ax,8855h
		int 15h			; CY=error is not reliable...
		cmp ax,8855h		; ...so make sure AL is modified
		jne mem5
		mov ax,88AAh
		int 15h			; AX=ext mem size, in K
		cmp ax,88AAh
		je mem6
mem5:
		xor dx,dx		; move to DX multiplies by 256
		mov dl,ah
		mov ah,al
		xor al,al

		jmp short mem7		; DX:AX=RAM top
mem6:
		xor ax,ax
		xor dx,dx
mem7:
	pop bx
	pop cx
	pop di
	pop es
	ret


buffer_e820: times	128 db 0
con_mem			dd 0
ext_mem			dd 0
bBootDrive		DB 0x00
ErrorMsg			DB 'Press any key to reboot', 0
A20msg			db 'A20 address line enabled', 13,10,0
Pmode  			DB 'Setting CR0->PMode Bit (Entering Protected Mode)', 13,10,0
KLoadingMsg 	DB 13,10, 'Loading Kernel',0
mem_top			dd 0
dot					db '.',0
done_msg			DB 13,10,'Done!', 13,10,0

SectorsPerTrack         dw      18              ; Sectors per track
NumHeads                dw      2               ; Number of heads (2 as double sided floppy)

gdtr
	dw gdt_end-gdt-1; length of gdt 
	dd gdt	    ; linear, physical address of gdt	

align 8, db 0
gdt
nullsel equ $-gdt ; 0h = 000b
gdt0    ; null descriptor
	dd 0 		
	dd 0
codesel equ  $-gdt ; = 8h = 1000b 
code_gdt    ; code descriptor 4 GB flat segment starting 0000:0000h 
	dw 0ffffh
	dw 0h 
	db 0h
	db 09ah
	db 0cfh
	db 0h
datasel equ $-gdt ;=10h  = 10000b 
data_gdt 	; data descriptor 4 GB flat segment starting 0000:0000h
	dw 0ffffh		; limit 4 GB
	dw 0h			; Base 0000:0000h
	db 0h			
	db 092h			
	db 0cfh
	db 0h
videosel	equ	$-gdt ;= 18h = 11000b
	dw 3999			; limit 80*25*2-1
	dw 0x8000		; base 0xB8000
	db 0x0B
	db 0x92			; present, ring 0, data, expand-up, writable
	db 0			; byte-granular, 16-bit
	db 0

gdt_end


times 1024-($-$$) db 0
