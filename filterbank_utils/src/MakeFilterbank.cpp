/*
 * MakeFilterbank.cpp
 *
 *  Created on: Aug 29, 2016
 *      Author: jlippuner
 */

#include "MakeFilterbank.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <endian.h>
#include <thread>

#ifndef _LARGEFILE64_SOURCE
  #define _LARGEFILE64_SOURCE 1
#endif
#include <errno.h>
#include <unistd.h>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/version.hpp>

#ifndef BOOST_VERSION
#  error "Boost version not available in MakeFilterbank.cpp"
#endif

// determine if we're using the old boost::filesystem version 2
#if BOOST_VERSION <= 104200
#  define BOOST_FILESYSTEM_V2
#elif BOOST_VERSION <= 104900
#  if BOOST_FILESYSTEM_VERSION == 2
#    define BOOST_FILESYSTEM_V2
#  endif
#endif

#ifdef _OPENMP
  #include <omp.h>
#endif

#include "utils.hpp"

namespace {

std::string GetInputPrefix(const std::string& dada) {
  boost::filesystem::path path(dada);
#ifdef BOOST_FILESYSTEM_V2
  std::string stem = path.stem();
#else
  std::string stem = path.stem().string();
#endif

  boost::regex expr("(.*)_[0-9]+\\.[0-9]+");

  boost::smatch match;
  if (boost::regex_match(stem, match, expr))
    return match[1].str();
  else
    return stem;
}

} // namespace [unnamed]

MakeFilterbank::MakeFilterbank(const MakeFilterbankConfig& config) :
  mConf(config),
  mInBuf(nullptr),
  mOutBuf(nullptr) {
  printf("Input config:\n");
  printf("  Bandwidth: %.3f\n", mConf.Bandwidth_MHz);
  printf("  Channel offset: %.3f\n", mConf.ChannelOffset_MHz);
  printf("  Num channels: %i\n", mConf.NumChannels);
  printf("  Input bits: %i\n", mConf.InputBits);
  printf("  Output bits: %i\n", mConf.OutputBits);
  printf("\n");
  printf("  Source:\n");
  printf("    Name: %s\n", mConf.Source.Name.c_str());
  printf("    RA: %.3f\n", mConf.Source.RA);
  printf("    Dec: %.3f\n", mConf.Source.Dec);
  printf("\n");
  printf("  Observation:\n");
  printf("    Azimuth: %.3f\n", mConf.Observation.Azimuth);
  printf("    Zenith: %.3f\n", mConf.Observation.Zenith);
  printf("    Start MJD: %.3f\n", mConf.Observation.Start_MJD);
  printf("    Telescope ID: %i\n", mConf.Observation.TelescopeId);
  printf("    Machine ID: %i\n", mConf.Observation.MachineId);
  printf("    Num IFs: %i\n", mConf.Observation.NumIFs);
  printf("    Sample time: %.3f\n", mConf.Observation.SampleTime_us);
  printf("    Barycentric: %s\n", mConf.Observation.Barycentric ? "Y" : "N");
  printf("    Pulsarcentric: %s\n",
      mConf.Observation.Pulsarcentric ? "Y" : "N");
  printf("\n");
  printf("  Filterbanks\n");
  for (size_t i = 0; i < mConf.Filterbanks.size(); ++i) {
    printf("  [%lu] '%s' @ %.3f MHz (%i to %i)\n", i,
        mConf.Filterbanks[i].Name.c_str(), mConf.Filterbanks[i].FreqCh1_MHz,
        mConf.Filterbanks[i].StartChannelIdx,
        mConf.Filterbanks[i].EndChannelIdx);
  }
  printf("\n");

  // make header template
  SigProcHeader header;

  header.source_name = mConf.Source.Name;
  header.src_raj = mConf.Source.RA;
  header.src_dej = mConf.Source.Dec;

  header.az_start = mConf.Observation.Azimuth;
  header.za_start = mConf.Observation.Zenith;
  header.tsamp = mConf.Observation.SampleTime_us * 1.0e-6;
  header.telescope_id = mConf.Observation.TelescopeId;
  header.machine_id = mConf.Observation.MachineId;
  header.data_type = 1; // filterbank
  header.barycentric = mConf.Observation.Barycentric;
  header.pulsarcentric = mConf.Observation.Pulsarcentric;
  header.nifs = mConf.Observation.NumIFs > 0 ? mConf.Observation.NumIFs : 1;

  header.foff = mConf.ChannelOffset_MHz;

  if (mConf.OutputBits > 0) {
    if (mConf.OutputBits != 32)
      throw std::runtime_error("Currently only 32 output bits are implemented");
  } else {
    mConf.OutputBits = 32;
  }

  header.nbits = mConf.OutputBits;

  if (mConf.InputBits != 16)
    throw std::runtime_error("Currently only 16 input bits are implemented");

  // create the filterbank files
  mHeaders.resize(mConf.Filterbanks.size());
  mNumChans.resize(mConf.Filterbanks.size());
  mStart.resize(mConf.Filterbanks.size());

  for (size_t i = 0; i < mConf.Filterbanks.size(); ++i) {
    if ((mConf.Filterbanks[i].StartChannelIdx >= 0)
        && (mConf.Filterbanks[i].StartChannelIdx < mConf.NumChannels)
        && (mConf.Filterbanks[i].EndChannelIdx >= 0)
        && (mConf.Filterbanks[i].EndChannelIdx < mConf.NumChannels)) {
      mStart[i] = mConf.Filterbanks[i].StartChannelIdx;
      mNumChans[i] = mConf.Filterbanks[i].EndChannelIdx - mStart[i] + 1;

      if (mNumChans[i] <= 0)
        throw std::runtime_error("Got a non-positive number of channels "
            "for filterbank file " + mConf.Filterbanks[i].Name);
    } else {
      mStart[i] = 0;
      mNumChans[i] = mConf.NumChannels;
    }

    auto thisHead = header;
    thisHead.fch1 = mConf.Filterbanks[i].FreqCh1_MHz;
    thisHead.nchans = mNumChans[i];
    mHeaders[i] = thisHead;
  }

  // allocate buffer
  size_t bufSize = (size_t)mConf.NumChannels * mConf.Filterbanks.size();
  mInBuf = (char*)malloc(bufSize * (size_t)(mConf.InputBits / 8));
  mOutBuf = (char*)malloc(bufSize * (size_t)(mConf.OutputBits / 8));
}

MakeFilterbank::~MakeFilterbank() {
  if (mInBuf != nullptr)
    free(mInBuf);
  if (mOutBuf != nullptr)
    free(mOutBuf);
  CloseSigProcFiles();
}

void MakeFilterbank::ProcessDadaFile(const std::string& dadaFile,
    const std::string& outputPrefix) {
  std::string output = outputPrefix + GetInputPrefix(dadaFile);
  MakeSigProcFiles(dadaFile, output);
  DoProcessDadaFile(dadaFile);
  CloseSigProcFiles();
}

void MakeFilterbank::ProcessDirectory(const std::string& directory,
    const std::string& outputPrefix, const std::string& inputPrefix,
    const int skip, const int num) {
  std::string prefix = inputPrefix;
  auto dadaFiles = GetDadaFilesInDir(directory, prefix,
      prefix == "" ? &prefix : nullptr);

  if (dadaFiles.size() == 0) {
    printf("Found no dada files in '%s'. Exiting\n", directory.c_str());
    return;
  } else {
    printf("Found %lu dada files with prefix '%s' in '%s'\n\n",
        dadaFiles.size(), prefix.c_str(), directory.c_str());
  }

  std::string output = outputPrefix + prefix;
  MakeSigProcFiles(dadaFiles[0], output);

  size_t end = num > 0 ? skip + num : dadaFiles.size();
  end = std::min(dadaFiles.size(), end);

  printf("Skipping first %i files and processing %lu files\n", skip, end);

  for (size_t i = 0; i < end; ++ i) {
    if (i >= (size_t)skip) {
      DoProcessDadaFile(dadaFiles[i]);
    } else {
      printf("Skipping %s (skip %lu/%i)\n", dadaFiles[i].c_str(), i, skip);
    }
  }
  CloseSigProcFiles();
}

void MakeFilterbank::MonitorDirectory(const std::string& directory,
    const std::string& outputPrefix, const std::string& inputPrefix,
    const int skip, const int num) {
  std::string prefix = inputPrefix;
  auto dadaFiles = GetDadaFilesInDir(directory, prefix,
      prefix == "" ? &prefix : nullptr);

  if (dadaFiles.size() == 0) {
    printf("Directory '%s' does not contain any dada files, waiting for files "
        "to appear\n", directory.c_str());
  } else {
    printf("Directory '%s' already contains %lu dada files with prefix '%s', "
        "setting monitoring prefix to '%s'\n", directory.c_str(),
        dadaFiles.size(), prefix.c_str(), prefix.c_str());
  }

  printf("Skipping first %i files and processing %s files\n", skip,
      num > 0 ? std::to_string(num).c_str() : "all");
  int numSkipped = 0;
  int numActuallyProcessed = 0;

  std::string lastFileProcessed = "";
  size_t numProcessed = 0;

  std::string monitoringFile = "";

  uintmax_t maxSize = -1;
  uintmax_t lastSize = 0;
  int numNoChange = 0;

  while (true) {
    if (monitoringFile == "") {
      // find if there are new files that we haven't processed
      dadaFiles = GetDadaFilesInDir(directory, prefix, nullptr);

      if (dadaFiles.size() > numProcessed) {
        // there are new files
        for (size_t i = 0; i < dadaFiles.size(); ++i) {
          if ((lastFileProcessed == "") || (dadaFiles[i] > lastFileProcessed)) {
            if (i != numProcessed)
              throw std::runtime_error("Next dada file was expected to be the "
                  + std::to_string(numProcessed) + "-th file, but got the "
                  + std::to_string(i) + "-th file");

            monitoringFile = dadaFiles[i];
            printf("File '%s' appeared, waiting for file size to remain "
                "constant for 5 seconds\n", monitoringFile.c_str());
            numNoChange = 0;
            printf("\33[2K\r%s: ...", monitoringFile.c_str());
            fflush(stdout);
            break;
          }
        }

        if (monitoringFile == "")
          throw std::runtime_error("Got non-increasing dada file names");
        else
          continue;
      }
    } else {
      // we are currently monitoring a file to see when its size stops changing
      if (numNoChange == 5) {
        printf("\33[2K\r");

        // file has not changed in the last 5 seconds, process it
        if (lastFileProcessed == "") {
          // this is the first file
          if (prefix == "") {
            prefix = GetInputPrefix(monitoringFile);
            printf("Setting monitoring prefix to '%s'\n", prefix.c_str());
          }

          MakeSigProcFiles(monitoringFile, outputPrefix + prefix);
        }

        if (numSkipped >= skip) {
          DoProcessDadaFile(monitoringFile);
          ++numActuallyProcessed;
        } else {
          ++numSkipped;
          printf("Skipping %s (skip %i/%i)\n", monitoringFile.c_str(),
              numSkipped, skip);
        }

        lastFileProcessed = monitoringFile;
        ++numProcessed;
        monitoringFile = "";

        numNoChange = 0;
        lastSize = 0;

        if ((num > 0) && (numActuallyProcessed >= num)) {
          printf("Processed the requested number of files. Exiting\n");
          break;
        }

        printf("Waiting for next file ...\n");
      } else {
        // still waiting
        auto size = boost::filesystem::file_size(monitoringFile);
        if (size < maxSize) {
          if (size == lastSize) {
            ++numNoChange;
            printf("\33[2K\r%s: %lu, waiting %i s", monitoringFile.c_str(),
                size, 5 - numNoChange);
            fflush(stdout);
          } else if (size > lastSize) {
            numNoChange = 0;
            lastSize = size;
            printf("\33[2K\r%s: %lu", monitoringFile.c_str(), size);
            fflush(stdout);
          } else {
            throw std::runtime_error("File '" + monitoringFile
                + "' has shrunk in size");
          }
        } else {
          throw std::runtime_error("File '" + monitoringFile
              + "' has an invalid size");
        }
      }
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

std::vector<std::string> MakeFilterbank::GetDadaFilesInDir(
    const std::string& directory, const std::string& filterPrefix,
    std::string * const prefixUsed) const {
  boost::filesystem::path dir(directory);
  if (!boost::filesystem::is_directory(dir))
    throw std::runtime_error("'" + directory + "' is not a directory");

  std::vector<std::string> dadaFiles;
  std::string prefix = filterPrefix;

  boost::filesystem::directory_iterator enditr;
  for (boost::filesystem::directory_iterator itr(dir); itr != enditr; ++itr) {
#ifdef BOOST_FILESYSTEM_V2
    auto pth = *itr;
    std::string ext = pth.extension();
#else
    auto pth = itr->path();
    std::string ext = pth.extension().string();
#endif

    if (boost::filesystem::is_regular_file(pth) && (ext == ".dada")) {
      std::string fn = pth.string();
      if (prefix == "")
        prefix = GetInputPrefix(fn);

      if (prefix == GetInputPrefix(fn))
        dadaFiles.push_back(fn);
    }
  }

  if (prefixUsed != nullptr)
    *prefixUsed = prefix;

  if (dadaFiles.size() == 0)
    return std::vector<std::string>();

  std::sort(dadaFiles.begin(), dadaFiles.end());

  return dadaFiles;
}

void MakeFilterbank::MakeSigProcFiles(const std::string& inputPath,
    const std::string& outputPrefix) {
  printf("Input path: %s\n", inputPath.c_str());
  printf("Output prefix: %s\n\n", outputPrefix.c_str());

  // get start time
  double tstart = 0.0;

  if (mConf.Observation.Start_MJD > 0.0) {
    tstart = mConf.Observation.Start_MJD;
  } else {
    tstart = MakeMJD(inputPath);

    if (tstart == 0.0) {
      printf("WARNING: Cannot parse input path '%s' as a date and time. "
          "Setting starting MJD to 0\n\n", inputPath.c_str());
    } else {
      printf("Determined start time from input prefix: MJD = %.10f\n\n",
          tstart);
    }
  }

  for (size_t i = 0; i < mConf.Filterbanks.size(); ++i) {
    auto header = mHeaders[i];

    header.rawdatafile = inputPath;
    header.tstart = tstart;
    std::string fname = outputPrefix + "_" + mConf.Filterbanks[i].Name + ".fil";

    printf("Creating filterbank file '%s'\n", fname.c_str());
    printf("  fch1 = %.3f MHz, %i channels\n", header.fch1, header.nchans);
    printf("  extracting channels %i to %i from dada files\n\n", mStart[i],
        mStart[i] + mNumChans[i]);

    // make parent directory
    auto parent = boost::filesystem::path(fname).parent_path();
#ifdef BOOST_FILESYSTEM_V2
    if (parent.string() != "")
      boost::filesystem::create_directories(parent);
#else
    boost::filesystem::create_directories(boost::filesystem::absolute(parent));
#endif

    mpFils.emplace_back(new SigProc(fname, header));
  }
}

void MakeFilterbank::CloseSigProcFiles() {
  for (size_t i = 0; i < mpFils.size(); ++i)
    mpFils[i].reset();
  mpFils.clear();
}

template<bool FLIP, bool BIGENDIAN>
void MakeFilterbank::Do2ProcessDadaFile(const std::string& dadaFile) {
  printf("Processing %s... %3i%%", dadaFile.c_str(), 0);
  fflush(stdout);

  std::ifstream istm(dadaFile, std::fstream::in | std::fstream::binary);

  if (istm.fail()) {
    printf("\n");
    throw std::runtime_error("Failed to open file '" + dadaFile + "'");
  }

  // get file size
  istm.seekg(0, std::ios::end);
  // subtract header size
  size_t size = (size_t)istm.tellg() - (size_t)(512 * 8);
  istm.seekg(512 * 8, std::ios::beg); // skip header

  size_t specSize = mpFils.size()
      * (size_t)(mConf.NumChannels * mConf.InputBits / 8);
  size_t numSpec = size / specSize;

  if (numSpec * specSize != size) {
    printf("\n");
    throw std::runtime_error("File '" + dadaFile + "' does not contain an "
        "integer number of spectra");
  }

  uint16_t * in = (uint16_t*)mInBuf;
  float * out = (float*)mOutBuf;
  const int stride = 4; // TODO is this always 4 or is it mpFils.size()?

#ifdef _OPENMP
  omp_set_num_threads(mpFils.size());
#endif

  for (size_t s = 0; s < numSpec; ++s) {
    istm.read(mInBuf, specSize);

#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for (size_t f = 0; f < mpFils.size(); ++f) {
      size_t off = f * mConf.NumChannels;
      int part = mConf.NumChannels / stride;

      for (int p = 0; p < stride; ++p) {
        for (int i = 0; i < part; ++i) {
          size_t inIdx =
              FLIP ? mConf.NumChannels - 1 - (p * part + i) : p * part + i;
          uint16_t val = in[off + stride * i + p];
          val = BIGENDIAN ? be16toh(val) : le16toh(val);
          out[off + inIdx] = (float)val;
        }
      }

      // write to filterbank files
      size_t len = mNumChans[f] * mConf.OutputBits / 8;

      if ((size_t)write(mpFils[f]->FD(), (char*)(out + off + mStart[f]), len)
          != len) {
        perror("Failure in MakeFilterbank::Do2ProcessDadaFile");
        throw std::runtime_error("Failed to write data");
      }
    }

    int prog = (double)(s + 1) / (double)numSpec * 100.0;
    printf("\b\b\b\b%3i%%", prog);
    fflush(stdout);
  }

  istm.close();

  printf("\33[2K\rProcessing %s... flushing cache (this could take a while)",
      dadaFile.c_str());
  fflush(stdout);

  for (size_t f = 0; f < mpFils.size(); ++f)
    mpFils[f]->HardFlush();

  printf("\33[2K\rProcessing %s... done\n", dadaFile.c_str());
}

template
void MakeFilterbank::Do2ProcessDadaFile<true, true>(const std::string&);

template
void MakeFilterbank::Do2ProcessDadaFile<false, true>(const std::string&);

template
void MakeFilterbank::Do2ProcessDadaFile<true, false>(const std::string&);

template
void MakeFilterbank::Do2ProcessDadaFile<false, false>(const std::string&);
