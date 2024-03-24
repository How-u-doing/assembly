# Register Renaming

See https://godbolt.org/z/8c8EK4ha3
```c++
void foo(int* m) {
    int r1 = m[1024];
    r1 = r1 + 2;
    m[1032] = r1;
    int r2 = m[2048];
    r2 = r2 + 4;
    m[2056] = r2;
}

void bar(int* m) {
    int r1 = m[1024];
    r1 = r1 + 2;
    m[1032] = r1;
    r1 = m[2048];
    r1 = r1 + 4;
    m[2056] = r1;
}
```

For `x86_64`, gcc `-O2` produces identical assembly:
```assembly
foo(int*):
        movl    4096(%rdi), %eax
        addl    $2, %eax
        movl    %eax, 4128(%rdi)
        movl    8192(%rdi), %eax
        addl    $4, %eax
        movl    %eax, 8224(%rdi)
        ret
bar(int*):
        movl    4096(%rdi), %eax
        addl    $2, %eax
        movl    %eax, 4128(%rdi)
        movl    8192(%rdi), %eax
        addl    $4, %eax
        movl    %eax, 8224(%rdi)
        ret
```
There's no register renaming job done by the compiler. Instead, it's done by the
powerful CPU hardware. After register renaming, the first 3 instructions and the
last 3 ones can be scheduled to different functional units to run concurrently.

For `ARM64`, gcc `-O1` produces assembly code that reuses the same register.
```assembly
foo(int*):
        ldr     w1, [x0, 4096]
        add     w1, w1, 2
        str     w1, [x0, 4128]
        ldr     w1, [x0, 8192]
        add     w1, w1, 4
        str     w1, [x0, 8224]
        ret
bar(int*):
        ldr     w1, [x0, 4096]
        add     w1, w1, 2
        str     w1, [x0, 4128]
        ldr     w1, [x0, 8192]
        add     w1, w1, 4
        str     w1, [x0, 8224]
        ret
```

In `-O2`, however, register renaming finally happens!
```assembly
foo(int*):
        ldr     w2, [x0, 4096]
        ldr     w1, [x0, 8192]
        add     w2, w2, 2
        str     w2, [x0, 4128]
        add     w1, w1, 4
        str     w1, [x0, 8224]
        ret
bar(int*):
        ldr     w2, [x0, 4096]
        ldr     w1, [x0, 8192]
        add     w2, w2, 2
        str     w2, [x0, 4128]
        add     w1, w1, 4
        str     w1, [x0, 8224]
        ret
```
Maybe because some ARM processors aren't equipped with register renaming
functionality. (Register renaming and OoO execution make the CPU so
power-hungry as [reservation stations](https://en.wikipedia.org/wiki/Reservation_station)
are busy doing their work ;)
