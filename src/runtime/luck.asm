.code

EXTERN ReadProcessMemory: PROC
EXTERN WriteProcessMemory: PROC

Luck_ReadVirtualMemory PROC
    jmp ReadProcessMemory
Luck_ReadVirtualMemory ENDP

Luck_WriteVirtualMemory PROC
    jmp WriteProcessMemory
Luck_WriteVirtualMemory ENDP

END
