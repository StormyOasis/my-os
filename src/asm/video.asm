;video.asm
;contains functions pertaining for display

%ifndef _VIDEO_ASM
%define _VIDEO_ASM

[BITS 32]
[SECTION .text]

;data
CRTCAddHi		EQU		0x0C
CRTCAddLo		EQU		0x0D
CRTCPort1		EQU		0x03d4
CRTCPort2		EQU		0x03d5
CRTC0C		DB		0
CRTC0D		DB		0
;code
;=====================================================================
;InitVideo - near
;
;IN: NONE
;OUT: NONE
;

InitVideo:

	MOV AL, CRTCAddHi
	MOV DX, CRTCPort1
	OUT DX, AL
	MOV AL, [CRTC0C]
	MOV DX, CRTCPort2
	OUT DX, AL
	
	MOV AL, CRTCAddLo
	MOV DX, CRTCPort1
	OUT DX, AL
	MOV AL, [CRTC0D]
	MOV DX, CRTCPort2
	OUT DX, AL

	retn

%endif
