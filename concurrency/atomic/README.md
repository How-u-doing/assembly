# Atomic Operations
## compare_exchange_weak
From [cppreference](https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange):
```c++
bool compare_exchange_weak( T& expected, T desired,
                            std::memory_order order =
                                std::memory_order_seq_cst ) noexcept;
```
> Atomically compares the value representation (since C++20) of `*this` with that of `expected`. If those are bitwise-equal, replaces the former with `desired` (performs read-modify-write operation). Otherwise, loads the actual value stored in `*this` into `expected` (performs load operation).
Returns `true` if the underlying atomic value was successfully changed, `false` otherwise.

```c++
#include <atomic>

void add7_weak(std::atomic<int> &x) {
    int expected = x;
    while (!x.compare_exchange_weak(expected, expected + 7)) {
        // do nothing
    }
}
```
x86-64 gcc 13.2 with `-O2` optimizations generates the following assembly code:
```assembly
add7_weak(std::atomic<int>&):
        movl    (%rdi), %eax
.L8:
        leal    7(%rax), %edx
        lock cmpxchgl   %edx, (%rdi)
        jne     .L8
        ret
```
From the generated assembly code, we can see that reference `x` is treated as a pointer to
a `std::atomic<int>` object stored in `%rdi`, the `expected` value is in `%eax`, and the
`desired` value is in `%edx`. The `lock cmpxchg` instruction does the job that
`compare_exchange_weak` is supposed to do. But the `lock cmpxchg` instruction has only
two operands, the source operand `%edx` being the `desired` value, and the destination operand
`(%rdi)` being the memory location (`x`) to be updated. So, where is the `expected` value?
In `%eax`, I've told you. It's implicitly used by the `lock cmpxchg` instruction, otherwise
`%eax` is unused. We can write `expected` in the loop body and see how the assembly code changes:
```c++
void add7_weak(std::atomic<int> &x) {
    int expected = x;
    while (!x.compare_exchange_weak(expected, expected + 7)) {
        expected = x;
    }
}
```
The assembly code becomes:
```assembly
add7_weak(std::atomic<int>&):
.L12:
        movl    (%rdi), %eax
        leal    7(%rax), %edx
        lock cmpxchgl   %edx, (%rdi)
        jne     .L12
        ret
```
That is, `%eax` is now updated with the value stored in `x` within the loop.
If `%eax` is not used by the `lock cmpxchg` instruction, the optimizing compiler will
be silly to update `%eax` within the loop. A quick way to find out what `lock cmpxchg` does
is to look up the
[x86 and amd64 instruction reference for `cmpxchg`](https://www.felixcloutier.com/x86/cmpxchg).
In fact, there are some other instructions that also implicitly use `%rax`. For example,
the `syscall` instruction uses `%rax` for the system call number.

Also note that the [compare-and-swap](https://en.wikipedia.org/wiki/Compare-and-swap) (CAS)
operation does not write the `expected` value back to the memory location if the comparison
fails. So, the `lock cmpxchg` instruction does a little bit of extra work for CAS as it also
writes `%rax` if the comparison fails.
```c
int compare_and_swap(int* reg, int oldval, int newval)
{
    ATOMIC();
    int old_reg_val = *reg;
    if (old_reg_val == oldval)
        *reg = newval;
    END_ATOMIC();
    return old_reg_val;
}
```
