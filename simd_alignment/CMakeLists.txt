cmake_minimum_required(VERSION 3.14)

project(xsimd_benchmark)

# you need to install google benchmark first
find_package(benchmark REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

include_directories($ENV{XSIMD_HOME}/include)

set(BIN_NAME bench)
add_executable(${BIN_NAME} src/bench.cc)
target_link_libraries(${BIN_NAME} benchmark::benchmark)

set(BIN_NAME unaligned)
add_executable(${BIN_NAME} src/unaligned.cc)

function(add_bench_shift shift_size)
    set(BIN_NAME bench_shift_${shift_size})
    add_executable(${BIN_NAME} src/bench_shift.cc)
    target_compile_definitions(${BIN_NAME} PRIVATE SMALL_SHIFT=${shift_size})
    target_link_libraries(${BIN_NAME} benchmark::benchmark)
endfunction()

add_bench_shift(1)
add_bench_shift(2)
add_bench_shift(4)
add_bench_shift(8)
add_bench_shift(16)
add_bench_shift(32)

set(BIN_NAME aligned_data_vmovupd_ins)
add_executable(${BIN_NAME} src/aligned_data_vmovupd_ins.cc)
target_link_libraries(${BIN_NAME} benchmark::benchmark)
