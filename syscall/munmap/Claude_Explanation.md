# Debugging a SIGSEGV after an over-sized `munmap`

## Problem

`munmap.cc` maps the first 1 MiB of `/bin/ls`, then calls `munmap` on that
address with a length of **1 GiB** (`unmap_size`), via a raw `syscall`:

```c++
void *addr = do_mmap(fd, 1ULL * 1024 * 1024);       // 1 MiB
syscall_munmap(addr, 1024ULL * 1024 * 1024);        // 1 GiB  <-- bug
```

The process dies with SIGSEGV. In the core:

- PC is the instruction right after `syscall` in `syscall_munmap`
  (`mov %rax,-0x18(%rbp)`).
- `$rax == 0`, so `munmap` succeeded.
- `$rbp-0x18` is on the `[stack]`, which was **not** unmapped.
- On loading the core, GDB prints
  `Cannot access memory at address 0x...2b4128 / 0x...2b4130`.
- `info proc mappings` lists only `a.out`.

Questions:
1. Did the stack store `-0x18(%rbp)` cause the crash?
2. Where do the GDB "Cannot access memory" messages come from?

## What the 1 GiB unmap removed

Starting at the 1 MiB `/bin/ls` mapping, the 1 GiB range also covers:

- all of `libc.so.6` (text, rodata, data, bss)
- all of `ld-linux-x86-64.so.2`
- the anonymous mappings between them, including the **main thread's
  TCB/static TLS**, where `%fs` points
  (`arch_prctl(ARCH_SET_FS, 0x761925454740)` falls in
  `761925454000-761925457000`)

It does **not** reach the stack (`0x7ffc...`), vvar, vdso or `a.out`.

## Answer 1: the stack store did not crash. The kernel sent the signal.

### Evidence

| Run | strace signal line |
|---|---|
| default | `SIGSEGV {si_code=SI_KERNEL, si_addr=NULL}` |
| `GLIBC_TUNABLES=glibc.pthread.rseq=0` | `SIGSEGV {si_code=SEGV_MAPERR, si_addr=0x76192522a1ca}` |

- A real page fault always gives `SEGV_MAPERR`/`SEGV_ACCERR` plus an address.
  `SI_KERNEL` with `si_addr=NULL` comes from the kernel calling
  `force_sig(SIGSEGV)`.

### Mechanism (default run, rseq on)

Since glibc 2.35, glibc registers a `struct rseq` (restartable sequences)
with the kernel for every thread. It lives inside the thread's TLS/TCB.

1. `munmap` removes the last references to the libc/ld.so files (ld.so
   closed the fds after mapping). The final `fput()` is deferred as task work,
   which sets `TIF_NOTIFY_RESUME`. Preemption/migration during the long unmap
   can set the same flag.
2. On the way back to user space the kernel runs `resume_user_mode_work()`
   → `task_work_run()` and `rseq_handle_notify_resume()`.
3. `rseq_handle_notify_resume()` reads `rseq->rseq_cs` and writes
   `cpu_id` into the registered rseq area. That memory is gone, so
   `get_user`/`put_user` fails with `-EFAULT` → `force_sig(SIGSEGV)`.
4. The signal arrives before any user instruction runs, so the saved PC is the
   instruction right after `syscall`. That is why the backtrace points at
   `mov %rax,-0x18(%rbp)`, even though that store is harmless.

The glibc `munmap()` wrapper would crash the same way.

### Confirmation run (rseq off)

With `GLIBC_TUNABLES=glibc.pthread.rseq=0`:

- The `rseq(...) = 0` line is missing from strace, so no rseq area is registered.
- `munmap` returns normally. The store to `-0x18(%rbp)`, the rest of
  `syscall_munmap` and `main`'s epilogue all run.
- The crash moves to the `ret` from `main`:
  `si_addr = 0x76192522a1ca` = libc base `0x761925200000` + `0x2a1ca`,
  inside libc's r-x segment. On Ubuntu 24.04 (glibc 2.39) this is the return
  address in `__libc_start_call_main`, right after `call *%rax` into `main`.
  `si_addr` equal to the PC means the fault is an **instruction fetch** from
  the unmapped libc page.

  ```bash
  gdb -batch -ex 'info symbol 0x2a1ca' /lib/x86_64-linux-gnu/libc.so.6
  ```

### Summary table

| Run | Who raises SIGSEGV | When | PC / si_addr |
|---|---|---|---|
| rseq on (default) | kernel, `force_sig` in the rseq notify-resume path | syscall exit, before returning to user | PC = insn after `syscall`; `SI_KERNEL`, `si_addr=NULL` |
| `rseq=0` | MMU page fault, instruction fetch | `ret` from `main` into libc | PC = `si_addr` = libc+0x2a1ca; `SEGV_MAPERR` |

In both runs the store to `-0x18(%rbp)` succeeds.

## Answer 2: GDB's "Cannot access memory at 0x...2b4128/4130"

- These addresses are in ld.so's writable data segment
  (`789eba2b3000-789eba2b5000 rw-p ... ld-linux-x86-64.so.2`), which holds
  `_r_debug` and the `link_map` list.
- When GDB opens the core, it reads `DT_DEBUG` from `a.out`'s `.dynamic`
  (still mapped and in the core). That gives a pointer to `_r_debug`, which GDB
  then follows to walk `r_map` / `link_map` and load shared-library symbols.
- That memory was unmapped before the crash, so it is not in the core. The
  reads fail (some are retried, so `...128` appears twice), and no shared
  libraries are loaded.
- `info proc mappings` on a core is built from the NT_FILE note, which only
  lists **file-backed** mappings that still exist. libc/ld.so are gone, so
  only `a.out` appears. The stack is still in the core: anonymous mappings
  just aren't listed in NT_FILE (check with `info files`).

## Other kernel-side pointers into the unmapped TLS

From strace:

- `set_tid_address(0x761925454a10)`: `clear_child_tid`
- `set_robust_list(0x761925454a20, 24)`: robust futex list

Both point into the unmapped TCB region. At exit, the kernel's write to
`clear_child_tid` and its robust-list walk would fail **silently**, without
a signal. rseq is the only one of these that raises SIGSEGV, so it is the one
that shows up.

## Takeaways

- A crash PC right after `syscall` with `SI_KERNEL` / `si_addr=NULL` means the
  kernel raised the signal on syscall exit. It does not mean the next
  instruction faulted.
- Unmapping memory you don't own (libc, ld.so, TLS) breaks the process
  immediately. The kernel itself depends on per-thread user memory (rseq,
  `clear_child_tid`, robust list).
- Fix: unmap exactly what was mapped (`munmap(addr, map_size)`).

## Useful commands

```bash
strace ./a.out                                   # see si_code / si_addr
GLIBC_TUNABLES=glibc.pthread.rseq=0 strace ./a.out
gdb ./a.out core.a.out.<pid>
(gdb) p $_siginfo                                # si_code=128 (SI_KERNEL) vs SEGV_MAPERR
(gdb) info files                                 # segments actually in the core
```
