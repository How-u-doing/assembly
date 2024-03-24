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
