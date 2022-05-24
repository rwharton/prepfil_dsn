cmake_minimum_required(VERSION 2.8.9 FATAL_ERROR)

enable_testing()

# set Release build type as default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
    "Configuration type (one of Debug, RelWithDebInfo, Release, MinSizeRel)"
    FORCE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

project(filterbank_utils CXX)

# enable warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# enable C++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# generate debugging symbols for release and debug
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# explicitly set DEBUG flag in Debug mode
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

# to make shared libraries we need position independent code
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# allow user to override install directories
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")

option(CUDA "Turn CUDA on or off" On)

if(${CUDA})
  find_package(CUDA)
  if (${CUDA_FOUND})
    include_directories(${CUDA_INCLUDE_DIRS})
    set(EXTERNAL_LIBS "${EXTERNAL_LIBS};${CUDA_LIBRARIES}")
    set(EXTERNAL_LIBS "${EXTERNAL_LIBS};${CUDA_CUFFT_LIBRARIES}")
    set(CUDA_SEPARABLE_COMPILATION On)
    #set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_52,code=sm_52; -gencode arch=compute_61,code=sm_61)
    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_80,code=sm_80)
    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};--ptxas-options=-v)
    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};--maxrregcount 32)
  endif()
else()
  set(CUDA_FOUND False)
endif()

if (${CUDA_FOUND})
  find_package(FFTW)
else()
  find_package(FFTW REQUIRED)
endif()

if (${FFTW_FOUND})
  include_directories(${FFTW_INCLUDES})
  set(EXTERNAL_LIBS "${EXTERNAL_LIBS};${FFTW_LIBRARIES}")
endif()

find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
set(EXTERNAL_LIBS "${EXTERNAL_LIBS};${PROTOBUF_LIBRARIES}")
set(PROTOBUF_IMPORT_DIRS "${filterbank_utils_SOURCE_DIR}/src")

find_package(Boost 1.32.0 REQUIRED COMPONENTS filesystem system regex)
include_directories(${Boost_INCLUDE_DIRS})
set(EXTERNAL_LIBS "${EXTERNAL_LIBS};${Boost_LIBRARIES}")

find_package(OpenMP)
if (${OPENMP_FOUND})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

include_directories(src)

# include the genrated protobuf headers
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

add_subdirectory(src)
add_subdirectory(exec)

if (${CUDA_FOUND})
  set(CUDA_SRCS "${CUDA_SRCS};src/BaselineRemover_GPU.cu")

  cuda_add_library(filterbank_utils_cuda_lib ${CUDA_SRCS})
  cuda_compute_separable_compilation_object_file_name(
    filterbank_utils_cuda_link
    filterbank_utils_cuda_lib
    "${filterbank_utils_cuda_lib_SEPARABLE_COMPILATION_OBJECTS}"
  )

set(LIBS "${LIBS};${filterbank_utils_cuda_lib_SEPARABLE_COMPILATION_OBJECTS};${filterbank_utils_cuda_link}")
endif()

# create static library that will be used to link tests and executables
add_library(filterbank_utils_static
  ${LIBS}
)

set_target_properties(filterbank_utils_static
  PROPERTIES LINKER_LANGUAGE CXX
)

add_subdirectory(tests)
