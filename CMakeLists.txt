cmake_minimum_required(VERSION 3.10)

project(emulator)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Werror)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -coverage -O0")
set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -coverage -O0")

include_directories(src/)

# SDL
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})


# Build main executable
add_executable(emulator src/main.cpp src/romloader.cpp src/instructiondecoder.cpp src/cpu.cpp src/ppu.cpp src/timer.cpp src/joypad.cpp src/sdldisplay.cpp)
target_link_libraries(emulator ${SDL2_LIBRARIES})

add_executable(tracecomp src/tracecomp.cpp)
target_link_libraries(tracecomp)


# Build tests
enable_testing()
add_subdirectory(3rdparty/Catch2)
include_directories(3rdparty/Catch2/single_include)

add_executable(test_cpu tests/test_cpu.cpp src/cpu.cpp src/romloader.cpp src/instructiondecoder.cpp src/ppu.cpp src/timer.cpp src/joypad.cpp src/sdldisplay.cpp)
target_link_libraries(test_cpu Catch2::Catch2 ${SDL2_LIBRARIES})
add_test(NAME test_cpu COMMAND test_cpu)

add_executable(test_romloader tests/test_romloader.cpp src/romloader.cpp)
target_link_libraries(test_romloader Catch2::Catch2)
add_test(NAME test_romloader COMMAND test_romloader)

add_executable(test_instructiondecoder tests/test_instructiondecoder.cpp src/instructiondecoder.cpp src/cpu.cpp src/timer.cpp src/joypad.cpp src/romloader.cpp src/ppu.cpp src/sdldisplay.cpp)
target_link_libraries(test_instructiondecoder Catch2::Catch2 ${SDL2_LIBRARIES})
add_test(NAME test_instructiondecoder COMMAND test_instructiondecoder)

add_executable(test_ppu tests/test_ppu.cpp src/ppu.cpp src/romloader.cpp src/joypad.cpp src/sdldisplay.cpp)
target_link_libraries(test_ppu Catch2::Catch2 ${SDL2_LIBRARIES})
add_test(NAME test_ppu COMMAND test_ppu)

add_executable(test_timer tests/test_timer.cpp src/timer.cpp src/romloader.cpp)
target_link_libraries(test_timer Catch2::Catch2)
add_test(NAME test_timer COMMAND test_timer)
