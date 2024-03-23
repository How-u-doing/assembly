.section .rodata
msg:
    .ascii "Hello World\n"
len:
    .int . - msg

.section .text
.global _start
_start:
    # write(1, "Hello World\n", sizeof "Hello World\n" - 1)
    mov  $1, %rax           # syscall # for write
    mov  $1, %rdi           # file descriptor for stdout
    lea  msg(%rip), %rsi    # address of the message
    mov  len(%rip), %rdx    # length of the message
    syscall

    mov %rax, %rdi          # use the return code of write (# of bytes written)
                            # as the argument for _exit()
    # _exit(bytes_written)
    mov $60, %rax           # syscall # for _exit
    syscall
