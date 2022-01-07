/*
 * test.cpp
 *
 *  Created on: Sep 12, 2016
 *      Author: jlippuner
 */

#include <cmath>
#include <string>
#include <vector>

#include "Barycenter.hpp"
#include "SigProc.hpp"

int main(int, char ** argv) {
  std::string path(argv[1]);
  const SigProc f(path);

  float * data = (float*)malloc(f.Header().Data_size());

  auto chan = f.GetChannel(0, true);

  Barycenter bary(f.Header().tsamp, f.Header().tstart, f.Header().nsamples,
      f.Header().src_raj, f.Header().src_dej, "GS");

  bary.DoBarycenterCorrection(chan.data(), data, f.Header().nsamples);

  auto head = f.Header();
  head.barycentric = 1;
  head.tstart = bary.BaryStartMJD();

  SigProc fout(path + "_bary", head);
  fout.SetChannel(0, data, true);
}

