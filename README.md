# prepfil_dsn

(Based on build + instructions from Aaron Pearlman)

Installation instructions:

REQUIREMENTS

- C++11-capable C++ compiler
- CMake (>= 2.8.8, https://cmake.org/download/)
- Google Protobuf (>= 2.0, https://github.com/google/protobuf)
- Boost (>= 1.32, only need Filesystem and Regex components,
  http://www.boost.org/users/download/)
- CUDA (optional, https://developer.nvidia.com/cuda-downloads)
- FFTW (required if CUDA is not available, http://www.fftw.org/download.html)
- OpenMP (optional, http://openmp.org/)
- tempo

Installation Instructions:

1. git clone https://github.com/rwharton/prepfil_dsn.git
2. cd prepfil_dsn/filterbank_utils/

3. You might need to update the GPU architecture.  Check in prepfil_dsn/filterbank_utils/CMakeLists.txt (Line 44):

set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_52,code=sm_52)

4. mkdir build/
5. cd build/
6. cmake -DCMAKE_INSTALL_PREFIX=/path/to/prepfil_dsn/filterbank_utils/install ..
7. make -j8
8. make test
9. make install
