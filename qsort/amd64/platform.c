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
    qsort: just a basic implementation of qsort
*/

#define AMD64
#include "syscalls.h"

typedef unsigned long int  u64;
typedef unsigned int       u32;
typedef unsigned short int u16;
typedef unsigned char      u8;

typedef long int    i64;
typedef int         i32;
typedef short int   i16;
typedef signed char i8;

typedef i64 intptr;
typedef u64 uintptr;

#include "../main.c"
