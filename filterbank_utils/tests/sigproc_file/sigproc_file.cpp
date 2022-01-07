/*
 * sigproc_file.cpp
 *
 *  Created on: Aug 8, 2016
 *      Author: jlippuner
 */

#include "SigProc.hpp"

int main(int, char**) {
  const SigProc original("input");

  // play with file that shouldn't modify it
  {
    SigProc copy("copy", original.Header());
    copy.SetData(original.GetData());

    for (int c = 0; c < copy.Header().nchans; ++c) {
      auto dat = copy.GetChannel(c);
      copy.SetChannel(c, dat);
    }

    int nchan = copy.Header().nchans;
    auto dat = copy.GetChannels(nchan / 4, nchan / 2);
    copy.SetChannels(nchan / 4, dat);
  }

  printf("opening copy\n");
  const SigProc copy("copy");
  printf("done\n");

  if (original.Header() != copy.Header()) {
    printf("Headers differ\n");
    return 1;
  }

  if (original.GetData() != copy.GetData()) {
    printf("Data differs\n");
    return 1;
  }

  return 0;
}
