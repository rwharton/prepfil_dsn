/*
 * MakeFilterbankConfig.cpp
 *
 *  Created on: Aug 29, 2016
 *      Author: jlippuner
 */

#include "MakeFilterbankConfig.hpp"

#include <stdexcept>

#include <fcntl.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

Filterbank_t::Filterbank_t() :
    Name(""),
    FreqCh1_MHz(0.0),
    StartChannelIdx(-1),
    EndChannelIdx(-1) {}

Filterbank_t Filterbank_t::FromPB(
    const make_filterbank_proto::filterbank_t& fb) {
  Filterbank_t out;

  if (fb.has_name())
    out.Name = fb.name();

  if (fb.has_fch1_mhz())
    out.FreqCh1_MHz = fb.fch1_mhz();

  if (fb.has_start())
    out.StartChannelIdx = fb.start();

  if (fb.has_end())
    out.EndChannelIdx = fb.end();

  return out;
}

void Filterbank_t::SetPB(make_filterbank_proto::filterbank_t * const fb) const {
  if (Name != "")
    fb->set_name(Name);

  if (FreqCh1_MHz > 0.0)
    fb->set_fch1_mhz(FreqCh1_MHz);

  if (StartChannelIdx >= 0)
    fb->set_start(StartChannelIdx);

  if (EndChannelIdx >= 0)
    fb->set_end(EndChannelIdx);
}

Source_t::Source_t() :
    Name(""),
    RA(0.0),
    Dec(0.0) {}

Source_t Source_t::FromPB(const make_filterbank_proto::source_t& src) {
  Source_t out;

  if (src.has_name())
    out.Name = src.name();

  if (src.has_ra())
    out.RA = src.ra();

  if (src.has_dec())
    out.Dec = src.dec();

  return out;
}

void Source_t::SetPB(make_filterbank_proto::source_t * const src) const {
  if (Name != "")
    src->set_name(Name);

  src->set_ra(RA);
  src->set_dec(Dec);
}

Observation_t::Observation_t() :
    Azimuth(0.0),
    Zenith(0.0),
    Start_MJD(0.0),
    TelescopeId(-1),
    MachineId(-1),
    NumIFs(0),
    SampleTime_us(0.0),
    Barycentric(false),
    Pulsarcentric(false) {}

Observation_t Observation_t::FromPB(
    const make_filterbank_proto::observation_t& obs) {
  Observation_t out;

  if (obs.has_azimuth())
    out.Azimuth = obs.azimuth();

  if (obs.has_zenith())
    out.Zenith = obs.zenith();

  if (obs.has_start_mjd())
    out.Start_MJD = obs.start_mjd();

  if (obs.has_telescope_id())
    out.TelescopeId = obs.telescope_id();

  if (obs.has_machine_id())
    out.MachineId = obs.machine_id();

  if (obs.has_num_ifs())
    out.NumIFs = obs.num_ifs();

  if (obs.has_sample_time_us())
    out.SampleTime_us = obs.sample_time_us();

  if (obs.has_barycentric())
    out.Barycentric = obs.barycentric();

  if (obs.has_pulsarcentric())
    out.Pulsarcentric = obs.pulsarcentric();

  return out;
}

void Observation_t::SetPB(
    make_filterbank_proto::observation_t * const obs) const {
  obs->set_azimuth(Azimuth);
  obs->set_zenith(Zenith);

  if (Start_MJD > 0.0)
    obs->set_start_mjd(Start_MJD);

  if (TelescopeId >= 0)
    obs->set_telescope_id(TelescopeId);

  if (MachineId >= 0)
    obs->set_machine_id(MachineId);

  if (NumIFs > 0)
    obs->set_num_ifs(NumIFs);

  if (SampleTime_us > 0.0)
    obs->set_sample_time_us(SampleTime_us);

  obs->set_barycentric(Barycentric);
  obs->set_pulsarcentric(Pulsarcentric);
}

MakeFilterbankConfig::MakeFilterbankConfig() :
    Bandwidth_MHz(-1.0),
    ChannelOffset_MHz(0.0),
    NumChannels(-1),
    InputBits(-1),
    OutputBits(-1),
    Endian(Endianness_t::LITTLE){}

MakeFilterbankConfig MakeFilterbankConfig::Read(const std::string& path) {
  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  int fileDesc = open(path.c_str(), O_RDONLY);

  if (fileDesc < 0)
    throw std::runtime_error("Cannot open file '" + path + "' for reading");

  google::protobuf::io::FileInputStream inp(fileDesc);
  inp.SetCloseOnDelete(true);

  make_filterbank_proto::make_filterbank_config conf;

  if (!google::protobuf::TextFormat::Parse(&inp, &conf))
    throw std::runtime_error("Failed to parse '" + path + "': "
        + conf.DebugString());

  return MakeFilterbankConfig::FromPB(conf);
}

MakeFilterbankConfig MakeFilterbankConfig::FromPB(
    const make_filterbank_proto::make_filterbank_config& conf) {
  MakeFilterbankConfig out;

  out.Bandwidth_MHz = conf.bandwidth_mhz();
  out.ChannelOffset_MHz = conf.channel_offset_mhz();
  out.NumChannels = conf.num_channels();
  out.InputBits = conf.input_bits();
  if (conf.endian() == conf.LITTLE)
    out.Endian = Endianness_t::LITTLE;
  else if (conf.endian() == conf.BIG)
    out.Endian = Endianness_t::BIG;
  else
    throw std::runtime_error("Unknown endian");

  if (conf.has_output_bits())
    out.OutputBits = conf.output_bits();

  if (conf.has_source())
    out.Source = Source_t::FromPB(conf.source());

  if (conf.has_observation())
    out.Observation = Observation_t::FromPB(conf.observation());

  out.Filterbanks.resize(conf.filterbank_size());
  for (int i = 0; i < conf.filterbank_size(); ++i)
    out.Filterbanks[i] = Filterbank_t::FromPB(conf.filterbank(i));

  return out;
}

void MakeFilterbankConfig::SetPB(
    make_filterbank_proto::make_filterbank_config * const conf) const {
  conf->set_bandwidth_mhz(Bandwidth_MHz);
  conf->set_channel_offset_mhz(ChannelOffset_MHz);
  conf->set_num_channels(NumChannels);
  conf->set_input_bits(InputBits);

  if (OutputBits > 0)
    conf->set_output_bits(OutputBits);

  if (Endian == Endianness_t::LITTLE)
    conf->set_endian(conf->LITTLE);
  else if (Endian == Endianness_t::BIG)
    conf->set_endian(conf->BIG);
  else
    throw std::runtime_error("Unknown endian");

  Source.SetPB(conf->mutable_source());
  Observation.SetPB(conf->mutable_observation());

  for (const Filterbank_t& fb : Filterbanks)
    fb.SetPB(conf->add_filterbank());
}
