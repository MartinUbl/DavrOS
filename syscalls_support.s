global syscall_int_handler

extern syscall_handler_internal

; int 80h direct handler
syscall_int_handler:
    align 4
    pusha                           ; save all registers
    push ds                         ; save original segment registers
    push es
    push fs
    push gs
    push ebx                        ; save original EBX value
    mov bx, 0x10                    ; store kernel data segment selector in BX
    mov ds, bx                      ; and move to data segment register
    pop ebx                         ; restore old EBX value

    push edi                        ; 6th parameter
    push esi                        ; 5th parameter
    push edx                        ; 4th parameter
    push ecx                        ; 3rd parameter
    push ebx                        ; 2nd parameter
    push eax                        ; 1st parameter

    call syscall_handler_internal   ; call C function

    pop ebx                         ; pop arguments ("cleanup stack")
    pop ebx
    pop ebx
    pop ebx
    pop ebx
    pop ebx

    pop gs                          ; restore original segment registers
    pop fs
    pop es
    pop ds
    popa                            ; restore original register set
    iret                            ; we're done
