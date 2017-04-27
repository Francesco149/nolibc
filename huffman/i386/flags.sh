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
#   huffman: implementation of file compression using huffman
#            coding

export COMPILER_FLAGS="-m32 -Wno-long-long -DLEGACY_MMAP=1"
