;publics.inc
;Defines address for public calls

[BITS 32]

;align 8, db 0

_AddCallGate EQU 0x40:0x00
_AddIDTGate  EQU 0x48:0x00
_EndOfIRQ	   EQU 0x50:0x00
_MaskIRQ	   EQU 0x58:0x00
_UnMaskIRQ   EQU 0x60:0x00
_AllcatePage EQU 0x68:0x00
