/*
 * make_filterbank_config.cpp
 *
 *  Created on: Aug 30, 2016
 *      Author: jlippuner
 */

#include <cmath>

#include "MakeFilterbankConfig.hpp"

int main(int, char**) {
  auto conf = MakeFilterbankConfig::Read("conf");

  printf("Config:\n");
  printf("  Bandwidth: %.3f\n", conf.Bandwidth_MHz);
  printf("  Channel offset: %.3f\n", conf.ChannelOffset_MHz);
  printf("  Num channels: %i\n", conf.NumChannels);
  printf("  Input bits: %i\n", conf.InputBits);
  printf("  Output bits: %i\n", conf.OutputBits);
  printf("\n");
  printf("  Source:\n");
  printf("    Name: %s\n", conf.Source.Name.c_str());
  printf("    RA: %.3f\n", conf.Source.RA);
  printf("    Dec: %.3f\n", conf.Source.Dec);
  printf("\n");
  printf("  Observation:\n");
  printf("    Azimuth: %.3f\n", conf.Observation.Azimuth);
  printf("    Zenith: %.3f\n", conf.Observation.Zenith);
  printf("    Start MJD: %.3f\n", conf.Observation.Start_MJD);
  printf("    Telescope ID: %i\n", conf.Observation.TelescopeId);
  printf("    Machine ID: %i\n", conf.Observation.MachineId);
  printf("    Num IFs: %i\n", conf.Observation.NumIFs);
  printf("    Sample time: %.3f\n", conf.Observation.SampleTime_us);
  printf("    Barycentric: %s\n", conf.Observation.Barycentric ? "Y" : "N");
  printf("    Pulsarcentric: %s\n", conf.Observation.Pulsarcentric ? "Y" : "N");
  printf("\n");
  printf("  Filterbanks\n");
  for (size_t i = 0; i < conf.Filterbanks.size(); ++i) {
    printf("  [%lu] '%s' @ %.3f MHz (%i to %i)\n", i,
        conf.Filterbanks[i].Name.c_str(), conf.Filterbanks[i].FreqCh1_MHz,
        conf.Filterbanks[i].StartChannelIdx, conf.Filterbanks[i].EndChannelIdx);
  }
  printf("\n");

  if (conf.Bandwidth_MHz != 1000.0) return 1;
  if (conf.ChannelOffset_MHz != -1.0) return 1;
  if (conf.NumChannels != 1024) return 1;

  if (conf.Source.Name != "J1119-6127") return 1;
  if (conf.Source.RA != 111914.3) return 1;
  if (conf.Source.Dec != -612749.5) return 1;

  if (conf.Observation.Start_MJD != 57619.769143518519) return 1;
  if (conf.Observation.TelescopeId != 12) return 1;
  if (conf.Observation.MachineId != 999) return 1;
  if (conf.Observation.NumIFs != 0) return 1;
  if (conf.Observation.SampleTime_us != 512.0) return 1;
  if (conf.Observation.Barycentric != false) return 1;
  if (conf.Observation.Pulsarcentric != true) return 1;
  if (conf.Filterbanks.size() != 4) return 1;

  if (conf.Filterbanks[0].Name != "S-LCP") return 1;
  if (conf.Filterbanks[0].FreqCh1_MHz != 2293.0) return 1;
  if (conf.Filterbanks[0].StartChannelIdx != 0) return 1;
  if (conf.Filterbanks[0].EndChannelIdx!= 95) return 1;

  if (conf.Filterbanks[1].Name != "S-RCP") return 1;
  if (conf.Filterbanks[1].FreqCh1_MHz != 2293.0) return 1;
  if (conf.Filterbanks[1].StartChannelIdx != 150) return 1;
  if (conf.Filterbanks[1].EndChannelIdx!= 250) return 1;

  if (conf.Filterbanks[2].Name != "X-LCP") return 1;
  if (conf.Filterbanks[2].FreqCh1_MHz != 8690.0) return 1;
  if (conf.Filterbanks[2].StartChannelIdx != 300) return 1;
  if (conf.Filterbanks[2].EndChannelIdx!= 600) return 1;

  if (conf.Filterbanks[3].Name != "X-RCP") return 1;
  if (conf.Filterbanks[3].FreqCh1_MHz != 8690.0) return 1;
  if (conf.Filterbanks[3].StartChannelIdx != 700) return 1;
  if (conf.Filterbanks[3].EndChannelIdx!= 1023) return 1;

  return 0;
}
