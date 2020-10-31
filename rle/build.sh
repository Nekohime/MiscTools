#!/usr/bin/env sh

test -f rle && rm rle
clang-10 rle.c -o rle -Wall -Wpedantic -Wextra -Wunused -std=c11 -Wl,--gc-sections
strip rle -R .comment -R .gnu.version

ls -l