/*
 * BaselineRemover_GPU.cpp
 *
 *  Created on: Aug 17, 2016
 *      Author: jlippuner
 */

#include "BaselineRemover.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <thrust/device_ptr.h>
#include <thrust/functional.h>
#include <thrust/reduce.h>
#include <thrust/system/cuda/execution_policy.h>
#pragma GCC diagnostic pop

#include "CUDA.hpp"
#include "utils.hpp"

struct high_pass_functor {
  high_pass_functor(const float df, const float f_cutoff, const float div) :
      _df(df),
      _f_cutoff(f_cutoff),
      _div(div) {
  }

  __device__ cufftComplex operator()(const cufftComplex val, const size_t idx) {
    float f = (float)idx * _df;
    float mult = _div * 0.5 * (tanhf(2.0 * (f - _f_cutoff)) + 1.0);

    cufftComplex res = val;
    res.x = val.x * mult;
    res.y = val.y * mult;
    return res;
  }

  const float _df;
  const float _f_cutoff;
  const float _div;
};

struct GPU_Impl : public Impl {
  GPU_Impl(const size_t stream_batch_size, const size_t N_pad, const size_t num_out,
      const size_t out_size, const int real_stride, const int complex_stride) :
      Stream_batch_size(stream_batch_size),
      d_data(nullptr),
      d_real(new cufftReal*[Stream_batch_size]),
      d_complex(new cufftComplex*[Stream_batch_size]),
      stream(new cudaStream_t[Stream_batch_size]),
      plan_r2c(new cufftHandle[Stream_batch_size]),
      plan_c2r(new cufftHandle[Stream_batch_size]) {
    CUCHK(cudaMalloc(&d_data, Stream_batch_size * out_size));

    int N_pad_int = (int)N_pad;
    int num_out_int = (int)num_out;

    for (size_t s = 0; s < Stream_batch_size; ++s) {
      d_real[s] = d_data + s * real_stride;
      d_complex[s] = (cufftComplex*)d_real[s];

      CUCHK(cudaStreamCreate(stream + s));
      CUFFTCHK(cufftPlanMany(plan_r2c + s, 1, &N_pad_int, &N_pad_int, 1,
          real_stride, &num_out_int, 1, complex_stride, CUFFT_R2C, 1));
      CUFFTCHK(cufftPlanMany(plan_c2r + s, 1, &N_pad_int, &num_out_int, 1,
          complex_stride, &N_pad_int, 1, real_stride, CUFFT_C2R, 1));

      CUFFTCHK(cufftSetStream(plan_r2c[s], stream[s]));
      CUFFTCHK(cufftSetStream(plan_c2r[s], stream[s]));
    }
  }

  ~GPU_Impl() {
    CUCHK(cudaFree(d_data));

    for (size_t s = 0; s < Stream_batch_size; ++s) {
      CUFFTCHK(cufftDestroy(plan_r2c[s]));
      CUFFTCHK(cufftDestroy(plan_c2r[s]));
      CUCHK(cudaStreamDestroy(stream[s]));
    }

    delete [] d_real;
    delete [] d_complex;
    delete [] stream;
    delete [] plan_r2c;
    delete [] plan_c2r;
  }

  size_t Stream_batch_size;

  cufftReal * d_data;
  cufftReal ** d_real;
  cufftComplex ** d_complex;

  cudaStream_t * stream;

  cufftHandle * plan_r2c;
  cufftHandle * plan_c2r;
};

bool BaselineRemover::GPU_Available() {
  return true;
}

void BaselineRemover::GPU_Init(const size_t total_num_channels) {
  // round up to multiple of 256 bytes
  mOut_size = 256 * ((mOut_size + 255) / 256);

  int real_stride = mOut_size / sizeof(cufftReal);
  int complex_stride = mOut_size / sizeof(cufftComplex);

  // figure out who many channels we can do in parallel
  size_t free_mem = 0;
  size_t total_mem = 0;
  CUCHK(cudaMemGetInfo(&free_mem, &total_mem));

  size_t work_size_r2c, work_size_c2r;
  int N_pad_int = (int)mN_pad;
  int num_out_int = (int)mNum_out;

  CUFFTCHK(cufftEstimateMany(1, &N_pad_int, &N_pad_int, 1, real_stride,
      &num_out_int, 1, complex_stride, CUFFT_R2C, 1, &work_size_r2c));

  CUFFTCHK(cufftEstimateMany(1, &N_pad_int, &num_out_int, 1, complex_stride,
      &N_pad_int, 1, real_stride, CUFFT_C2R, 1, &work_size_c2r));

  size_t total_size = mOut_size + work_size_r2c + work_size_c2r;
  size_t stream_batch_size = 0.95 * (double)free_mem / (double)total_size;

  stream_batch_size = std::min(stream_batch_size, total_num_channels);
  stream_batch_size = std::min(stream_batch_size, (size_t)32);

  if (stream_batch_size <= 0)
    throw std::runtime_error("Not enough memory for SigProc::RemoveBaseline");

  mpImpl = new GPU_Impl(stream_batch_size, mN_pad, mNum_out, mOut_size,
      real_stride, complex_stride);
}

void BaselineRemover::GPU_Process_batch(float * const data,
    const size_t num_channels) {
  auto impl = dynamic_cast<GPU_Impl*>(mpImpl);
  if (impl == nullptr)
    throw std::runtime_error("Could not cast mpImpl to GPU_Impl");

  size_t num_stream_batches = (num_channels + impl->Stream_batch_size - 1)
      / impl->Stream_batch_size;

  for (size_t i = 0; i < num_stream_batches; ++i) {
    size_t num_streams = std::min(impl->Stream_batch_size,
        num_channels - i * impl->Stream_batch_size);

    for (size_t s = 0; s < num_streams; ++s) {
      size_t channel_idx = i * impl->Stream_batch_size + s;

      // copy data to GPU
      CUCHK(cudaMemsetAsync(impl->d_real[s], 0, mOut_size,
          impl->stream[s]));
      CUCHK(cudaMemcpyAsync(impl->d_real[s], data + channel_idx * mN,
          mN * sizeof(float), cudaMemcpyHostToDevice, impl->stream[s]));

      // subtract mean of signal
      thrust::device_ptr<float> signal(impl->d_real[s]);
      float sum = thrust::reduce(thrust::cuda::par.on(impl->stream[s]),
          signal, signal + mN);
      float mean = sum / (float)mN;
      thrust::for_each(thrust::cuda::par.on(impl->stream[s]), signal,
          signal + mN, thrust::placeholders::_1 -= mean);

      // do FFT
      CUFFTCHK(cufftExecR2C(impl->plan_r2c[s], impl->d_real[s],
          impl->d_complex[s]));

      // apply high pass filter
      thrust::device_ptr<cufftComplex> spectrum(impl->d_complex[s]);
      thrust::counting_iterator<size_t> cnt(0);
      thrust::transform(thrust::cuda::par.on(impl->stream[s]), spectrum,
          spectrum + mNum_out, cnt, spectrum,
          high_pass_functor(mDf, mF_cutoff, mDiv));

      CUFFTCHK(cufftExecC2R(impl->plan_c2r[s], impl->d_complex[s],
          impl->d_real[s]));

      // copy data back
      CUCHK(cudaMemcpyAsync(data + channel_idx * mN, impl->d_real[s],
          mN * sizeof(float), cudaMemcpyDeviceToHost, impl->stream[s]));
    }
  }

  for (size_t s = 0; s < impl->Stream_batch_size; ++s) {
    CUCHK(cudaStreamSynchronize(impl->stream[s]));
  }
}

size_t BaselineRemover::GPU_Ram_per_channel() const {
  return 0;
}
