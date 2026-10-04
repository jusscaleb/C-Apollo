cmake_minimum_required(VERSION 3.20)
project(Apollo C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Default to Release for optimized Apollo compiler binary
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

# Include header directories
include_directories(src src/API)
include_directories(src src/Semantics)
include_directories(src src/Parser)

# Find all C source files recursively under src/
file(GLOB_RECURSE SOURCES "src/*.c")

# Define target executable
add_executable(apollo ${SOURCES})

# Include and link MSYS2 MinGW LLVM dependencies
if(DEFINED ENV{MINGW_PREFIX})
    target_include_directories(apollo PRIVATE "$ENV{MINGW_PREFIX}/include")
    target_link_directories(apollo PRIVATE "$ENV{MINGW_PREFIX}/lib")
else()
    target_include_directories(apollo PRIVATE "C:/msys64/mingw64/include")
    target_link_directories(apollo PRIVATE "C:/msys64/mingw64/lib")
endif()

find_library(LLVM_LIB NAMES LLVM-19 LLVM HINTS "$ENV{MINGW_PREFIX}/lib" "C:/msys64/mingw64/lib")
if(LLVM_LIB)
    target_link_libraries(apollo PRIVATE ${LLVM_LIB})
else()
    target_link_libraries(apollo PRIVATE LLVM-19)
endif()
