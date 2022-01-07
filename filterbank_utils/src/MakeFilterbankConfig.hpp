/*
 * MakeFilterbankConfig.hpp
 *
 *  Created on: Aug 29, 2016
 *      Author: jlippuner
 */

#ifndef SRC_SIGPROC_MAKEFILTERBANKCONFIG_HPP_
#define SRC_SIGPROC_MAKEFILTERBANKCONFIG_HPP_

#include <string>
#include <vector>

#include "MakeFilterbankConfig.pb.h"

struct Filterbank_t {
  Filterbank_t();

  static Filterbank_t FromPB(const make_filterbank_proto::filterbank_t& fb);

  void SetPB(make_filterbank_proto::filterbank_t * const fb) const;

  std::string Name;
  double FreqCh1_MHz;
  int StartChannelIdx, EndChannelIdx;
};

struct Source_t {
  Source_t();

  static Source_t FromPB(const make_filterbank_proto::source_t& src);

  void SetPB(make_filterbank_proto::source_t * const src) const;

  std::string Name;
  double RA, Dec;
};

struct Observation_t {
  Observation_t();

  static Observation_t FromPB(const make_filterbank_proto::observation_t& obs);

  void SetPB(make_filterbank_proto::observation_t * const obs) const;

  double Azimuth, Zenith;
  double Start_MJD;
  int TelescopeId, MachineId;
  int NumIFs;
  double SampleTime_us;
  bool Barycentric, Pulsarcentric;
};

struct MakeFilterbankConfig {
  static MakeFilterbankConfig Read(const std::string& path);

  static MakeFilterbankConfig FromPB(
      const make_filterbank_proto::make_filterbank_config& conf);

  void SetPB(make_filterbank_proto::make_filterbank_config * const conf) const;

  double Bandwidth_MHz;
  double ChannelOffset_MHz;
  int NumChannels;

  int InputBits;
  int OutputBits;

  enum class Endianness_t {
    LITTLE,
    BIG
  };

  Endianness_t Endian;

  Source_t Source;
  Observation_t Observation;

  std::vector<Filterbank_t> Filterbanks;

private:
  MakeFilterbankConfig();
};

#endif /* SRC_SIGPROC_MAKEFILTERBANKCONFIG_HPP_ */
