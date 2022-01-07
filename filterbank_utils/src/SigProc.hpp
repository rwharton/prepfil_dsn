/*
 * SigProc.hpp
 *
 *  Created on: Aug 8, 2016
 *      Author: jlippuner
 */

#ifndef SIGPROC_HPP_
#define SIGPROC_HPP_

#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

#include "SigProcHeader.hpp"

class SigProc {
public:
  // open a file
  SigProc(const std::string& filename);

  // create a new file
  SigProc(const std::string& filename, const SigProcHeader& header);

  ~SigProc()  noexcept(false);

  const SigProcHeader& Header() const {
    return mHeader;
  }

  size_t HeaderSize() const {
    return mHeaderSize;
  }

  void HardFlush();

  void GetChannels(float * const data, const size_t if_idx,
      const size_t first_channel_idx, const size_t num_channels,
      const bool print_progress = false) const {
    if (print_progress)
      DoGetChannels<true>(data, if_idx, first_channel_idx, num_channels);
    else
      DoGetChannels<false>(data, if_idx, first_channel_idx, num_channels);
  }

  void GetChannels(float * const data, const size_t first_channel_idx,
      const size_t num_channels, const bool print_progress = false) const {
    GetChannels(data, 0, first_channel_idx, num_channels, print_progress);
  }

  std::vector<float> GetChannels(const size_t if_idx,
      const size_t first_channel_idx, const size_t num_channels,
      const bool print_progress = false) const {
    std::vector<float> res((size_t)mHeader.nsamples * num_channels);
    GetChannels(res.data(), if_idx, first_channel_idx, num_channels,
        print_progress);
    return res;
  }

  std::vector<float> GetChannels(const size_t first_channel_idx,
      const size_t num_channels, const bool print_progress = false) const {
    return GetChannels((size_t)0, first_channel_idx, num_channels,
        print_progress);
  }

  void GetChannel(float * const data, const size_t if_idx,
      const size_t channel_idx, const bool print_progress = false) const {
    GetChannels(data, if_idx, channel_idx, 1, print_progress);
  }

  void GetChannel(float * const data, const size_t channel_idx,
      const bool print_progress = false) const {
    GetChannels(data, 0, channel_idx, 1, print_progress);
  }

  std::vector<float> GetChannel(const size_t if_idx, const size_t channel_idx,
      const bool print_progress = false) const {
    return GetChannels(if_idx, channel_idx, 1, print_progress);
  }

  std::vector<float> GetChannel(const size_t channel_idx,
      const bool print_progress = false) const {
    return GetChannel((size_t)0, channel_idx, print_progress);
  }

  void SetChannels(const size_t if_idx, const size_t first_channel_idx,
      const float * const data, const size_t num_channels,
      const bool print_progress = false) {
    if (print_progress)
      DoSetChannels<true>(if_idx, first_channel_idx, data, num_channels);
    else
      DoSetChannels<false>(if_idx, first_channel_idx, data, num_channels);
  }

  void SetChannels(const size_t first_channel_idx, const float * const data,
      const size_t num_channels, const bool print_progress = false) {
    SetChannels(0, first_channel_idx, data, num_channels, print_progress);
  }

  void SetChannels(const size_t if_idx, const size_t first_channel_idx,
      const std::vector<float>& data, const bool print_progress = false) {
    size_t num_channels = data.size() / mHeader.nsamples;
    if (data.size() != num_channels * mHeader.nsamples)
      throw std::invalid_argument("got channel data that is not a multiple "
          "of nsamples");

    SetChannels(if_idx, first_channel_idx, data.data(), num_channels,
        print_progress);
  }

  void SetChannels(const size_t first_channel_idx,
      const std::vector<float>& data, const bool print_progress = false) {
    SetChannels(0, first_channel_idx, data, print_progress);
  }

  void SetChannel(const size_t if_idx, const size_t channel_idx,
      const float * const data, const bool print_progress = false) {
    SetChannels(if_idx, channel_idx, data, 1, print_progress);
  }

  void SetChannel(const size_t channel_idx, const float * const data,
      const bool print_progress = false) {
    SetChannels(0, channel_idx, data, 1, print_progress);
  }

  void SetChannel(const size_t if_idx, const size_t channel_idx,
      const std::vector<float>& data, const bool print_progress = false) {
    SetChannels(if_idx, channel_idx, data, print_progress);
  }

  void SetChannel(const size_t channel_idx, const std::vector<float>& data,
      const bool print_progress = false) {
    SetChannel(0, channel_idx, data, print_progress);
  }

  void GetData(float * const data) const;

  std::vector<float> GetData() const {
    size_t num_elements = (size_t)mHeader.nifs * (size_t)mHeader.nchans
        * (size_t)mHeader.nsamples;
    std::vector<float> data(num_elements);
    GetData(data.data());
    return data;
  }

  int FD() const {
    return mFD;
  }

  void SetData(const float * const data);

  void SetData(const std::vector<float>& data) {
    size_t num_elements = (size_t)mHeader.nifs * (size_t)mHeader.nchans
        * (size_t)mHeader.nsamples;

    if (data.size() != num_elements)
      throw std::invalid_argument("Called SetData with invalid data size");
    SetData(data.data());
  }

private:
  template<bool PRINT>
  void DoGetChannels(float * const data, const size_t if_idx,
      const size_t first_channel_idx, const size_t num_channels) const;

  template<bool PRINT>
  void DoSetChannels(const size_t if_idx, const size_t first_channel_idx,
      const float * const data, const size_t num_channels);

  SigProc(const SigProcHeader header, const int fd) :
    mHeader(header),
    mHeaderSize(0),
    mFD(fd),
    mReadOnly(true) {}

  SigProcHeader mHeader;
  size_t mHeaderSize;

  int mFD; // file descriptor for I/O
  bool mReadOnly;
};

#endif // SIGPROC_HPP_
