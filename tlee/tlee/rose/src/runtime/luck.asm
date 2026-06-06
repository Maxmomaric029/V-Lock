.code

; Forward to kernel32 ReadProcessMemory/WriteProcessMemory
; Works on ALL Windows versions unlike hardcoded syscall numbers

EXTERN ReadProcessMemory: PROC
EXTERN WriteProcessMemory: PROC

Luck_ReadVirtualMemory PROC
    jmp ReadProcessMemory
Luck_ReadVirtualMemory ENDP

Luck_WriteVirtualMemory PROC
    jmp WriteProcessMemory
Luck_WriteVirtualMemory ENDP

END
