%ifndef __STATICDATA_H_
%define __STATICDATA_H_

;global _StaticpSysTSS 
;global _StaticpSysTD 
;global _StaticSchedulerData 


;did this in order to staticly allocate memory for system tss and td.
;_StaticpSysTSS times (512) db 0
;_StaticpSysTD times (512) db 0

;The scheduler data is only 154 bytes long, but by allocating 256 bytes
;I allow space for expansion of the structure.  This also aligns the structure
;on quarter kb boundry.  However, in reality this and the following two entries
;are not guarenteed to be aligned in any way.

;_StaticSchedulerData times (256) db 0

%endif