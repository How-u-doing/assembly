# Hello World

```bash
$ as hello.s -o hello.o
$ objdump -dr hello.o

hello.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <_start>:
   0:	48 c7 c0 01 00 00 00 	mov    $0x1,%rax
   7:	48 c7 c7 01 00 00 00 	mov    $0x1,%rdi
   e:	48 8d 35 00 00 00 00 	lea    0x0(%rip),%rsi        # 15 <_start+0x15>
			11: R_X86_64_PC32	.rodata-0x4
  15:	48 8b 15 00 00 00 00 	mov    0x0(%rip),%rdx        # 1c <_start+0x1c>
			18: R_X86_64_PC32	.rodata+0x8
  1c:	0f 05                	syscall 
  1e:	48 89 c7             	mov    %rax,%rdi
  21:	48 c7 c0 3c 00 00 00 	mov    $0x3c,%rax
  28:	0f 05                	syscall 
$ ld hello.o -o hello
$ objdump -d hello

hello:     file format elf64-x86-64


Disassembly of section .text:

0000000000401000 <_start>:
  401000:	48 c7 c0 01 00 00 00 	mov    $0x1,%rax
  401007:	48 c7 c7 01 00 00 00 	mov    $0x1,%rdi
  40100e:	48 8d 35 eb 0f 00 00 	lea    0xfeb(%rip),%rsi        # 402000 <msg>
  401015:	48 8b 15 f0 0f 00 00 	mov    0xff0(%rip),%rdx        # 40200c <len>
  40101c:	0f 05                	syscall 
  40101e:	48 89 c7             	mov    %rax,%rdi
  401021:	48 c7 c0 3c 00 00 00 	mov    $0x3c,%rax
  401028:	0f 05                	syscall 
$ objdump --full-contents hello

hello:     file format elf64-x86-64

Contents of section .text:
 401000 48c7c001 00000048 c7c70100 0000488d  H......H......H.
 401010 35eb0f00 00488b15 f00f0000 0f054889  5....H........H.
 401020 c748c7c0 3c000000 0f05               .H..<.....      
Contents of section .rodata:
 402000 48656c6c 6f20576f 726c640a 0c000000  Hello World.....
$ ./hello
Hello World
$ echo $?
12
```
