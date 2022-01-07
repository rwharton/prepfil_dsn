/*
 * BaselineRemover_CPU.cpp
 *
 *  Created on: Aug 17, 2016
 *      Author: jlippuner
 */

#include "BaselineRemover.hpp"

#include <cmath>
#include <cstring>
#include <stdexcept>

#include <fftw3.h>

struct CPU_Impl : public Impl {
  CPU_Impl(const size_t N_pad, const size_t out_size) {
    dat_real = (float*)fftw_malloc(out_size);
    dat_complex = (fftwf_complex*)dat_real;

    plan_r2c = fftwf_plan_dft_r2c_1d(N_pad, dat_real, dat_complex,
        FFTW_ESTIMATE);
    plan_c2r = fftwf_plan_dft_c2r_1d(N_pad, dat_complex, dat_real,
        FFTW_ESTIMATE);
  }

  ~CPU_Impl() {
    fftwf_destroy_plan(plan_r2c);
    fftwf_destroy_plan(plan_c2r);
    fftw_free(dat_real);
  }

  float * dat_real;
  fftwf_complex * dat_complex;
  fftwf_plan plan_r2c, plan_c2r;
};

bool BaselineRemover::CPU_Available() {
  return true;
}

void BaselineRemover::CPU_Init(const size_t /*total_num_channels*/) {
  mpImpl = new CPU_Impl(mN_pad, mOut_size);
}

void BaselineRemover::CPU_Process_batch(float * const data,
    const size_t num_channels) {
  auto impl = dynamic_cast<CPU_Impl*>(mpImpl);
  if (impl == nullptr)
    throw std::runtime_error("Could not cast mpImpl to CPU_Impl");

  for (size_t c = 0; c < num_channels; ++c) {
    // copy data and subtract mean
    memset(impl->dat_real, 0, mOut_size);
    memcpy(impl->dat_real, data + c * mN, mN * sizeof(float));

    double sum = 0.0;
    for (size_t i = 0; i < mN; ++i)
      sum += impl->dat_real[i];

    double mean = sum / (double)mN;

    for (size_t i = 0; i < mN; ++i)
      impl->dat_real[i] -= mean;

    fftwf_execute(impl->plan_r2c);

    // apply high pass filter
    for (size_t i = 0; i < mNum_out; ++i) {
      double f = (double)i * mDf;
      double mult = mDiv * 0.5 * (tanh(2.0 * (f - mF_cutoff)) + 1.0);

      impl->dat_complex[i][0] *= mult;
      impl->dat_complex[i][1] *= mult;
    }

    fftwf_execute(impl->plan_c2r);

    memcpy(data + c * mN, impl->dat_real, mN * sizeof(float));
  }
}

size_t BaselineRemover::CPU_Ram_per_channel() const {
  return 0;
}
