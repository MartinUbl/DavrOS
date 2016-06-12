global switch_task

switch_task:

    ; do not let anyone interrupt us
    cli

    ; set ESI value to our exchange point
    mov esi, 0x2000

    ; set data segment register values
    mov ax, 0x20 | 0x03
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; restore part of registers
    mov ebx, [esi + 4]
    mov ecx, [esi + 8]
    mov edx, [esi + 12]
    mov edi, [esi + 20]
    mov ebp, [esi + 24]

    ; switch page table
    mov eax, [esi + 40]
    mov cr3, eax

    ; restore EAX value
    mov eax, [esi + 28]

    ; the stack is now properly mapped after CR3 exchange, so this is safe
    mov esp, [esi + 28]

    ; prepare stack contents for context switch
    push dword 0x20 | 0x03      ; SS
    push dword [esi + 28]       ; ESP
    pushf                       ; EFLAGS
    push dword 0x18 | 0x03      ; CS
    push dword [esi + 32]       ; EIP

    ; restore original value of ESI register using stack
    push dword [esi + 16]
    pop esi

    ; enable interrupts
    sti

    iret
