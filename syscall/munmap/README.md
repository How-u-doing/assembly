## munmap() Crash 👻

In March, I was debugging a mysterious crash involing shared memory and the `munmap()` syscall. The crash happened right after `munmap()` was called. I was shocked to find out that the kernel actually returned `0` (success) for the `munmap()` syscall when an "invalid" length was passed.

It was an inconsistency bug: the file was `mmap()`ed with 16GB for reading, but `munmap()`ed with 24GB read from the obsolete file header. I also hand-coded the `munmap()` syscall in assembly to bypass the glibc wrapper, same result.

The `munmap(2)` [manual](https://man7.org/linux/man-pages/man2/munmap.2.html) states:

> The address addr must be a multiple of the page size (but length
need not be).  **All pages containing a part of the indicated range
are unmapped, and subsequent references to these pages will
generate SIGSEGV.  It is not an error if the indicated range does
not contain any mapped pages**.

That is to say, any valid virtual memory areas in this range $VMA_{i} \in [P, P+L)$ will be unmapped.
```
    P                                                            P+L
----+-------------+-----+-------------+-----+-------------+------+-----
    |             |     |             |     |             |
    +----VMA_i----+     +----VMA_j----+     +----VMA_k----+
```

### Reproducible Example

```cpp
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cstdio>

int syscall_munmap(void *addr, size_t length) {
    long ret;

    asm volatile("syscall"
                 : "=a"(ret)
                 : "a"(11),     // SYS_munmap
                   "D"(addr),   // rdi
                   "S"(length)  // rsi
                 : "rcx", "r11", "memory");

    if (ret < 0) {
        errno = -ret;
        printf("syscall_munmap failed, addr:%p, length:%zu, ret:%ld\n", addr, length, ret);
        return -1;
    }

    return (int)ret;
}

void do_unmap(void *addr, size_t length) {
    if (munmap(addr, length) < 0) {
        printf("munmap failed: %s\n", strerror(errno));
    } else {
        printf("Successfully unmapped memory at: %p\n", addr);
    }
}

void *do_mmap(int fd, size_t length) {
    void *addr = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        printf("mmap failed: %s\n", strerror(errno));
        return addr;
    }
    printf("Mapped memory at: %p\n", addr);
    return addr;
}

void print_mappings() {
    int pid = getpid();
    char cmd[256];
    snprintf(cmd, sizeof cmd, "cat /proc/%d/maps", pid);
    system(cmd);
}

int main() {
    const size_t map_size = 1ULL * 1024 * 1024;
    const size_t unmap_size = 1024ULL * 1024 * 1024;

    int fd = open("/bin/ls", O_RDONLY);

    if (fd < 0) {
        printf("Failed to open file: %s\n", strerror(errno));
        return 1;
    }

    void *addr = do_mmap(fd, map_size);

    print_mappings();

    // do_unmap(addr, unmap_size);
    syscall_munmap(addr, unmap_size);

    print_mappings();

    return 0;
}
```

```bash
⋊> ~ g++ munmap.cc
⋊> ~ ./a.out
Mapped memory at: 0x789eb9f00000
6543562fd000-6543562fe000 r--p 00000000 fd:03 261826                     /home/mark/a.out
6543562fe000-6543562ff000 r-xp 00001000 fd:03 261826                     /home/mark/a.out
6543562ff000-654356300000 r--p 00002000 fd:03 261826                     /home/mark/a.out
654356300000-654356301000 r--p 00002000 fd:03 261826                     /home/mark/a.out
654356301000-654356302000 rw-p 00003000 fd:03 261826                     /home/mark/a.out
654375c85000-654375ca6000 rw-p 00000000 00:00 0                          [heap]
789eb9f00000-789eba000000 r--p 00000000 fd:03 1058201                    /usr/bin/ls
789eba000000-789eba028000 r--p 00000000 fd:03 1077453                    /usr/lib/x86_64-linux-gnu/libc.so.6
789eba028000-789eba1b1000 r-xp 00028000 fd:03 1077453                    /usr/lib/x86_64-linux-gnu/libc.so.6
789eba1b1000-789eba200000 r--p 001b1000 fd:03 1077453                    /usr/lib/x86_64-linux-gnu/libc.so.6
789eba200000-789eba204000 r--p 001ff000 fd:03 1077453                    /usr/lib/x86_64-linux-gnu/libc.so.6
789eba204000-789eba206000 rw-p 00203000 fd:03 1077453                    /usr/lib/x86_64-linux-gnu/libc.so.6
789eba206000-789eba213000 rw-p 00000000 00:00 0 
789eba26b000-789eba26e000 rw-p 00000000 00:00 0 
789eba279000-789eba27b000 rw-p 00000000 00:00 0 
789eba27b000-789eba27c000 r--p 00000000 fd:03 1077450                    /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
789eba27c000-789eba2a7000 r-xp 00001000 fd:03 1077450                    /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
789eba2a7000-789eba2b1000 r--p 0002c000 fd:03 1077450                    /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
789eba2b1000-789eba2b3000 r--p 00036000 fd:03 1077450                    /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
789eba2b3000-789eba2b5000 rw-p 00038000 fd:03 1077450                    /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7fff865ec000-7fff8660d000 rw-p 00000000 00:00 0                          [stack]
7fff8660d000-7fff86611000 r--p 00000000 00:00 0                          [vvar]
7fff86611000-7fff86613000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
fish: Job 1, './a.out' terminated by signal SIGSEGV (Address boundary error)
```

In the output, we can see that range R `[0x789eb9f00000, 0x789ef9f00000)` would be unmapped by the kernel, including the innocent virtual memory objects `libc.so.6 [0x789eba000000, 0x789eba206000)` and `ld-linux-x86-64.so.2 [0x789eba27b000, 0x789eba2b5000)`.

Let's examine the core dump with `gdb` to see what happened:
```bash
⋊> ~ gdb ./a.out core.a.out.3442637.1773676675 
GNU gdb (Ubuntu 15.0.50.20240403-0ubuntu1) 15.0.50.20240403-git
...
Reading symbols from a.out...
(No debugging symbols found in a.out)
[New LWP 809401]
Cannot access memory at address 0x789eba2b4130
Cannot access memory at address 0x789eba2b4128
Cannot access memory at address 0x789eba2b4128
Core was generated by `./a.out'.
Program terminated with signal SIGSEGV, Segmentation fault.
#0  0x00006543562fe290 in syscall_munmap(void*, unsigned long) ()
(gdb) bt
#0  0x00006543562fe290 in syscall_munmap(void*, unsigned long) ()
#1  0x00006543562fe4ee in main ()
(gdb) info proc mappings 
Mapped address spaces:

          Start Addr           End Addr       Size     Offset objfile
      0x6543562fd000     0x6543562fe000     0x1000        0x0 /home/mark/a.out
      0x6543562fe000     0x6543562ff000     0x1000     0x1000 /home/mark/a.out
      0x6543562ff000     0x654356300000     0x1000     0x2000 /home/mark/a.out
      0x654356300000     0x654356301000     0x1000     0x2000 /home/mark/a.out
      0x654356301000     0x654356302000     0x1000     0x3000 /home/mark/a.out
(gdb) disassemble 
Dump of assembler code for function _Z14syscall_munmapPvm:
   0x00006543562fe269 <+0>:     endbr64
   0x00006543562fe26d <+4>:     push   %rbp
   0x00006543562fe26e <+5>:     mov    %rsp,%rbp
   0x00006543562fe271 <+8>:     push   %rbx
   0x00006543562fe272 <+9>:     sub    $0x28,%rsp
   0x00006543562fe276 <+13>:	mov    %rdi,-0x28(%rbp)
   0x00006543562fe27a <+17>:	mov    %rsi,-0x30(%rbp)
   0x00006543562fe27e <+21>:	mov    $0xb,%eax
   0x00006543562fe283 <+26>:	mov    -0x28(%rbp),%rdx
   0x00006543562fe287 <+30>:	mov    -0x30(%rbp),%rsi
   0x00006543562fe28b <+34>:	mov    %rdx,%rdi
   0x00006543562fe28e <+37>:	syscall
=> 0x00006543562fe290 <+39>:	mov    %rax,-0x18(%rbp)
   0x00006543562fe294 <+43>:	cmpq   $0x0,-0x18(%rbp)
   0x00006543562fe299 <+48>:	jns    0x6543562fe2d6 <_Z14syscall_munmapPvm+109>
   0x00006543562fe29b <+50>:	mov    -0x18(%rbp),%rax
   0x00006543562fe29f <+54>:	neg    %eax
   0x00006543562fe2a1 <+56>:	mov    %eax,%ebx
   0x00006543562fe2a3 <+58>:	call   0x6543562fe0e0 <__errno_location@plt>
   0x00006543562fe2a8 <+63>:	mov    %ebx,%edx
   0x00006543562fe2aa <+65>:	mov    %edx,(%rax)
   0x00006543562fe2ac <+67>:	mov    -0x18(%rbp),%rcx
   0x00006543562fe2b0 <+71>:	mov    -0x30(%rbp),%rdx
   0x00006543562fe2b4 <+75>:	mov    -0x28(%rbp),%rax
   0x00006543562fe2b8 <+79>:	mov    %rax,%rsi
   0x00006543562fe2bb <+82>:	lea    0xd46(%rip),%rax        # 0x6543562ff008
   0x00006543562fe2c2 <+89>:	mov    %rax,%rdi
   0x00006543562fe2c5 <+92>:	mov    $0x0,%eax
   0x00006543562fe2ca <+97>:	call   0x6543562fe130 <printf@plt>
   0x00006543562fe2cf <+102>:	mov    $0xffffffff,%eax
   0x00006543562fe2d4 <+107>:	jmp    0x6543562fe2da <_Z14syscall_munmapPvm+113>
   0x00006543562fe2d6 <+109>:	mov    -0x18(%rbp),%rax
   0x00006543562fe2da <+113>:	mov    -0x8(%rbp),%rbx
   0x00006543562fe2de <+117>:	leave
   0x00006543562fe2df <+118>:	ret
End of assembler dump.
(gdb) p $rax
$1 = 0
(gdb) p $rbp-0x18
$2 = (void *) 0x7fff8660c968
(gdb)
```

It appears that the process crashed upon executing a store to `-0x18(%rbp)` (`0x7fff8660c968`), which is in the stack `[0x7fff865ec000, 0x7fff8660d000)`, which does **not** intersect the unmapped range. We can also see that the `SYS_munmap` returned `0` (success) in `$rax`, and gdb emits warnings about not being able to access memory at `0x789eba2b4128` and `0x789eba2b4130`, which are inside the unmapped `ld.so`.

What's even interesting is that this `munmap()` would fail when running inside `gdb` instead of crashing. So, `munmap()` behaves somewhat differently under ptrace mode.
