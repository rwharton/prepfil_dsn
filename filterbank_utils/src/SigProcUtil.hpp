/*
 * SigProcUtil.hpp
 *
 *  Created on: Aug 16, 2016
 *      Author: jlippuner
 */

#ifndef SIGPROCUTIL_HPP_
#define SIGPROCUTIL_HPP_

#include <memory>

#include "SigProc.hpp"
#include "RFIMask.hpp"

class SigProcUtil {
public:
  SigProcUtil(bool useGPU = true) :
      mMaxAbsoluteMemKB(0),
      mMaxFracMem(0.0),
      mUseGPU(useGPU) {
  }

  SigProcUtil(const size_t maxAbsoluteMem_kB, bool useGPU = true) :
      mMaxAbsoluteMemKB(maxAbsoluteMem_kB),
      mMaxFracMem(0.0),
      mUseGPU(useGPU) {
  }

  SigProcUtil(const double maxFracMem, bool useGPU = true) :
      mMaxAbsoluteMemKB(0),
      mMaxFracMem(maxFracMem),
      mUseGPU(useGPU) {
    SetFractionalMemLimit(maxFracMem);
  }

  SigProcUtil(const size_t maxAbsoluteMem_kB, const double maxFracMem,
      bool useGPU = true) :
      mMaxAbsoluteMemKB(maxAbsoluteMem_kB),
      mMaxFracMem(maxFracMem),
      mUseGPU(useGPU) {
    SetFractionalMemLimit(maxFracMem);
  }

  void SetAbsoluteMemLimit_kB(const size_t maxAbsoluteMem_kB) {
    mMaxAbsoluteMemKB = maxAbsoluteMem_kB;
  }

  void SetFractionalMemLimit(const double maxFracMem) {
    if ((maxFracMem < 0.0) || (maxFracMem > 1.0))
      throw std::invalid_argument("maxFracMem must be between 0.0 and 1.0");

    mMaxFracMem = maxFracMem;
  }

  void SetUseGPU(const bool useGPU) {
    mUseGPU = useGPU;
  }

  void SetMask(const RFIMask& mask) {
    mpMask = std::unique_ptr<RFIMask>(new RFIMask(mask));
  }

  void Meminfo(size_t * const total_kB, size_t * const available_kB) const;

  void ModifyHeader(const std::string& input_file,
      const std::string& output_file, const SigProcHeader newHeader) const;

  void AverageSamples(const SigProc& input, const std::string& output,
      const int num_samples_to_average) const {
    Process(input, output, num_samples_to_average, 0, 0.0, 0.0, "");
  }

  void CorrectBandpass(const SigProc& input, const std::string& output,
      const int num_samples_to_estimate_bandpass,
      const double bandpass_smoothing_parameter) const {
    Process(input, output, 1, num_samples_to_estimate_bandpass,
        bandpass_smoothing_parameter, 0.0, "");
  }

  void RemoveBaseline(const SigProc& input, const std::string& output,
      const double baseline_length_in_sec) const {
    Process(input, output, 1, 0, 0.0, baseline_length_in_sec, "");
  }

  void BarycenterCorrect(const SigProc& input, const std::string& output,
      const std::string observatoryCodeForBarycentering) const {
    Process(input, output, 1, 0, 0.0, 0.0, observatoryCodeForBarycentering);
  }

  void Process(const SigProc& input, const std::string& output,
      const int num_samples_to_average,
      const int num_samples_to_estimate_bandpass,
      const double bandpass_smoothing_parameter,
      const double baseline_length_in_sec,
      const std::string observatoryCodeForBarycentering) const {
    DoProcess0(input, output, num_samples_to_average,
        num_samples_to_estimate_bandpass, bandpass_smoothing_parameter,
        baseline_length_in_sec, observatoryCodeForBarycentering);
  }

private:
  size_t BufferSize() const;

  void GetBatches(const size_t samples_per_channel, const size_t num_channels,
      size_t * const batch_size, size_t * const num_concurrent_batches) const;

  void DoProcess0(const SigProc& input, const std::string& output,
      const int num_avg, const int num_bp, const double bp_smooth,
      const double base, const std::string obs) const {
    if (num_avg == 1)
      DoProcess1<false>(input, output, num_avg, num_bp, bp_smooth, base, obs);
    else
      DoProcess1<true>(input, output, num_avg, num_bp, bp_smooth, base, obs);
  }

  template<bool do_avg>
  void DoProcess1(const SigProc& input, const std::string& output,
      const int num_avg, const int num_bp, const double bp_smooth,
      const double base, const std::string obs) const {
    if (num_bp == 0)
      DoProcess2<do_avg, false>(input, output, num_avg, num_bp, bp_smooth,
          base, obs);
    else
      DoProcess2<do_avg, true>(input, output, num_avg, num_bp, bp_smooth,
          base, obs);
  }

  template<bool do_avg, bool do_bp>
  void DoProcess2(const SigProc& input, const std::string& output,
      const int num_avg, const int num_bp, const double bp_smooth,
      const double base, const std::string obs) const {
    if (base == 0.0)
      DoProcess3<do_avg, do_bp, false>(input, output, num_avg, num_bp,
          bp_smooth, base, obs);
    else
      DoProcess3<do_avg, do_bp, true>(input, output, num_avg, num_bp, bp_smooth,
          base, obs);
  }

  template<bool do_avg, bool do_bp, bool do_base>
  void DoProcess3(const SigProc& input, const std::string& output,
      const int num_avg, const int num_bp, const double bp_smooth,
      const double base, const std::string obs) const {
    if (obs == "")
      DoProcess<do_avg, do_bp, do_base, false>(input, output, num_avg, num_bp,
          bp_smooth, base, obs);
    else
      DoProcess<do_avg, do_bp, do_base, true>(input, output, num_avg, num_bp,
          bp_smooth, base, obs);
  }

  template<bool do_avg, bool do_bp, bool do_base, bool do_bary>
  void DoProcess(const SigProc& input, const std::string& output,
      const int num_avg, const int num_bp, const double bp_smooth,
      const double base, const std::string obs) const;

  size_t mMaxAbsoluteMemKB;
  double mMaxFracMem;
  bool mUseGPU;

  std::unique_ptr<RFIMask> mpMask;
};

#endif // SIGPROCUTIL_HPP_
