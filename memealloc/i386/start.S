/*
    This code is public domain and comes with no warranty.
    You are free to do whatever you want with it. You can
    contact me at lolisamurai@tfwno.gf but don't expect any
    support.
    I hope you will find the code useful or at least
    interesting to read. Have fun!
    -----------------------------------------------------------
    This file is part of "nolibc", a compilation of reusable
    code snippets I copypaste and tweak for my libc-free
    linux software.

    DISCLAIMER: these snippets might be incomplete, broken or
                just plain wrong, as many of them haven't had
                the chance to be heavily tested yet.
    -----------------------------------------------------------
    memealloc:
    A resizable and relocatable memory pool/allocator that uses
    relative pointers.
*/

#include "syscalls.h"

.intel_syntax noprefix
.text
    .globl _start, syscall
    .globl syscall1, syscall2, syscall3, syscall4, syscall5
    .globl syscall6

    _start:
        xor ebp,ebp
        pop esi
        mov ecx,esp
        and esp,-16
        push 0xb16b00b5
        push 0xb16b00b5
        push ecx
        push esi
        call main
        add esp,16
        mov ebx,eax
        mov eax,SYS_exit
        int 0x80
        ret

    syscall:
        mov eax,[esp+4]
        int 0x80
        ret

    syscall1:
        push ebx
        mov eax,[esp+4+4]
        mov ebx,[esp+8+4]
        int 0x80
        pop ebx
        ret

    syscall2:
        push ebx
        mov eax,[esp+4+4]
        mov ebx,[esp+8+4]
        mov ecx,[esp+12+4]
        int 0x80
        pop ebx
        ret

    syscall3:
        push ebx
        mov eax,[esp+4+4]
        mov ebx,[esp+8+4]
        mov ecx,[esp+12+4]
        mov edx,[esp+16+4]
        int 0x80
        pop ebx
        ret

    syscall4:
        push ebx
        push esi
        mov eax,[esp+4+8]
        mov ebx,[esp+8+8]
        mov ecx,[esp+12+8]
        mov edx,[esp+16+8]
        mov esi,[esp+20+8]
        int 0x80
        pop esi
        pop ebx
        ret

    syscall5:
        push ebx
        push esi
        push edi
        mov eax,[esp+4+12]
        mov ebx,[esp+8+12]
        mov ecx,[esp+12+12]
        mov edx,[esp+16+12]
        mov esi,[esp+20+12]
        mov edi,[esp+24+12]
        int 0x80
        pop edi
        pop esi
        pop ebx
        ret

    syscall6:
        push ebx
        push esi
        push edi
        push ebp
        mov eax,[esp+4+12]
        mov ebx,[esp+8+12]
        mov ecx,[esp+12+12]
        mov edx,[esp+16+12]
        mov esi,[esp+20+12]
        mov edi,[esp+24+12]
        mov ebp,[esp+28+12]
        int 0x80
        pop ebp
        pop edi
        pop esi
        pop ebx
        ret
