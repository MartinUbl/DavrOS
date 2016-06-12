global outb
global inb
global io_wait
global halt
global get_kernel_physical_start
global get_kernel_physical_end
global cpuinfo_load_vendor
global load_gdt
global load_idt
global flush_tss
global int_disable
global int_enable
global flushtlb

extern kernel_physical_start
extern kernel_physical_end

; outb - send a byte to an I/O port
; stack: [esp + 8] data byte
;        [esp + 4] I/O port
;        [esp    ] return address
outb:
    mov al, [esp + 8]       ; move the data to be sent into the al register
    mov dx, [esp + 4]       ; move the address of the I/O port into the dx register
    out dx, al              ; send the data to the I/O port
    ret                     ; return to the calling function


; inb - returns a byte from the given I/O port
; stack: [esp + 4] address of the I/O port
;        [esp    ] return address
inb:
    mov dx, [esp + 4]       ; move the address of the I/O port to the dx register
    in  al, dx              ; read a byte from the I/O port and store it in the al register
    ret                     ; return the read byte


; io_wait - waits for input/output operation
; stack: [esp    ] return address
io_wait:
    mov dx, 0x80            ; use not used port 0x80
    mov ax, 0x00            ; send 0x00 there
    out dx, al              ; send data
    ret                     ; return back


; halt - halts machine
halt:
    hlt                     ; halt the CPU


get_kernel_physical_start:
    mov eax, kernel_physical_start
    ret

get_kernel_physical_end:
    mov eax, kernel_physical_end
    ret


; cpuinfo_load_vendor - loads vendor name to supplied address
; stack: [esp + 4] target address
;        [esp    ] return address
cpuinfo_load_vendor:
    mov eax, 0                  ; flag for "vendor string"
    cpuid                       ; call cpuinfo
    mov eax, [esp + 4]          ; move target pointer to eax
    mov dword [eax], ebx        ; store ebx part
    mov dword [eax + 4], edx    ; store ecx part
    mov dword [eax + 8], ecx    ; store edx part
    mov dword [eax + 12], 0x0   ; terminate with zero
    ret                         ; return back


; load_gdt - loads GDT table to memory using lgdt instruction
; stack: [esp + 4] GDTR record
;        [esp    ] return address
load_gdt:
    mov eax, [esp + 4]      ; move GDTR address to eax
    lgdt [eax]              ; load the address to internal segmentation register
    jmp 0x08:init_data_sp   ; perform far jump to "fix" CS (code segment register)
init_data_sp:
    ; 0x10 is the new data segment selector
    mov ax, 0x10            ; move segment selector to ax
    mov ds, ax              ; init DS
    mov es, ax              ; init ES
    mov fs, ax              ; init FS
    mov gs, ax              ; init GS
    mov ss, ax              ; init stack segment (stack is in data segment)
    ret                     ; return back


; load_idt - loads IDT table to memory using lidt instruction
; stack: [esp + 4] IDTR record
;        [esp    ] return address
load_idt:
    mov eax, [esp + 4]      ; move IDTR address to eax
    lidt [eax]              ; load the address
    ret                     ; return back


; flush_tss - flushes TSS segment
; stack: [esp + 4] segment to use
;        [esp    ] return address
flush_tss:
    mov eax, [esp + 4]      ; move IDTR address to eax
    ltr ax                  ; load the address
    ret                     ; return back


; int_disable - disable interrupts
int_disable:
    cli                     ; disable interrupts
    ret                     ; return back


; int_disable - enable interrupts
int_enable:
    sti                     ; enable interrupts
    ret                     ; return back

; flushtlb
flushtlb:
    mov eax, [esp + 4]
    invlpg [eax]
    ret
