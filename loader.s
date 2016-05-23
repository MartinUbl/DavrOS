global loader                       ; the entry symbol for ELF

    MAGIC_NUMBER equ 0x1BADB002     ; define the magic number constant
    FLAGS        equ 0x0            ; multiboot flags
    CHECKSUM     equ -MAGIC_NUMBER  ; calculate the checksum
                                    ; (magic number + checksum + flags should equal 0)

    ; paging-related values
    PAGE_DIR            equ 0x9C000 ; page directory table
    PAGE_TABLE_0        equ 0x9D000 ; 0th page table. Address must be 4KB aligned
    PAGE_TABLE_768      equ 0x9E000 ; 768th page table. Address must be 4KB aligned
    PAGE_TABLE_ENTRIES  equ 1024    ; each page table has 1024 entries
    PAGE_PRIV           equ 5       ; attributes (page is present; page is writable; supervisor mode)

    MB_PERSISTENT       equ 0x90000 ; address, where we store address of GRUB multiboot structure

    ; misc
    KERNEL_STACK_SIZE equ 4096      ; kernel stack size

    extern kernel_loader_med

    section .bss
    align 4
    kernel_stack:                   ; set up space for kernel stack
        resb KERNEL_STACK_SIZE      ; reserve KERNEL_STACK_SIZE bytes for stack

    section .text                   ; start of the text (code) section
    align 4                         ; the code must be 4 byte aligned
        dd MAGIC_NUMBER             ; write the magic number to the machine code,
        dd FLAGS                    ; the flags,
        dd CHECKSUM                 ; and the checksum

    loader:                         ; the loader label (defined as entry point in linker script)
        mov eax, MB_PERSISTENT      ; load address of MB_PERSISTENT to store original ebx value
        mov dword [eax], ebx        ; store ebx value for later reuse
        mov esp, kernel_stack + KERNEL_STACK_SIZE
        jmp near enable_paging      ; near jump to paging-enabling routine; any far jump would
                                    ; result into GPF or PF exceptions (therefore leading to higher
                                    ; level faults since we didn't initialize IDT yet)
    higher_half:
                                    ; initialize kernel stack by pointing esp to the end of
                                    ; kernel stack reserved space in BSS
        mov eax, MB_PERSISTENT      ; load MB_PERSISTENT address
        mov edx, dword [eax]        ; restore original multiboot structure address
        push edx                    ; pass it as argument for kernel entry
        call kernel_loader_med      ; call C function

    .endlessloop:
        jmp .endlessloop            ; loop forever

    enable_paging:
        pusha                       ; save frame

        ; idenitity map 1st page table (first 4MB)
        mov eax, PAGE_TABLE_0               ; first page table
        mov ebx, 0x0 | PAGE_PRIV            ; starting physical address of page
        mov ecx, PAGE_TABLE_ENTRIES         ; for every page in table...

    .loop:
        mov dword [eax], ebx                ; write the entry
        add eax, 4                          ; go to next page entry in table (Each entry is 4 bytes)
        add ebx, 4096                       ; go to next page address (Each page is 4Kb)
        loop .loop                          ; go to next entry

        ; set up the entries in the directory table
        mov eax, PAGE_TABLE_0 | PAGE_PRIV   ; 1st table is directory entry 0
        mov dword [PAGE_DIR], eax

        mov eax, PAGE_TABLE_768 | PAGE_PRIV ; 768th entry in directory table
        mov dword [PAGE_DIR+(768*4)], eax

        ; install page directory table
        mov eax, PAGE_DIR
        mov cr3, eax                        ; cr3 is page directory address register

        ; enable paging
        mov eax, cr0
        or eax, 0x80000000
        mov cr0, eax                        ; enable paging bit (0x80000000) in cr0 status register

        ; map the 768th table to physical addr 1MB
        ; the 768th table maps 3GB and higher of virtual memory
        mov eax, PAGE_TABLE_768             ; first page table
        mov ebx, 0x0 | PAGE_PRIV            ; starting physical address of page
        mov ecx, PAGE_TABLE_ENTRIES         ; for every page in table...
    .loop2:
        mov dword [eax], ebx                ; write the entry
        add eax, 4                          ; go to next page entry in table (Each entry is 4 bytes)
        add ebx, 4096                       ; go to next page address (Each page is 4Kb)
        loop .loop2                         ; go to next entry

        popa                        ; restore frame
        lea ebx, [higher_half]      ; prepare far jump to higher half
        jmp ebx                     ; wheeee!
