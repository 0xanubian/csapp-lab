This repo contains all of my solutions for the labs exercises from the book CS:APP (computer systems a programmer's perspective).

## Lab progress

- [ ] Data Lab
- [x] Bomb Lab
- [x] Attack Lab
- [ ] Architecture Lab
- [x] Cache Lab
- [ ] Shell Lab
- [ ] Malloc Lab
- [ ] Proxy Lab

## Description of labs

- Data Lab: In this lab you implement simple logical, two's complement, and floating point functions, but using a highly restricted subset of C.


- [Bomb Lab](https://github.com/0xanubian/csapp-lab/tree/main/bomb_lab): In this lab you are given a binary with 6 different levels of checks that you are supposed to bypass in order to diffuse the bomb. You use a disassembler and a debugger to reverse engineer the logic and bypass the checks.


- [Attack Lab](https://github.com/0xanubian/csapp-lab/tree/main/attack_lab): In this lab you exploit 2 given binaries to hijack the control flow to exec arbitrary functions using stack overflow vulnerabilities and return oriented programming. The first binary's(ctarget) stack is executable so you can write your shellcode on the stack and jump to your stack. The second binary's(rtarget) doesn't has executable stack and has stack aslr enabled, so you use ROP to populate the right register with the right data and jump to the desired functions.


- [Cache Lab](https://github.com/0xanubian/csapp-lab/tree/main/cache_lab): In this lab you have to implement a cache simulator in part A and optimise a matrix transpose function in part B. In part A you receive `s`, `b` and `E` from cli args, you have to parse them and then make a cache system with these no of sets and these no of lines. The line replacement algorithm is LRU. Its a good exercise to learn about machine state, pointer manipulation, debugging memory errors, making a software from just specifications. Now for part B you implement 3 different functions for three different matrix size. you learn about blocking/tiling, diagonal conflict.
