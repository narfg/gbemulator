cmake_minimum_required(VERSION 3.10)

project(emulator)

add_compile_options(-Wall -Wextra -Werror)

include_directories(src/)
include_directories(3rdparty/minifb/include)


# Build main executable
add_executable(emulator src/main.cpp src/romloader.cpp src/instructiondecoder.cpp src/cpu.cpp src/ppu.cpp src/timer.cpp 3rdparty/minifb/src/x11/X11MiniFB.c)
target_link_libraries(emulator X11)

add_executable(tracecomp src/tracecomp.cpp)
target_link_libraries(tracecomp)


# Build tests
enable_testing()
add_subdirectory(3rdparty/Catch2)
include_directories(3rdparty/Catch2/single_include)

add_executable(test_cpu tests/test_cpu.cpp src/cpu.cpp src/romloader.cpp src/instructiondecoder.cpp src/ppu.cpp src/timer.cpp 3rdparty/minifb/src/x11/X11MiniFB.c)
target_link_libraries(test_cpu Catch2::Catch2 X11)
add_test(NAME test_cpu COMMAND test_cpu)

add_executable(test_romloader tests/test_romloader.cpp src/romloader.cpp)
target_link_libraries(test_romloader Catch2::Catch2)
add_test(NAME test_romloader COMMAND test_romloader)

add_executable(test_instructiondecoder tests/test_instructiondecoder.cpp src/instructiondecoder.cpp src/cpu.cpp src/timer.cpp src/romloader.cpp src/ppu.cpp 3rdparty/minifb/src/x11/X11MiniFB.c)
target_link_libraries(test_instructiondecoder Catch2::Catch2 X11)
add_test(NAME test_instructiondecoder COMMAND test_instructiondecoder)

add_executable(test_ppu tests/test_ppu.cpp src/ppu.cpp src/romloader.cpp 3rdparty/minifb/src/x11/X11MiniFB.c)
target_link_libraries(test_ppu Catch2::Catch2 X11)
add_test(NAME test_ppu COMMAND test_ppu)

add_executable(test_timer tests/test_timer.cpp src/timer.cpp src/romloader.cpp)
target_link_libraries(test_timer Catch2::Catch2)
add_test(NAME test_timer COMMAND test_timer)
