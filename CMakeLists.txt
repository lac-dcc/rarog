cmake_minimum_required(VERSION 3.20.0)
project(Rarog)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

#===============================================================================
# LLVM config
#===============================================================================
set(MLIR_INSTALL_DIR "$ENV{MLIR_INSTALL_DIR}" CACHE PATH "LLVM installation directory")
set(LLVM_CMAKE_CONFIG_DIR "" "${MLIR_INSTALL_DIR}/lib/cmake/llvm/")
set(MLIR_CMAKE_CONFIG_DIR "" "${MLIR_INSTALL_DIR}/lib/cmake/mlir/")
list(APPEND CMAKE_PREFIX_PATH "${LLVM_CMAKE_CONFIG_DIR}")
list(APPEND CMAKE_PREFIX_PATH "${MLIR_CMAKE_CONFIG_DIR}")

find_package(LLVM REQUIRED CONFIG)
find_package(MLIR REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")

list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${MLIR_LLVM_DIR}")

include(AddLLVM)
include(AddMLIR)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${MLIR_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

#===============================================================================
# Project config
#===============================================================================
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(lib)
add_subdirectory(tools)
# add_subdirectory(test)