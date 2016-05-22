global handle_keyboard_irq
global handle_pit_irq
global handle_floppy_irq

extern __keyboard_irq_handler
extern __pit_irq_handler
extern __floppy_irq_handler

handle_keyboard_irq:
    align 4
    pusha
    cld
    call __keyboard_irq_handler
    popa
    iret

handle_pit_irq:
    align 4
    pusha
    cld
    call __pit_irq_handler
    popa
    iret

handle_floppy_irq:
    align 4
    pusha
    cld
    call __floppy_irq_handler
    popa
    iret
