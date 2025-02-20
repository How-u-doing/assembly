## Data alignment impact on SIMD operations

I always thought (& heard people saying) that aligned operations are significantly faster than unaligned operations (20%+, or even several times!). But that's soooo not true, at least on a modern CPU. The **difference is little pronounced** as is with [Daniel Lemire's blog].

`round()` (291645: 318643), `floor()` (236888: 259448), `ceil()` (235430: 261249) has the most noticeable difference, around 10%. But I purposely made it aligned to 1 byte. In reality, the compiler is likely to align it to 8 or 16 bytes, ~~which would make the difference way less noticeable~~.

However, if the **data is aligned to a simd vecter size** and we apply an **unaligned** load/store operation (**vmovupd** instead of vmovapd) to the data, then there's **no performance penalty**! See the section [Using vmovupd on aligned data](#using-vmovupd-on-aligned-data).

[Daniel Lemire's blog]: https://lemire.me/blog/2012/05/31/data-alignment-for-speed-myth-or-reality/

## Results
The following benchmarks are performed on a Q4'21 [i9-12900KF] processor with AVX2 support.

### Using vmovupd on aligned data
```bash
$ make XSIMD_HOME=$HOME/repos/xsimd

$ build/aligned_data_vmovupd_ins --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
2025-02-20T16:57:09+08:00
Running build/aligned_data_vmovupd_ins
Run on (24 X 3187.2 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x12)
  L1 Instruction 32 KiB (x12)
  L2 Unified 1280 KiB (x12)
  L3 Unified 30720 KiB (x1)
Load Average: 0.42, 0.38, 0.22
abs_aligned x addr: 0x5583e1464900, z addr: 0x5583e1c05b00
------------------------------------------------------------------------
Benchmark                              Time             CPU   Iterations
------------------------------------------------------------------------
BM_xsimd_abs_aligned_mean         241623 ns       250479 ns           10
BM_xsimd_abs_aligned_median       241816 ns       250722 ns           10
BM_xsimd_abs_aligned_stddev         2547 ns         2726 ns           10
BM_xsimd_abs_aligned_cv             1.05 %          1.09 %            10
abs_unaligned x addr: 0x5583e23a6d20, z addr: 0x5583e2b47f20
BM_xsimd_abs_unaligned_mean       243412 ns       251866 ns           10
BM_xsimd_abs_unaligned_median     243152 ns       251532 ns           10
BM_xsimd_abs_unaligned_stddev       2125 ns         2141 ns           10
BM_xsimd_abs_unaligned_cv           0.87 %          0.85 %            10

$ objdump -Cd build/aligned_data_vmovupd_ins | grep vmovapd -B 7 -A 3
    b2b2:       c4 e2 7d 19 0d fd 3d    vbroadcastsd 0x53dfd(%rip),%ymm1        # 5f0b8 <_IO_stdin_used+0x10b8>
    b2b9:       05 00
    b2bb:       4d 85 f6                test   %r14,%r14
    b2be:       74 26                   je     b2e6 <BM_xsimd_abs_aligned(benchmark::State&)+0x106>
    b2c0:       31 c0                   xor    %eax,%eax
    b2c2:       66 0f 1f 44 00 00       nopw   0x0(%rax,%rax,1)
    b2c8:       c5 f5 55 04 c3          vandnpd (%rbx,%rax,8),%ymm1,%ymm0
    b2cd:       c4 c1 7d 29 44 c5 00    vmovapd %ymm0,0x0(%r13,%rax,8)
    b2d4:       48 83 c0 04             add    $0x4,%rax
    b2d8:       48 3d 40 42 0f 00       cmp    $0xf4240,%rax
    b2de:       75 e8                   jne    b2c8 <BM_xsimd_abs_aligned(benchmark::State&)+0xe8>

$ objdump -Cd build/aligned_data_vmovupd_ins | grep vmovupd -B 7 -A 3
    b132:       c4 e2 7d 19 0d 7d 3f    vbroadcastsd 0x53f7d(%rip),%ymm1        # 5f0b8 <_IO_stdin_used+0x10b8>
    b139:       05 00
    b13b:       4d 85 f6                test   %r14,%r14
    b13e:       74 26                   je     b166 <BM_xsimd_abs_unaligned(benchmark::State&)+0x106>
    b140:       31 c0                   xor    %eax,%eax
    b142:       66 0f 1f 44 00 00       nopw   0x0(%rax,%rax,1)
    b148:       c5 f5 55 04 c3          vandnpd (%rbx,%rax,8),%ymm1,%ymm0
    b14d:       c4 c1 7d 11 44 c5 00    vmovupd %ymm0,0x0(%r13,%rax,8)
    b154:       48 83 c0 04             add    $0x4,%rax
    b158:       48 3d 40 42 0f 00       cmp    $0xf4240,%rax
    b15e:       75 e8                   jne    b148 <BM_xsimd_abs_unaligned(benchmark::State&)+0xe8>
```

### Alignments of [1 2 4 8 16 32] bytes
```bash
$ build/bench_shift_1 --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
2025-02-20T10:30:27+08:00
Running build/bench_shift_1
Run on (24 X 3187.2 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x12)
  L1 Instruction 32 KiB (x12)
  L2 Unified 1280 KiB (x12)
  L3 Unified 30720 KiB (x1)
Load Average: 0.22, 0.28, 0.31
ceil_aligned x addr: 0x55d29c6fe8c0, z addr: 0x55d29d640cc0
-------------------------------------------------------------------------
Benchmark                               Time             CPU   Iterations
-------------------------------------------------------------------------
BM_xsimd_ceil_aligned_mean         241286 ns       251899 ns           10
BM_xsimd_ceil_aligned_median       241740 ns       252454 ns           10
BM_xsimd_ceil_aligned_stddev         2444 ns         2583 ns           10
BM_xsimd_ceil_aligned_cv             1.01 %          1.03 %            10
ceil_unaligned x addr: 0x55d29dde1ec1, z addr: 0x55d29ed242c1
BM_xsimd_ceil_unaligned_mean       253026 ns       263941 ns           10
BM_xsimd_ceil_unaligned_median     252097 ns       262997 ns           10
BM_xsimd_ceil_unaligned_stddev       6954 ns         7253 ns           10
BM_xsimd_ceil_unaligned_cv           2.75 %          2.75 %            10
BM_std_ceil_aligned_mean           271441 ns       283211 ns           10
BM_std_ceil_aligned_median         269863 ns       281563 ns           10
BM_std_ceil_aligned_stddev           3691 ns         3851 ns           10
BM_std_ceil_aligned_cv               1.36 %          1.36 %            10
BM_std_ceil_unaligned_mean         283187 ns       295926 ns           10
BM_std_ceil_unaligned_median       278000 ns       290422 ns           10
BM_std_ceil_unaligned_stddev         8303 ns         8691 ns           10
BM_std_ceil_unaligned_cv             2.93 %          2.94 %            10
abs_aligned x addr: 0x55d29c6fe8c0, z addr: 0x55d29d640cc0
BM_xsimd_abs_aligned_mean          233549 ns       243986 ns           10
BM_xsimd_abs_aligned_median        232596 ns       243024 ns           10
BM_xsimd_abs_aligned_stddev          3440 ns         3591 ns           10
BM_xsimd_abs_aligned_cv              1.47 %          1.47 %            10
abs_unaligned x addr: 0x55d29dde1ec1, z addr: 0x55d29ed242c1
BM_xsimd_abs_unaligned_mean        242558 ns       253165 ns           10
BM_xsimd_abs_unaligned_median      242042 ns       252684 ns           10
BM_xsimd_abs_unaligned_stddev        2540 ns         2710 ns           10
BM_xsimd_abs_unaligned_cv            1.05 %          1.07 %            10
BM_std_abs_aligned_mean            233580 ns       243547 ns           10
BM_std_abs_aligned_median          232259 ns       242321 ns           10
BM_std_abs_aligned_stddev            3140 ns         3250 ns           10
BM_std_abs_aligned_cv                1.34 %          1.33 %            10
BM_std_abs_unaligned_mean          241130 ns       251600 ns           10
BM_std_abs_unaligned_median        241447 ns       251618 ns           10
BM_std_abs_unaligned_stddev          2077 ns         1915 ns           10
BM_std_abs_unaligned_cv              0.86 %          0.76 %            10

$ build/bench_shift_2 --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
2025-02-20T10:31:25+08:00
Running build/bench_shift_2
Run on (24 X 3187.2 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x12)
  L1 Instruction 32 KiB (x12)
  L2 Unified 1280 KiB (x12)
  L3 Unified 30720 KiB (x1)
Load Average: 0.69, 0.40, 0.36
ceil_aligned x addr: 0x55b7dfeb48c0, z addr: 0x55b7e0df6cc0
-------------------------------------------------------------------------
Benchmark                               Time             CPU   Iterations
-------------------------------------------------------------------------
BM_xsimd_ceil_aligned_mean         233366 ns       243988 ns           10
BM_xsimd_ceil_aligned_median       232429 ns       242993 ns           10
BM_xsimd_ceil_aligned_stddev         4650 ns         4862 ns           10
BM_xsimd_ceil_aligned_cv             1.99 %          1.99 %            10
ceil_unaligned x addr: 0x55b7e1597ec2, z addr: 0x55b7e24da2c2
BM_xsimd_ceil_unaligned_mean       248785 ns       260125 ns           10
BM_xsimd_ceil_unaligned_median     249955 ns       261393 ns           10
BM_xsimd_ceil_unaligned_stddev       4743 ns         4926 ns           10
BM_xsimd_ceil_unaligned_cv           1.91 %          1.89 %            10
BM_std_ceil_aligned_mean           270288 ns       281794 ns           10
BM_std_ceil_aligned_median         268417 ns       279845 ns           10
BM_std_ceil_aligned_stddev           4951 ns         5162 ns           10
BM_std_ceil_aligned_cv               1.83 %          1.83 %            10
BM_std_ceil_unaligned_mean         286047 ns       297732 ns           10
BM_std_ceil_unaligned_median       282564 ns       294091 ns           10
BM_std_ceil_unaligned_stddev         8553 ns         8950 ns           10
BM_std_ceil_unaligned_cv             2.99 %          3.01 %            10
abs_aligned x addr: 0x55b7dfeb48c0, z addr: 0x55b7e0df6cc0
BM_xsimd_abs_aligned_mean          231536 ns       241535 ns           10
BM_xsimd_abs_aligned_median        231650 ns       241826 ns           10
BM_xsimd_abs_aligned_stddev          3076 ns         3088 ns           10
BM_xsimd_abs_aligned_cv              1.33 %          1.28 %            10
abs_unaligned x addr: 0x55b7e1597ec2, z addr: 0x55b7e24da2c2
BM_xsimd_abs_unaligned_mean        240873 ns       251571 ns           10
BM_xsimd_abs_unaligned_median      241398 ns       252195 ns           10
BM_xsimd_abs_unaligned_stddev        4453 ns         4602 ns           10
BM_xsimd_abs_unaligned_cv            1.85 %          1.83 %            10
BM_std_abs_aligned_mean            231466 ns       241594 ns           10
BM_std_abs_aligned_median          230540 ns       240851 ns           10
BM_std_abs_aligned_stddev            3697 ns         3841 ns           10
BM_std_abs_aligned_cv                1.60 %          1.59 %            10
BM_std_abs_unaligned_mean          242785 ns       252511 ns           10
BM_std_abs_unaligned_median        242844 ns       251901 ns           10
BM_std_abs_unaligned_stddev          1795 ns         2063 ns           10
BM_std_abs_unaligned_cv              0.74 %          0.82 %            10

$ build/bench_shift_4 --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
2025-02-20T10:32:24+08:00
Running build/bench_shift_4
Run on (24 X 3187.2 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x12)
  L1 Instruction 32 KiB (x12)
  L2 Unified 1280 KiB (x12)
  L3 Unified 30720 KiB (x1)
Load Average: 0.88, 0.50, 0.40
ceil_aligned x addr: 0x5581e509e8c0, z addr: 0x5581e5fe0cc0
-------------------------------------------------------------------------
Benchmark                               Time             CPU   Iterations
-------------------------------------------------------------------------
BM_xsimd_ceil_aligned_mean         232485 ns       241328 ns           10
BM_xsimd_ceil_aligned_median       231673 ns       240125 ns           10
BM_xsimd_ceil_aligned_stddev         4289 ns         3695 ns           10
BM_xsimd_ceil_aligned_cv             1.84 %          1.53 %            10
ceil_unaligned x addr: 0x5581e6781ec4, z addr: 0x5581e76c42c4
BM_xsimd_ceil_unaligned_mean       246016 ns       261850 ns           10
BM_xsimd_ceil_unaligned_median     245745 ns       260861 ns           10
BM_xsimd_ceil_unaligned_stddev       2240 ns         2416 ns           10
BM_xsimd_ceil_unaligned_cv           0.91 %          0.92 %            10
BM_std_ceil_aligned_mean           263358 ns       275456 ns           10
BM_std_ceil_aligned_median         262448 ns       274509 ns           10
BM_std_ceil_aligned_stddev           3235 ns         3388 ns           10
BM_std_ceil_aligned_cv               1.23 %          1.23 %            10
BM_std_ceil_unaligned_mean         277219 ns       288929 ns           10
BM_std_ceil_unaligned_median       275967 ns       287456 ns           10
BM_std_ceil_unaligned_stddev         3347 ns         3390 ns           10
BM_std_ceil_unaligned_cv             1.21 %          1.17 %            10
abs_aligned x addr: 0x5581e509e8c0, z addr: 0x5581e5fe0cc0
BM_xsimd_abs_aligned_mean          232418 ns       238510 ns           10
BM_xsimd_abs_aligned_median        231931 ns       238554 ns           10
BM_xsimd_abs_aligned_stddev          4088 ns         2014 ns           10
BM_xsimd_abs_aligned_cv              1.76 %          0.84 %            10
abs_unaligned x addr: 0x5581e6781ec4, z addr: 0x5581e76c42c4
BM_xsimd_abs_unaligned_mean        237315 ns       250308 ns           10
BM_xsimd_abs_unaligned_median      236951 ns       251032 ns           10
BM_xsimd_abs_unaligned_stddev        4896 ns         3679 ns           10
BM_xsimd_abs_unaligned_cv            2.06 %          1.47 %            10
BM_std_abs_aligned_mean            229848 ns       241685 ns           10
BM_std_abs_aligned_median          229160 ns       240256 ns           10
BM_std_abs_aligned_stddev            3494 ns         3735 ns           10
BM_std_abs_aligned_cv                1.52 %          1.55 %            10
BM_std_abs_unaligned_mean          240861 ns       251250 ns           10
BM_std_abs_unaligned_median        240552 ns       250869 ns           10
BM_std_abs_unaligned_stddev          2541 ns         2571 ns           10
BM_std_abs_unaligned_cv              1.05 %          1.02 %            10

$ build/bench_shift_8 --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
2025-02-20T10:33:21+08:00
Running build/bench_shift_8
Run on (24 X 3187.2 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x12)
  L1 Instruction 32 KiB (x12)
  L2 Unified 1280 KiB (x12)
  L3 Unified 30720 KiB (x1)
Load Average: 0.95, 0.59, 0.43
ceil_aligned x addr: 0x55d7d34088c0, z addr: 0x55d7d434acc0
-------------------------------------------------------------------------
Benchmark                               Time             CPU   Iterations
-------------------------------------------------------------------------
BM_xsimd_ceil_aligned_mean         233782 ns       242477 ns           10
BM_xsimd_ceil_aligned_median       233010 ns       242022 ns           10
BM_xsimd_ceil_aligned_stddev         5256 ns         4394 ns           10
BM_xsimd_ceil_aligned_cv             2.25 %          1.81 %            10
ceil_unaligned x addr: 0x55d7d4aebec8, z addr: 0x55d7d5a2e2c8
BM_xsimd_ceil_unaligned_mean       252043 ns       260111 ns           10
BM_xsimd_ceil_unaligned_median     250955 ns       260165 ns           10
BM_xsimd_ceil_unaligned_stddev       3645 ns         2632 ns           10
BM_xsimd_ceil_unaligned_cv           1.45 %          1.01 %            10
BM_std_ceil_aligned_mean           264628 ns       283562 ns           10
BM_std_ceil_aligned_median         262258 ns       280329 ns           10
BM_std_ceil_aligned_stddev           7537 ns         8723 ns           10
BM_std_ceil_aligned_cv               2.85 %          3.08 %            10
BM_std_ceil_unaligned_mean         262157 ns       273950 ns           10
BM_std_ceil_unaligned_median       260910 ns       272734 ns           10
BM_std_ceil_unaligned_stddev         2964 ns         3061 ns           10
BM_std_ceil_unaligned_cv             1.13 %          1.12 %            10
abs_aligned x addr: 0x55d7d34088c0, z addr: 0x55d7d434acc0
BM_xsimd_abs_aligned_mean          232377 ns       242485 ns           10
BM_xsimd_abs_aligned_median        233089 ns       243349 ns           10
BM_xsimd_abs_aligned_stddev          3952 ns         4268 ns           10
BM_xsimd_abs_aligned_cv              1.70 %          1.76 %            10
abs_unaligned x addr: 0x55d7d4aebec8, z addr: 0x55d7d5a2e2c8
BM_xsimd_abs_unaligned_mean        246689 ns       251247 ns           10
BM_xsimd_abs_unaligned_median      247590 ns       250635 ns           10
BM_xsimd_abs_unaligned_stddev        3906 ns         3352 ns           10
BM_xsimd_abs_unaligned_cv            1.58 %          1.33 %            10
BM_std_abs_aligned_mean            229131 ns       241024 ns           10
BM_std_abs_aligned_median          229897 ns       240670 ns           10
BM_std_abs_aligned_stddev            4919 ns         4354 ns           10
BM_std_abs_aligned_cv                2.15 %          1.81 %            10
BM_std_abs_unaligned_mean          234016 ns       249419 ns           10
BM_std_abs_unaligned_median        234413 ns       249928 ns           10
BM_std_abs_unaligned_stddev          3845 ns         4131 ns           10
BM_std_abs_unaligned_cv              1.64 %          1.66 %            10

$ build/bench_shift_16 --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
2025-02-20T10:34:21+08:00
Running build/bench_shift_16
Run on (24 X 3187.2 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x12)
  L1 Instruction 32 KiB (x12)
  L2 Unified 1280 KiB (x12)
  L3 Unified 30720 KiB (x1)
Load Average: 0.98, 0.67, 0.47
ceil_aligned x addr: 0x5558e343f8c0, z addr: 0x5558e4381cc0
-------------------------------------------------------------------------
Benchmark                               Time             CPU   Iterations
-------------------------------------------------------------------------
BM_xsimd_ceil_aligned_mean         226190 ns       240296 ns           10
BM_xsimd_ceil_aligned_median       225531 ns       240759 ns           10
BM_xsimd_ceil_aligned_stddev         3412 ns         1668 ns           10
BM_xsimd_ceil_aligned_cv             1.51 %          0.69 %            10
ceil_unaligned x addr: 0x5558e4b22ed0, z addr: 0x5558e5a652d0
BM_xsimd_ceil_unaligned_mean       248232 ns       258851 ns           10
BM_xsimd_ceil_unaligned_median     248183 ns       258958 ns           10
BM_xsimd_ceil_unaligned_stddev       3081 ns         3153 ns           10
BM_xsimd_ceil_unaligned_cv           1.24 %          1.22 %            10
BM_std_ceil_aligned_mean           276793 ns       281224 ns           10
BM_std_ceil_aligned_median         276933 ns       281928 ns           10
BM_std_ceil_aligned_stddev           7428 ns         2756 ns           10
BM_std_ceil_aligned_cv               2.68 %          0.98 %            10
BM_std_ceil_unaligned_mean         256831 ns       276734 ns           10
BM_std_ceil_unaligned_median       254074 ns       273882 ns           10
BM_std_ceil_unaligned_stddev         6133 ns         6667 ns           10
BM_std_ceil_unaligned_cv             2.39 %          2.41 %            10
abs_aligned x addr: 0x5558e343f8c0, z addr: 0x5558e4381cc0
BM_xsimd_abs_aligned_mean          227151 ns       241102 ns           10
BM_xsimd_abs_aligned_median        227417 ns       240797 ns           10
BM_xsimd_abs_aligned_stddev          2530 ns         1972 ns           10
BM_xsimd_abs_aligned_cv              1.11 %          0.82 %            10
abs_unaligned x addr: 0x5558e4b22ed0, z addr: 0x5558e5a652d0
BM_xsimd_abs_unaligned_mean        239310 ns       250516 ns           10
BM_xsimd_abs_unaligned_median      238682 ns       250615 ns           10
BM_xsimd_abs_unaligned_stddev        3188 ns         2120 ns           10
BM_xsimd_abs_unaligned_cv            1.33 %          0.85 %            10
BM_std_abs_aligned_mean            236112 ns       239939 ns           10
BM_std_abs_aligned_median          235553 ns       238887 ns           10
BM_std_abs_aligned_stddev            5672 ns         3695 ns           10
BM_std_abs_aligned_cv                2.40 %          1.54 %            10
BM_std_abs_unaligned_mean          237084 ns       249948 ns           10
BM_std_abs_unaligned_median        239103 ns       249531 ns           10
BM_std_abs_unaligned_stddev          5134 ns         2569 ns           10
BM_std_abs_unaligned_cv              2.17 %          1.03 %            10

$ build/bench_shift_32 --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
2025-02-20T10:35:19+08:00
Running build/bench_shift_32
Run on (24 X 3187.2 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x12)
  L1 Instruction 32 KiB (x12)
  L2 Unified 1280 KiB (x12)
  L3 Unified 30720 KiB (x1)
Load Average: 1.00, 0.73, 0.50
ceil_aligned x addr: 0x560675f128c0, z addr: 0x560676e54cc0
-------------------------------------------------------------------------
Benchmark                               Time             CPU   Iterations
-------------------------------------------------------------------------
BM_xsimd_ceil_aligned_mean         227220 ns       242471 ns           10
BM_xsimd_ceil_aligned_median       226721 ns       242011 ns           10
BM_xsimd_ceil_aligned_stddev         3360 ns         2294 ns           10
BM_xsimd_ceil_aligned_cv             1.48 %          0.95 %            10
ceil_unaligned x addr: 0x5606775f5ee0, z addr: 0x5606785382e0
BM_xsimd_ceil_unaligned_mean       229333 ns       239297 ns           10
BM_xsimd_ceil_unaligned_median     229471 ns       239441 ns           10
BM_xsimd_ceil_unaligned_stddev       1952 ns         2037 ns           10
BM_xsimd_ceil_unaligned_cv           0.85 %          0.85 %            10
BM_std_ceil_aligned_mean           267561 ns       279050 ns           10
BM_std_ceil_aligned_median         266619 ns       278068 ns           10
BM_std_ceil_aligned_stddev           5450 ns         5684 ns           10
BM_std_ceil_aligned_cv               2.04 %          2.04 %            10
BM_std_ceil_unaligned_mean         269556 ns       276092 ns           10
BM_std_ceil_unaligned_median       268325 ns       273705 ns           10
BM_std_ceil_unaligned_stddev         7437 ns         8267 ns           10
BM_std_ceil_unaligned_cv             2.76 %          2.99 %            10
abs_aligned x addr: 0x560675f128c0, z addr: 0x560676e54cc0
BM_xsimd_abs_aligned_mean          232345 ns       242579 ns           10
BM_xsimd_abs_aligned_median        231282 ns       241441 ns           10
BM_xsimd_abs_aligned_stddev          4060 ns         4345 ns           10
BM_xsimd_abs_aligned_cv              1.75 %          1.79 %            10
abs_unaligned x addr: 0x5606775f5ee0, z addr: 0x5606785382e0
BM_xsimd_abs_unaligned_mean        227422 ns       242437 ns           10
BM_xsimd_abs_unaligned_median      226902 ns       241813 ns           10
BM_xsimd_abs_unaligned_stddev        4370 ns         2336 ns           10
BM_xsimd_abs_unaligned_cv            1.92 %          0.96 %            10
BM_std_abs_aligned_mean            229705 ns       241858 ns           10
BM_std_abs_aligned_median          229522 ns       241619 ns           10
BM_std_abs_aligned_stddev            1987 ns         3059 ns           10
BM_std_abs_aligned_cv                0.86 %          1.26 %            10
BM_std_abs_unaligned_mean          231490 ns       238828 ns           10
BM_std_abs_unaligned_median        231987 ns       237930 ns           10
BM_std_abs_unaligned_stddev          3667 ns         2902 ns           10
BM_std_abs_unaligned_cv              1.58 %          1.22 %            10
```

[i9-12900KF]: https://www.intel.com/content/www/us/en/products/sku/134600/intel-core-i912900kf-processor-30m-cache-up-to-5-20-ghz/specifications.html
