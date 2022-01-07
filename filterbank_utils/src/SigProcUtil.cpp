/*
 * SigProcUtil.cpp
 *
 *  Created on: Aug 16, 2016
 *      Author: jlippuner
 */

#include "SigProcUtil.hpp"

#include <algorithm>
#include <cstring>
#include <cmath>
#include <set>

#ifndef _LARGEFILE64_SOURCE
  #define _LARGEFILE64_SOURCE 1
#endif
#include <errno.h>
#include <unistd.h>

#include "Barycenter.hpp"
#include "BaselineRemover.hpp"
#include "utils.hpp"

void SigProcUtil::Meminfo(size_t * const total_kB,
    size_t * const available_kB) const {
  // read meminfo
  FILE * fmem = fopen("/proc/meminfo", "r");

  *total_kB = 0;
  *available_kB = 0;

  if (fmem == nullptr)
    return;

  size_t mem_free = 0;
  size_t mem_active_file = 0;
  size_t mem_inactive_file = 0;
  size_t mem_slab_reclaimable = 0;

  char buffer[1024];

  while (fgets(buffer, 1024, fmem) != nullptr) {
    char name[1024];
    size_t num;
    if (sscanf(buffer, "%s %lu", name, &num) != 2)
      continue;

    std::string str(name);
    if (str == "MemTotal:") {
      *total_kB = num;
    } else if (str == "MemAvailable:") {
      *available_kB = num;
    } else if (str == "MemFree:") {
      mem_free = num;
    } else if (str == "Active(file):") {
      mem_active_file = num;
    } else if (str == "Inactive(file):") {
      mem_inactive_file = num;
    } else if (str == "SReclaimable:") {
      mem_slab_reclaimable = num;
    }
  }

  fclose(fmem);

  if (*available_kB > 0)
    return;

  // get watermark_low
  FILE * fmin = fopen("/proc/sys/vm/min_free_kbytes", "r");

  if (fmin == nullptr) {
    *available_kB = mem_free;
    return;
  }

  size_t watermark_low = 0;
  if (fscanf(fmin, "%lu", &watermark_low) != 1) {
    *available_kB = mem_free;
    return;
  }

  fclose(fmin);

  long int avail = mem_free - watermark_low;

  size_t pagecache = mem_active_file + mem_inactive_file;
  pagecache -= std::min(pagecache / 2, watermark_low);

  avail += pagecache;

  avail += mem_slab_reclaimable
      - std::min(mem_slab_reclaimable / 2, watermark_low);

  if (avail < 0)
    avail = mem_free;

  *available_kB = avail;
}

namespace {

void seek(const int fd, off64_t off) {
  if (lseek64(fd, off, SEEK_SET) != off) {
    perror("Failure in seek");
    throw std::runtime_error("Failed to seek");
  }
}

void read_data(const int fd, void * const buf, const size_t len) {
  if ((size_t)read(fd, buf, len) != len) {
    perror("Failure in read_data");
    throw std::runtime_error("Failed to read data");
  }
}

void write_data(const int fd, const void * const buf, const size_t len) {
  if ((size_t)write(fd, buf, len) != len) {
    perror("Failure in write_data");
    throw std::runtime_error("Failed to write data");
  }
}

} // namespace [unnamed]

void SigProcUtil::ModifyHeader(const std::string& input_file,
    const std::string& output_file, const SigProcHeader newHeader) const {
  SigProc in_file(input_file);

  if (in_file.Header().Data_size() != newHeader.Data_size())
    throw std::runtime_error("ModifyHeader cannot modify the data size");

  if (input_file == output_file) {
    // in-place header modification
    if (in_file.HeaderSize() != newHeader.Get_output_size())
      throw std::runtime_error("Cannot modify header in-place because new "
          "header has different size");

    int fd = in_file.FD();
    seek(fd, 0);
    newHeader.Write(fd);
  } else {
    std::string temp_out = output_file + ".in_progress";
    SigProc out_file(temp_out, newHeader);

    int in_fd = in_file.FD();
    seek(in_fd, in_file.HeaderSize());

    int out_fd = out_file.FD();
    seek(out_fd, out_file.HeaderSize());

    size_t bufsize = 16 * 1024 * 1024; // 16 MB
    char * buf = (char*)malloc(bufsize);

    size_t num_chunks = newHeader.Data_size() / bufsize;

    printf("\33[2K\rCopying data... %3i%%", 0);
    fflush(stdout);

    for (size_t i = 0; i < num_chunks; ++i) {
      read_data(in_fd, buf, bufsize);
      write_data(out_fd, buf, bufsize);

      printf("\33[2K\rCopying data... %3i%%",
          (int)(100.0 * (double)(i + 1) / (double)num_chunks));
      fflush(stdout);
    }

    // and leftover part
    size_t leftover = newHeader.Data_size() - num_chunks * bufsize;
    if (leftover > 0) {
      read_data(in_fd, buf, leftover);
      write_data(out_fd, buf, leftover);
    }

    printf("\33[2K\rCopying data... done\n");
    free(buf);

    if (rename(temp_out.c_str(), output_file.c_str()) != 0)
      throw std::runtime_error(
          "Failed to rename file '" + temp_out + "' to '" + output_file + "'");
  }
}

size_t SigProcUtil::BufferSize() const {
  size_t total_kB, avail_kB;
  Meminfo(&total_kB, &avail_kB);
//  printf("Avail: %lu MB\n", avail_kB / 1024);

  size_t buff_kB = 0.8 * (double)avail_kB;

  if (mMaxAbsoluteMemKB > 0)
    buff_kB = std::min(buff_kB, mMaxAbsoluteMemKB);

  if (mMaxFracMem > 0.0)
    buff_kB = std::min(buff_kB, (size_t)(mMaxFracMem * (double)total_kB));

  return buff_kB * 1024;
}

void SigProcUtil::GetBatches(const size_t samples_per_channel,
    const size_t num_channels, size_t * const batch_size,
    size_t * const num_concurrent_batches) const {
  size_t buffer_size = BufferSize();

  *batch_size = buffer_size / (sizeof(float) * samples_per_channel);
  *batch_size = std::min(*batch_size, num_channels);

  // balance the batches
  size_t num_batches = (num_channels + *batch_size - 1) / *batch_size;
  *batch_size = num_channels / num_batches;

  size_t batch_data_size = *batch_size * (sizeof(float) * samples_per_channel);
//  printf("batch_data_size = %lu\n", batch_data_size / 1024l / 1024l);

  *num_concurrent_batches = buffer_size / batch_data_size;
}

template<bool do_avg, bool do_bp, bool do_base, bool do_bary>
void SigProcUtil::DoProcess(const SigProc& input, const std::string& output,
    const int num_samples_to_average,
    const int num_samples_to_estimate_bandpass,
    const double bandpass_smoothing_parameter,
    const double baseline_length_in_sec,
    const std::string observatoryCodeForBarycentering) const {
  if (num_samples_to_average <= 0)
    throw std::invalid_argument("Cannot average a non-positive number "
        "of samples");

  if (num_samples_to_estimate_bandpass < 0)
    throw std::invalid_argument("Cannot estimate bandpass with negative "
        "number of samples");

  if (baseline_length_in_sec < 0.0)
    throw std::invalid_argument("Cannot remove negative length baseline");

  auto header = input.Header();
  int in_n = header.nsamples;

  if (do_avg != (num_samples_to_average > 1))
    throw std::invalid_argument("do_avg and num_samples_to_average don't "
        "agree");
  if (do_bp != (num_samples_to_estimate_bandpass > 0))
    throw std::invalid_argument("do_bp and num_samples_to_estimate_bandpass "
        "don't agree");
  if (do_base != (baseline_length_in_sec > 0.0))
    throw std::invalid_argument("do_base and baseline_length_in_sec don't "
        "agree");
  if (do_bary != (observatoryCodeForBarycentering != ""))
    throw std::invalid_argument("do_bary and observatoryCodeForBarycentering "
        "don't agree");

  int out_n = in_n / num_samples_to_average;

  // set up for bandpass correction
  std::vector<float> bp(header.nchans, 1.0);
  size_t bp_samples = 0;

  if (do_bp) {
    if (num_samples_to_estimate_bandpass < 16384) {
      printf("WARNING: Using small number of samples (%i) to estimate "
          "bandpass\n", num_samples_to_estimate_bandpass);
    }

    if (header.nifs > 1)
      throw std::runtime_error("Don't know how to correct bandpass with "
          "multiple IFs");

    bp_samples = std::min(num_samples_to_estimate_bandpass, in_n);
  }

  size_t floats_per_channel = in_n;

  // set up for averaging
  if (do_avg) {
    floats_per_channel += (size_t)out_n;
    header.nsamples = out_n;
    header.tsamp *= (double)num_samples_to_average;
  }

  // set up for baseline removal
  std::unique_ptr<BaselineRemover> baseline_remover;
  if (do_base) {
    baseline_remover = std::unique_ptr<BaselineRemover>(
        new BaselineRemover(out_n, header.nchans, header.tsamp,
            baseline_length_in_sec, mUseGPU));

    floats_per_channel += baseline_remover->Ram_per_channel();
  }

  // set up for barycentering
  std::unique_ptr<Barycenter> bary;
  float * baryBuf = nullptr;
  if (do_bary && (header.barycentric == 0)) {
    bary = std::unique_ptr<Barycenter>(new Barycenter(header.tsamp,
        header.tstart, out_n, header.src_raj, header.src_dej,
        observatoryCodeForBarycentering));
    baryBuf = (float*)malloc(out_n * sizeof(float));

    header.barycentric = 1;
    header.tstart = bary->BaryStartMJD();
  }

  size_t batch_size;
  size_t num_concurrent_batches;
  GetBatches(floats_per_channel, header.nchans, &batch_size,
      &num_concurrent_batches);

  size_t num_batches = ((size_t)header.nchans + batch_size - 1) / batch_size;

  if (batch_size <= 0)
    throw std::runtime_error("Not enough memory");

  // use a different name for the incomplete file (which has the final
  // size, though)
  SigProc out(output + ".in_progress", header);

  // TODO add OpenMP

  float * buf_in = (float*)malloc(batch_size * (size_t)in_n * sizeof(float));

  float * buf_out = buf_in;
  if (do_avg)
    buf_out = (float*)malloc(batch_size * (size_t)out_n * sizeof(float));

  std::set<size_t> kill_idxs;

  // add channel killed by mask
  if (mpMask != nullptr)
    kill_idxs.insert(mpMask->ZappedChannels().begin(),
        mpMask->ZappedChannels().end());

  // measure bandpass
  if (do_bp) {
    printf("Measuring bandpass... ");
    fflush(stdout);

    // we checked above that nifs == 1
    std::vector<double> sum(header.nchans, 0.0);
    std::vector<int> num(header.nchans, 0);

    // read raw data in time chunks
    size_t max_t = batch_size * (size_t)in_n / (size_t)header.nchans;
    // max 16 MB
    size_t t_chunk = std::min(max_t, (size_t)(4 * 1024 * 1024 / header.nchans));
    t_chunk = std::min(t_chunk, bp_samples);

    size_t num_chunks = (bp_samples + t_chunk - 1) / t_chunk;

    int fd = input.FD();
    seek(fd, input.HeaderSize());

    for (size_t c = 0; c < num_chunks; ++c) {
      size_t first_t = c * t_chunk;
      size_t len = std::min(t_chunk, bp_samples - first_t);

      read_data(fd, (char*)buf_in, len * header.nchans * sizeof(float));

      if (mpMask == nullptr) {
        for (size_t t = 0; t < len; ++t) {
          for (size_t i = 0; i < (size_t)header.nchans; ++i) {
            sum[i] += buf_in[t * header.nchans + i];
          }
        }
      } else {
        for (size_t t = 0; t < len; ++t) {
          int interval = (first_t + t) / mpMask->IntervalSize();
          auto bad_channels = mpMask->ZappedChannelsPerInterval()[interval];

          for (size_t i = 0; i < (size_t)header.nchans; ++i) {
            if (bad_channels.count(i) == 0) {
              sum[i] += buf_in[t * header.nchans + i];
              num[i] += 1;
            }
          }
        }
      }

      printf("\33[2K\rMeasuring bandpass... %3i%%",
          (int)(100.0 * (double)(c + 1) / (double)(num_chunks)));
      fflush(stdout);
    }

    printf("\33[2K\rMeasuring bandpass... done\n");

    // write out bandpass and get indices of channels that need to be zeroed out
    std::string path = output + ".bandpass";
    FILE * fout = fopen(path.c_str(), "w");
    fprintf(fout, "# [1] = Channel number (starting at 1)\n");
    fprintf(fout, "# [2] = Channel frequency [MHz]\n");
    fprintf(fout, "# [3] = Bandpass\n");
    if (bandpass_smoothing_parameter > 0.0)
      fprintf(fout, "# [4] = Smoothed bandpass (parameter = %.2f)\n",
          bandpass_smoothing_parameter);

    double fch1 = header.fch1;
    double foff = header.foff;

    double bp_sum = 0.0;
    std::vector<float> orig_bp(bp.size());
    std::vector<float> delta_bp(bp.size());

    if (mpMask == nullptr) {
      for (size_t c = 0; c < orig_bp.size(); ++c)
        orig_bp[c] = sum[c] / (double)bp_samples;
    } else {
      for (size_t c = 0; c < orig_bp.size(); ++c) {
        if (num[c] == 0)
          bp[c] = 1.0;
        else
          orig_bp[c] = sum[c] / (double)num[c];
      }
    }

    // smooth bandpass if requested
    if (bandpass_smoothing_parameter > 0.0) {
      for (size_t i = 0; i < orig_bp.size(); ++i) {
        double sum = 0.0;
        double denom = 0.0;

        for (size_t j = 0; j < orig_bp.size(); ++j) {
          double d = (double)i - (double)j;
          double arg = d * d / (2.0 * bandpass_smoothing_parameter);
          arg = std::max(arg, 100.0);
          double k = exp(-arg);
          sum += k * orig_bp[j];
          denom += k;
        }

        bp[i] = sum / denom;
      }
    } else {
      bp = orig_bp;
    }

    for (size_t c = 0; c < bp.size(); ++c) {
      if (bandpass_smoothing_parameter > 0.0) {
        fprintf(fout, "%6lu  %12.3f  %18.8e  %18.8e\n", c + 1,
            fch1 + (double)c * foff, orig_bp[c], bp[c]);
      } else {
        fprintf(fout, "%6lu  %12.3f  %18.8e\n", c + 1, fch1 + (double)c * foff,
            bp[c]);
      }

      float median_bp = 0.0;
      if (c == 0)
        median_bp = bp[0];
      else if (c == 1)
        median_bp = median5(bp[0], bp[0], bp[1], bp[2], bp[3]);
      else if (c == bp.size() - 2)
        median_bp = median5(bp[c-2], bp[c-1], bp[c], bp[c+1], bp[c+1]);
      else if (c == bp.size() - 1)
        median_bp = bp[c];
      else
        median_bp = median5(bp[c-2], bp[c-1], bp[c], bp[c+1], bp[c+2]);

      delta_bp[c] = bp[c] - median_bp;
      bp_sum += delta_bp[c];
    }

    fclose(fout);

    double mean_delta_bp = bp_sum / (double)bp.size();

    bp_sum = 0.0;
    for (size_t i = 0; i < delta_bp.size(); ++i) {
      double diff = delta_bp[i] - mean_delta_bp;
      bp_sum += diff * diff;
    }

    double rms_delta_bp = sqrt(bp_sum / (double)bp.size());

    for (size_t i = 0; i < bp.size(); ++i) {
      double diff = abs(delta_bp[i] - mean_delta_bp);
      if (diff > 3.0 * rms_delta_bp || (bp[i] <= 0.0)) {
        // kill this channel
        kill_idxs.insert(i);
      }
    }

    printf("Zeroing channels: ");
    if (kill_idxs.size() == 0) {
      printf("none\n");
    } else {
      auto itr = kill_idxs.begin();
      printf("%lu", *itr + 1);
      ++itr;
      for (; itr != kill_idxs.end(); ++itr)
        printf(", %lu", *itr + 1);
      printf("\n");
    }
  }

  for (int if_idx = 0; if_idx < header.nifs; ++if_idx) {
    for (size_t b = 0; b < num_batches; ++b) {
      size_t first_channel = b * batch_size;
      size_t num_channels = std::min(batch_size,
          (size_t)header.nchans - first_channel);

      printf("Batch %lu of %lu: reading... ", b + 1, num_batches);
      fflush(stdout);

      input.GetChannels(buf_in, if_idx, first_channel, num_channels, true);

      printf("\33[2K\rBatch %lu of %lu: processing... ", b + 1, num_batches);
      fflush(stdout);

      if (do_bp || do_avg) {
        for (size_t c = 0; c < num_channels; ++c) {
          // check if we zero this channel
          size_t channel = first_channel + c;
          if (kill_idxs.count(channel) > 0) {
            memset(buf_out + c * out_n, 0, out_n * sizeof(float));
            continue;
          }

          // average samples
          for (size_t t = 0; t < (size_t)out_n; ++t) {
            if (do_avg) {
              double sum = 0.0;
              for (int i = 0; i < num_samples_to_average; ++i)
                sum += buf_in[c * in_n + t * num_samples_to_average + i];
              buf_out[c * out_n + t] = sum / (double)num_samples_to_average;
            }

            if (do_bp)
              buf_out[c * out_n + t] /= bp[channel];
          }
        }
      }

      if (do_base) {
        baseline_remover->Process_batch(buf_out, num_channels);
      }

      // apply RFI zap mask
      if (mpMask != nullptr) {
        for (size_t c = 0; c < num_channels; ++c) {
          size_t channel = first_channel + c;

          if (kill_idxs.count(channel) > 0)
            // this is a channel that gets zapped completely, which is already
            // done
            continue;

          for (int i : mpMask->ZappedIntervalsPerChannel()[channel]) {
            size_t len = std::min(mpMask->IntervalSize(),
                out_n - i * mpMask->IntervalSize());
            memset(buf_out + c * out_n + i * mpMask->IntervalSize(), 0,
                len * sizeof(float));
          }
        }
      }

      if (do_bary && (baryBuf != nullptr)) {
        for (size_t c = 0; c < num_channels; ++c) {
          memcpy(baryBuf, buf_out + c * out_n, out_n * sizeof(float));
          bary->DoBarycenterCorrection(baryBuf, buf_out + c * out_n, out_n);
        }
      }

      printf("\33[2K\rBatch %lu of %lu: writing... ", b + 1, num_batches);
      fflush(stdout);

      out.SetChannels(if_idx, first_channel, buf_out, num_channels, true);

      printf("\33[2K\rBatch %lu of %lu: done\n", b + 1, num_batches);
    }
  }

  // clean up
  baseline_remover = nullptr;

  free(buf_in);
  if (do_avg)
    free(buf_out);
  if (baryBuf != nullptr)
    free(baryBuf);

  std::string src = output + ".in_progress";
  if (rename(src.c_str(), output.c_str()) != 0)
    throw std::runtime_error(
        "Failed to rename file '" + src + "' to '" + output + "'");
}

// explicit template instantiations
template void SigProcUtil::DoProcess<true, true, true, true>(
    const SigProc&, const std::string&, const int, const int, const double,
    const double, const std::string) const;

template void SigProcUtil::DoProcess<true, true, true, false>(
    const SigProc&, const std::string&, const int, const int, const double,
    const double, const std::string) const;

template void SigProcUtil::DoProcess<true, true, false, true>(
    const SigProc&, const std::string&, const int, const int, const double,
    const double, const std::string) const;

template void SigProcUtil::DoProcess<true, true, false, false>(
    const SigProc&, const std::string&, const int, const int, const double,
    const double, const std::string) const;

template void SigProcUtil::DoProcess<true, false, true, true>(
    const SigProc&, const std::string&, const int, const int, const double,
    const double, const std::string) const;

template void SigProcUtil::DoProcess<true, false, true, false>(
    const SigProc&, const std::string&, const int, const int, const double,
    const double, const std::string) const;

template void SigProcUtil::DoProcess<true, false, false, true>(
    const SigProc&, const std::string&, const int, const int, const double,
    const double, const std::string) const;

template void SigProcUtil::DoProcess<true, false, false, false>(
    const SigProc&, const std::string&, const int, const int, const double,
    const double, const std::string) const;

template void SigProcUtil::DoProcess<false, true, true, true>(
    const SigProc&, const std::string&, const int, const int, const double,
    const double, const std::string) const;

template void SigProcUtil::DoProcess<false, true, true, false>(
    const SigProc&, const std::string&, const int, const int, const double,
    const double, const std::string) const;

template void SigProcUtil::DoProcess<false, true, false, true>(
    const SigProc&, const std::string&, const int, const int, const double,
    const double, const std::string) const;

template void SigProcUtil::DoProcess<false, true, false, false>(
    const SigProc&, const std::string&, const int, const int, const double,
    const double, const std::string) const;

template void SigProcUtil::DoProcess<false, false, true, true>(
    const SigProc&, const std::string&, const int, const int, const double,
    const double, const std::string) const;

template void SigProcUtil::DoProcess<false, false, true, false>(
    const SigProc&, const std::string&, const int, const int, const double,
    const double, const std::string) const;

template void SigProcUtil::DoProcess<false, false, false, true>(
    const SigProc&, const std::string&, const int, const int, const double,
    const double, const std::string) const;

template void SigProcUtil::DoProcess<false, false, false, false>(
    const SigProc&, const std::string&, const int, const int, const double,
    const double, const std::string) const;
