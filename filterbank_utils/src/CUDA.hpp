/*
 * CUDA.hpp
 *
 *  Created on: Jul 6, 2016
 *      Author: jlippuner
 */

#ifndef CUDA_HPP_
#define CUDA_HPP_

#include <string>

#include <cuda_runtime.h>
#include <cufft.h>

#define CUCHK(ans) { cuda_assert((ans), __FILE__, __LINE__); }

inline void cuda_assert(cudaError_t code, const char * const file,
    const int line, const bool abort = true) {
  if (code != cudaSuccess) {
    fprintf(stderr, "CUDA ERROR %i (%s) at %s:%d\n", code,
        cudaGetErrorString(code), file, line);
    if (abort)
      exit(code);
  }
}

#define CUFFTCHK(ans) { cufft_assert((ans), __FILE__, __LINE__); }

inline void cufft_assert(cufftResult code, const char * const file,
    const int line, const bool abort = true) {
  if (code != CUFFT_SUCCESS) {
    std::string error = "";

    switch (code) {
    case CUFFT_INVALID_PLAN:
      error = "cuFFT was passed an invalid plan handle";
      break;
    case CUFFT_ALLOC_FAILED:
      error = "cuFFT failed to allocate GPU or CPU memory";
      break;
    case CUFFT_INVALID_TYPE:
      error = "Deprecated error CUFFT_INVALID_TYPE";
      break;
    case CUFFT_INVALID_VALUE:
      error = "User specified an invalid pointer or parameter";
      break;
    case CUFFT_INTERNAL_ERROR:
      error = "Driver or internal cuFFT library error";
      break;
    case CUFFT_EXEC_FAILED:
      error = "Failed to execute an FFT on the GPU";
      break;
    case CUFFT_SETUP_FAILED:
      error = "The cuFFT library failed to initialize";
      break;
    case CUFFT_INVALID_SIZE:
      error = "User specified an invalid transform size";
      break;
    case CUFFT_UNALIGNED_DATA:
      error = "Deprecated error CUFFT_UNALIGNED_DATA";
      break;
    case CUFFT_INCOMPLETE_PARAMETER_LIST:
      error = "Missing parameters in call";
      break;
    case CUFFT_INVALID_DEVICE:
      error = "Execution of a plan was on different GPU than plan creation";
      break;
    case CUFFT_PARSE_ERROR:
      error = "Internal plan database error";
      break;
    case CUFFT_NO_WORKSPACE:
      error = "No workspace has been provided prior to plan execution";
      break;
    default:
      error = "Unknown CUFFT error";
    }

    fprintf(stderr, "CUFFT ERROR %i (%s) at %s:%d\n", code, error.c_str(), file,
        line);
    if (abort)
      exit(code);
  }
}


#endif // CUDA_HPP_
