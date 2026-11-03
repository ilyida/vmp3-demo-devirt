cmake_minimum_required(VERSION 3.15)
project(vmp3-demo-devirt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

add_subdirectory(dependencies/argparse)
add_subdirectory(dependencies/libpeconv/libpeconv)
add_subdirectory(dependencies/loguru)
add_subdirectory(dependencies/unicorn)
add_subdirectory(dependencies/zydis)

file(GLOB_RECURSE CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

add_executable(vmp3-demo-devirt ${CPP_FILES} ${HEADER_FILES})

set(LLVM_DIR "C:/Dev/llvm-project/build/lib/cmake/llvm")
find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

target_include_directories(vmp3-demo-devirt PRIVATE
    src
    dependencies/argparse/include
    dependencies/libpeconv/libpeconv/include
    dependencies/loguru
    dependencies/unicorn/include
    dependencies/zydis/include)

target_link_libraries(vmp3-demo-devirt PRIVATE
    libpeconv
    loguru
    unicorn
    Zydis
    LLVMCore
    LLVMSupport
    LLVMIRReader
    LLVMAnalysis
    LLVMX86Disassembler
    LLVMX86AsmParser
    LLVMX86CodeGen
    LLVMX86Desc
    LLVMX86Info)

add_compile_definitions(NOMINMAX)
