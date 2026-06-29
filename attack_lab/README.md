# Attack Lab

## Description

This file contains materials for one instance of the attacklab.

Files:

    ctarget

Linux binary with code-injection vulnerability.  To be used for phases
1-3 of the assignment.

    rtarget

Linux binary with return-oriented programming vulnerability.  To be
used for phases 4-5 of the assignment.

     cookie.txt

Text file containing 4-byte signature required for this lab instance.

     farm.c

Source code for gadget farm present in this instance of rtarget.  You
can compile (use flag -Og) and disassemble it to look for gadgets.

     hex2raw

Utility program to generate byte sequences.  See documentation in lab
handout.

## solution

solution files are in `levelx.txt` where x is the phase no

**phase3 - phase6 segfaults because of libc version mismatch. Run the exploit in gdb to see that the exploits calls the right functions.**

```bash
cat level1.txt | ./hex2raw | ./ctarget
cat level2.txt | ./hex2raw | ./ctarget
cat level3.txt | ./hex2raw | ./ctarget
cat level4.txt | ./hex2raw > raw1.bin
cat level5.txt | ./hex2raw > raw2.bin
cat level6.txt | ./hex2raw > raw3.bin

# in gdb
gdb ./rtarget
(gdb) set args -q < raw1.bin
(gdb) run
```
