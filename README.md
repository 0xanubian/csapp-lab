This repo contains all of my solutions for the labs exercises from the book CS:APP (computer systems a programmer's perspective).

## Lab progress

- [ ] Data Lab
- [x] Bomb Lab
- [x] Attack Lab
- [ ] Architecture Lab
- [ ] Cache Lab
- [ ] Shell Lab
- [ ] Malloc Lab
- [ ] Proxy Lab

## Description of labs

- Data Lab: In this lab you implement simple logical, two's complement, and floating point functions, but using a highly restricted subset of C.
- [Bomb Lab](https://github.com/0xanubian/csapp-lab/tree/main/bomb_lab): In this lab you are given a binary with 6 different levels of checks that you are supposed to bypass in order to diffuse the bomb. You use a disassembler and a debugger to reverse engineer the logic and bypass the checks.
- [Attack Lab](https://github.com/0xanubian/csapp-lab/tree/main/attack_lab): In this lab you exploit 2 given binaries to hijack the control flow to exec arbitrary functions using stack overflow vulnerabilities and return oriented programming. The first binary's(ctarget) stack is executable so you can write your shellcode on the stack and jump to your stack. The second binary's(rtarget) doesn't has executable stack and has stack aslr enabled, so you use ROP to populate the right register with the right data and jump to the desired functions.
