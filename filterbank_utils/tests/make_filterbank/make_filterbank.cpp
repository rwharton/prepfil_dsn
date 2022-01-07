/*
 * make_filterbank.cpp
 *
 *  Created on: Aug 31, 2016
 *      Author: jlippuner
 */

#include <cmath>

#include "MakeFilterbank.hpp"
#include "utils.hpp"

bool check_files(const std::string f1, const std::string f2) {
  const SigProc s1(f1);
  const SigProc s2(f2);

  bool res = compare_results(s1.GetData(), s2.GetData());

  if (res)
    printf("Data in files '%s' and '%s' are the same\n", f1.c_str(),
        f2.c_str());
  else
    printf("Data in files '%s' and '%s' differ\n", f1.c_str(), f2.c_str());

  return res;
}

int main(int, char**) {
  {
    auto conf = MakeFilterbankConfig::Read("conf_very_short");

    MakeFilterbank mf(conf);
    mf.ProcessDadaFile("very_short_big_endian.dada", "");

    if (!check_files("slcp_very_short_res.fil",
        "very_short_big_endian_S-LCP.fil"))
      return 1;
    if (!check_files("srcp_very_short_res.fil",
        "very_short_big_endian_S-RCP.fil"))
      return 1;
    if (!check_files("xlcp_very_short_res.fil",
        "very_short_big_endian_X-LCP.fil"))
      return 1;
    if (!check_files("xrcp_very_short_res.fil",
        "very_short_big_endian_X-RCP.fil"))
      return 1;
  }

  {
    auto conf = MakeFilterbankConfig::Read("conf_short_flip");

    MakeFilterbank mf(conf);
    mf.ProcessDadaFile("short_big_endian.dada", "flip_");

    if (!check_files("slcp_short_flip.fil", "flip_short_big_endian_S-LCP.fil"))
      return 1;
    if (!check_files("srcp_short_flip.fil", "flip_short_big_endian_S-RCP.fil"))
      return 1;
    if (!check_files("xlcp_short_flip.fil", "flip_short_big_endian_X-LCP.fil"))
      return 1;
    if (!check_files("xrcp_short_flip.fil", "flip_short_big_endian_X-RCP.fil"))
      return 1;
  }

  {
    auto conf = MakeFilterbankConfig::Read("conf_short_no-flip");

    MakeFilterbank mf(conf);
    mf.ProcessDadaFile("short_little_endian.dada", "no-flip_");

    if (!check_files("slcp_short_no-flip.fil",
        "no-flip_short_little_endian_S-LCP.fil"))
      return 1;
    if (!check_files("srcp_short_no-flip.fil",
        "no-flip_short_little_endian_S-RCP.fil"))
      return 1;
    if (!check_files("xlcp_short_no-flip.fil",
        "no-flip_short_little_endian_X-LCP.fil"))
      return 1;
    if (!check_files("xrcp_short_no-flip.fil",
        "no-flip_short_little_endian_X-RCP.fil"))
      return 1;
  }

  return 0;
}
