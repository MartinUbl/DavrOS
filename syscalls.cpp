#include "syscalls.h"
#include "console.h"
#include "stdint.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"

#define SYSCALL_PARAMS uint32_t ebx, uint32_t ecx, uint32_t edx, uint32_t esi, uint32_t edi
#define SYSCALL_PARAMS_BLANK uint32_t, uint32_t, uint32_t, uint32_t, uint32_t
#define EXISTING_SYSCALLS 25

static int __syscall_empty(SYSCALL_PARAMS_BLANK)
{
    // no syscall defined for this eax value

    return 0;
}

static int __syscall_write(SYSCALL_PARAMS)
{
    const char* outbuffer = (const char*)ecx;

    // TODO: validation?

    // stdout
    if (ebx == 0)
        Console::WriteN(outbuffer, edx);

    return 0;
}

static int (*syscall_table[EXISTING_SYSCALLS])(SYSCALL_PARAMS) = {
    &__syscall_empty,                   // 0 = EMPTY
    &__syscall_empty,                   // 1 = exit
    &__syscall_empty,                   // 2 = fork
    &__syscall_empty,                   // 3 = read
    &__syscall_write,                   // 4 = write
    &__syscall_empty,                   // 5 = open
    &__syscall_empty,                   // 6 = close
    &__syscall_empty,                   // 7 = waitpid
    &__syscall_empty,                   // 8 = creat
    &__syscall_empty,                   // 9 = link
    &__syscall_empty,                   // 10 = unlink
    &__syscall_empty,                   // 11 = execve
    &__syscall_empty,                   // 12 = chdir
    &__syscall_empty,                   // 13 = time
    &__syscall_empty,                   // 14 = mknod
    &__syscall_empty,                   // 15 = chmod
    &__syscall_empty,                   // 16 = lchown
    &__syscall_empty,                   // 17 = EMPTY
    &__syscall_empty,                   // 18 = stat
    &__syscall_empty,                   // 19 = lseek
    &__syscall_empty,                   // 20 = getpid
    &__syscall_empty,                   // 21 = mount
    &__syscall_empty,                   // 22 = oldumount
    &__syscall_empty,                   // 23 = setuid
    &__syscall_empty,                   // 24 = getuid
    // when adding more syscall entries, do not forget to increase EXISTING_SYSCALLS value
};

extern "C" int syscall_handler_internal(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx, uint32_t esi, uint32_t edi)
{
    if (eax < EXISTING_SYSCALLS)
        return (syscall_table[eax])(ebx, ecx, edx, esi, edi);

    return 0;
}
