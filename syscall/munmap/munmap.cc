#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cstdio>

int syscall_munmap(void *addr, size_t length) {
#if defined(__x86_64__)
    long ret;

    asm volatile("syscall"
                 : "=a"(ret)
                 : "a"(11),     // SYS_munmap
                   "D"(addr),   // rdi
                   "S"(length)  // rsi
                 : "rcx", "r11", "memory");

#elif defined(__aarch64__)
    register long x8 asm("x8") = 215;  // __NR_munmap on AArch64
    register long x0 asm("x0") = (long)addr;
    register long x1 asm("x1") = (long)length;
    asm volatile("svc 0" : "+r"(x0) : "r"(x1), "r"(x8) : "memory");
    long ret = x0;
#else
#error "Unsupported architecture"
#endif

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
