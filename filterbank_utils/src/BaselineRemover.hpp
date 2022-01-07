/*
 * BaselineRemover.hpp
 *
 *  Created on: Aug 17, 2016
 *      Author: jlippuner
 */

#ifndef BASELINEREMOVER_HPP_
#define BASELINEREMOVER_HPP_

#include <cstddef>

#include "utils.hpp"

struct Impl {
  virtual ~Impl() {}
};

class BaselineRemover {
public:
  BaselineRemover(const size_t num_samples, const size_t total_num_channels,
      const double tsamp_in_sec, const double baseline_length_in_sec,
      const bool useGPU) :
      mN(num_samples),
      mpImpl(nullptr),
      mUseGPU(useGPU) {
    mN_pad= next_larger_pow2(mN);
    mNum_out = mN_pad / 2 + 1;
    mOut_size = mNum_out * (size_t)(2 * sizeof(float));

    mDf = 1.0 / ((double)mN_pad * tsamp_in_sec);
    mF_cutoff = 1.0 / baseline_length_in_sec;
    mDiv = 1.0 / (double)mN_pad;

    if (!CPU_Available() || (GPU_Available() && mUseGPU))
      GPU_Init(total_num_channels);
    else
      CPU_Init(total_num_channels);
  }

  ~BaselineRemover() {
    if (mpImpl != nullptr)
      delete mpImpl;
  }

  void GPU_Init(const size_t total_num_channels);
  void CPU_Init(const size_t total_num_channels);

  void GPU_Process_batch(float * const data,const size_t num_channels);
  void CPU_Process_batch(float * const data,const size_t num_channels);

  void Process_batch(float * const data, const size_t num_channels) {
    if (!CPU_Available() || (GPU_Available() && mUseGPU))
      GPU_Process_batch(data, num_channels);
    else
      CPU_Process_batch(data, num_channels);
  }

  size_t GPU_Ram_per_channel() const;
  size_t CPU_Ram_per_channel() const;

  size_t Ram_per_channel() const {
    if (!CPU_Available() || (GPU_Available() && mUseGPU))
      return GPU_Ram_per_channel();
    else
      return CPU_Ram_per_channel();
  }

  static bool GPU_Available();

  static bool CPU_Available();

private:
  size_t mN, mN_pad, mOut_size, mNum_out;

  double mDf, mF_cutoff, mDiv;

  Impl * mpImpl;

  bool mUseGPU;
};

#endif // BASELINEREMOVER_HPP_
