/*
 * SigProc.cpp
 *
 *  Created on: Aug 8, 2016
 *      Author: jlippuner
 */

#include "SigProc.hpp"

#include <chrono>
#include <cmath>
#include <fstream>
#include <stdexcept>

// POSIX
#ifndef _LARGEFILE64_SOURCE
  #define _LARGEFILE64_SOURCE 1
#endif
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "utils.hpp"

SigProc::SigProc(const std::string& filename) {
  mReadOnly = false;
  errno = 0;

  mFD = open64(filename.c_str(), O_RDWR);

  if (mFD == -1) {
    errno = 0;
    // try open read-only
    mReadOnly = true;
    mFD = open64(filename.c_str(), O_RDONLY);
  }

  if (mFD == -1) {
    perror("Failure in SigProc::SigProc(const std::string&)");
    throw std::runtime_error("Failed to open file '" + filename + "'");
  }

  mHeader = SigProcHeader::Read(mFD);
  mHeaderSize = mHeader.Input_size;

  if (errno != 0) {
    perror("Failure in SigProc::SigProc(const std::string&)");
    throw std::runtime_error(
        "Failed to read header from file '" + filename + "'");
  }

  if (mHeader.nbits != 32)
    throw std::invalid_argument("Can only read 32-bit sigproc files");

  // check file size
  off64_t seek = lseek64(mFD, 0, SEEK_END);
  if (seek < 0) {
    perror("Failure in SigProc::SigProc(const std::string&)");
    throw std::runtime_error("Failed to seek end of '" + filename + "'");
  }

  size_t file_size = seek;
  seek = lseek64(mFD, mHeaderSize, SEEK_SET);
  if (seek < 0) {
    perror("Failure in SigProc::SigProc(const std::string&)");
    throw std::runtime_error("Failed to seek end of header in '"
        + filename + "'");
  }

  if (mHeader.nsamples <= 0) {
    size_t data_size = file_size - mHeaderSize;
    mHeader.nsamples = data_size / (size_t)mHeader.nifs / (size_t)mHeader.nchans
        / (size_t)(mHeader.nbits / 8);
  }

  size_t data_size = mHeader.Data_size();

  if (file_size > (data_size + mHeaderSize)) {
    printf("WARNING: The sigproc file '%s' has more data than it reports\n",
        filename.c_str());
  } else if (file_size < (data_size + mHeaderSize)) {
    throw std::invalid_argument("The sigproc file '" + filename + "' has less "
        "data than it reports");
  }
}

SigProc::SigProc(const std::string& filename, const SigProcHeader& header) :
  mHeader(header),
  mHeaderSize(header.Get_output_size()) {
  mReadOnly = false;
  errno = 0;

  if (mHeader.nbits != 32)
    throw std::invalid_argument("Can only write 32-bit sigproc files");

  if (mHeader.Data_size() == 0) {
    // we don't know the file size, so create a new file overwriting any
    // existing file
    mFD = open64(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC,
          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  } else {
    // we know the file size, allocate the entire file
    size_t file_size = mHeaderSize + mHeader.Data_size();

    // allocate file
    FILE * f = fopen(filename.c_str(), "wb");

    if (f == nullptr)
      throw std::runtime_error("Failed to open file '" + filename + "'");

    if (fseek(f, file_size - 1, SEEK_SET) != 0)
      throw std::runtime_error("Failed to allocate file '" + filename + "'");

    if (fputc(0, f) != 0)
      throw std::runtime_error("Failed to allocate file '" + filename + "'");

    if (fclose(f) != 0)
      throw std::runtime_error("Failed to close file '" + filename + "'");

    // open file as fstream
    mFD = open64(filename.c_str(), O_RDWR);
  }

  if (mFD == -1) {
      perror("Failure in SigProc::SigProc(const std::string&, "
          "const SigProcHeader&)");
      throw std::runtime_error("Failed to open file '" + filename + "'");
    }

  mHeader.Write(mFD);

  if (errno != 0) {
    perror("Failure in SigProc::SigProc(const std::string&,"
        "const SigProcHeader&)");
    throw std::runtime_error(
        "Failed to write header to file '" + filename + "'");
  }
}

SigProc::~SigProc() noexcept(false) {
  HardFlush();

  if (close(mFD) == -1) {
    perror("Failure in SigProc::~SigProc()");
    throw std::runtime_error("Failed to close file");
  }
}

void SigProc::HardFlush() {
  if (fsync(mFD) != 0) {
    perror("Failure in HardFlush");
    throw std::runtime_error("Failed to flush SigProc file");
  }
}

void SigProc::GetData(float * const data) const {
  size_t num_elements = (size_t)mHeader.nifs * (size_t)mHeader.nchans
      * (size_t)mHeader.nsamples;
  if ((size_t)lseek64(mFD, mHeaderSize, SEEK_SET) != mHeaderSize) {
    perror("Failure in SigProc::GetData");
    throw std::runtime_error("Failed to seek");
  }

  size_t nbytes = num_elements * (size_t)(mHeader.nbits / 8);
  if ((size_t)read(mFD, data, nbytes) != nbytes) {
    perror("Failure in SigProc::GetData");
    throw std::runtime_error("Failed to read");
  }
}

void SigProc::SetData(const float * const data) {
  if (mReadOnly)
    throw std::runtime_error("Cannot set data on a read-only sigproc file");

  size_t num_elements = (size_t)mHeader.nifs * (size_t)mHeader.nchans
      * (size_t)mHeader.nsamples;

  if ((size_t)lseek64(mFD, mHeaderSize, SEEK_SET) != mHeaderSize) {
    perror("Failure in SigProc::SetData");
    throw std::runtime_error("Failed to seek");
  }

  size_t nbytes = num_elements * (size_t)(mHeader.nbits / 8);
  if ((size_t)write(mFD, data, nbytes) != nbytes) {
    perror("Failure in SigProc::SetData");
    throw std::runtime_error("Failed to write");
  }
}

template<bool PRINT>
void SigProc::DoGetChannels(float * const data, const size_t if_idx,
    const size_t first_channel_idx, const size_t num_channels) const {
  int prev_prog = 0;

  if (PRINT) {
    printf("%2i%%", prev_prog);
    fflush(stdout);
  }

  if ((int)if_idx >= mHeader.nifs)
    throw std::out_of_range("if_idx is out of range");

  if ((int)(first_channel_idx + num_channels) > mHeader.nchans)
    throw std::out_of_range("Requested channels out of range");

  if (num_channels == 0) {
    if (PRINT)
      printf("\b\b\bdone (nothing read)");
    return;
  }

  for (size_t t = 0; t < (size_t)mHeader.nsamples; ++t) {
    std::vector<float> sample(num_channels);

    size_t offset = t * mHeader.nifs * mHeader.nchans + if_idx * mHeader.nchans
        + first_channel_idx;

    off64_t off = offset * (size_t)(mHeader.nbits / 8) + mHeaderSize;
    if (lseek64(mFD, off, SEEK_SET) != off) {
      perror("Failure in SigProc::DoGetChannels");
      throw std::runtime_error("Failed to seek");
    }

    size_t nbytes = num_channels * sizeof(float);
    if ((size_t)read(mFD, sample.data(), nbytes) != nbytes) {
      perror("Failure in SigProc::DoGetChannels");
      throw std::runtime_error("Failed to read");
    }

    for (size_t i = 0; i < num_channels; ++i) {
      data[i * mHeader.nsamples + t] = sample[i];
    }

    if (PRINT) {
      int prog = (int)(100.0 * (double)(t + 1) / (double)mHeader.nsamples);
      prog = std::min(99, prog);
      if (prog != prev_prog) {
        printf("\b\b\b%2i%%", prog);
        fflush(stdout);
        prev_prog = prog;
      }
    }
  }

  if (errno != 0) {
    perror("Failure in SigProc::DoGetChannels");
    throw std::runtime_error("An error occurred while reading channel from "
        "sigproc file");
  }

  if (PRINT)
    printf("\b\b\bdone");
}

template<bool PRINT>
void SigProc::DoSetChannels(const size_t if_idx, const size_t first_channel_idx,
    const float * const data, const size_t num_channels) {
  int prev_prog = 0;

  if (PRINT) {
    printf("%2i%%", prev_prog);
    fflush(stdout);
  }

  if (mReadOnly)
    throw std::runtime_error("Cannot set channels on a read-only sigproc file");

  if (num_channels == 0) {
    if (PRINT)
      printf("\b\b\bdone (nothing written)");
    return;
  }

  if ((int)if_idx >= mHeader.nifs)
    throw std::out_of_range("if_idx is out of range");

  if ((int)(first_channel_idx + num_channels) > mHeader.nchans)
    throw std::out_of_range("Requested channels out of range");

  for (size_t t = 0; t < (size_t)mHeader.nsamples; ++t) {
    std::vector<float> sample(num_channels);

    for (size_t i = 0; i < num_channels; ++i) {
      sample[i] = data[i * mHeader.nsamples + t];
    }

    size_t offset = t * mHeader.nifs * mHeader.nchans + if_idx * mHeader.nchans
        + first_channel_idx;

    off64_t off = offset * (size_t)(mHeader.nbits / 8) + mHeaderSize;
    if (lseek64(mFD, off, SEEK_SET) != off) {
      perror("Failure in SigProc::DoSetChannels");
      throw std::runtime_error("Failed to seek");
    }

    size_t nbytes = num_channels * sizeof(float);
    if ((size_t)write(mFD, sample.data(), nbytes) != nbytes) {
      perror("Failure in SigProc::DoSetChannels");
      throw std::runtime_error("Failed to write");
    }

    if (PRINT) {
      int prog = (int)(100.0 * (double)(t + 1) / (double)mHeader.nsamples);
      prog = std::min(99, prog);
      if (prog != prev_prog) {
        printf("\b\b\b%2i%%", prog);
        fflush(stdout);
        prev_prog = prog;
      }
    }
  }

  if (errno != 0) {
    perror("Failure in SigProc::DoSetChannels");
    throw std::runtime_error("An error occurred while writing channel from "
        "sigproc file");
  }

  if (PRINT)
    printf("\b\b\bdone");
}

// explicit template instantiations
template void SigProc::DoGetChannels<true>(float * const, const size_t,
    const size_t, const size_t) const;

template void SigProc::DoGetChannels<false>(float * const, const size_t,
    const size_t, const size_t) const;

template void SigProc::DoSetChannels<true>(const size_t, const size_t,
    const float * const, const size_t);

template void SigProc::DoSetChannels<false>(const size_t, const size_t,
    const float * const, const size_t);
