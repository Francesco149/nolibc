#!/bin/sh

#   This code is public domain and comes with no warranty.
#   You are free to do whatever you want with it. You can
#   contact me at lolisamurai@tfwno.gf but don't expect any
#   support.
#   I hope you will find the code useful or at least
#   interesting to read. Have fun!
#   -----------------------------------------------------------
#   This file is part of "nolibc", a compilation of reusable
#   code snippets I copypaste and tweak for my libc-free
#   linux software.
#
#   DISCLAIMER: these snippets might be incomplete, broken or
#               just plain wrong, as many of them haven't had
#               the chance to be heavily tested yet.
#   -----------------------------------------------------------
#   qsort: just a basic implementation of qsort

exename="qsort"
archname=${1:-amd64} # if not specified, default to amd64
shift

if [ -e $archname/flags.sh ]; then
    source $archname/flags.sh
fi

gcc -std=c89 -pedantic -s -O2 -Wall -Werror \
    -nostdlib \
    -fno-unwind-tables \
    -fno-asynchronous-unwind-tables \
    -fdata-sections \
    -Wl,--gc-sections \
    -Wa,--noexecstack \
    -fno-builtin \
    -fno-stack-protector \
    $COMPILER_FLAGS \
    $@ \
    $archname/start.S $archname/platform.c \
    -o $exename \
\
&& strip -R .comment $exename
