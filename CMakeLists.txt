cmake_minimum_required(VERSION 3.20)
project(Apollo C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Include header directories
include_directories(src src/API)
include_directories(src src/Semantics)
# Find all C source files recursively under src/
file(GLOB_RECURSE SOURCES "src/*.c")

# Define target executable
add_executable(apollo ${SOURCES})

# Include and link MSYS2 MinGW LLVM dependencies
target_include_directories(apollo PRIVATE "C:/msys64/mingw64/include")
target_link_directories(apollo PRIVATE "C:/msys64/mingw64/lib")
target_link_libraries(apollo PRIVATE LLVM-19)
