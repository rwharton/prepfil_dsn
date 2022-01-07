/*
 * ScanFile.cpp
 *
 *  Created on: Sep 12, 2016
 *      Author: jlippuner
 */

#include "ScanFile.hpp"

#include <cstdio>
#include <fstream>
#include <stdexcept>

#include "utils.hpp"

std::string Scan::OutDir() const {
  char buf[8];
  sprintf(buf, "s%02i-", Num);

  return std::string(buf) + ToUpper(Target);
}

ScanFile::ScanFile(const std::string& path) {
  std::ifstream ifs(path);

  if (!ifs.good())
    throw std::invalid_argument("Cannot open file '" + path + "' for reading");

  char name[32];
  int numMin, numSec, yearMonDay, hour, min, sec, number, dt;

  std::string line;

  // read each line
  while (std::getline(ifs, line)) {
    for (size_t i = 0; i < line.size(); ++i)
      if (!isprint(line[i]))
        line[i] = ' ';

    int ret = sscanf(line.c_str(), "%s %d %d %d %d %d %d %d %d", name, &numMin,
        &numSec, &yearMonDay, &hour, &min, &sec, &number, &dt);

    if (ret == EOF) {
      break;
    } else if (ret == 9) {
      Scan scan;
      scan.Target = ToUpper(std::string(name));
      scan.Num = number;
      scan.SampleTimeMicroSec = (1024.0 * ((double)dt + 1.0)) / 1.0e3;

      int yr = yearMonDay / 10000 + 2000;
      int mo = (yearMonDay / 100) % 100;
      int day = yearMonDay % 100;

      char buf[32];
      sprintf(buf, "%04i-%02i-%02i-%02i:%02i:%02i", yr, mo, day, hour, min,
          sec);

      scan.Prefix = std::string(buf);
      scan.MJD = MakeMJD(scan.Prefix);

//      printf("s%02i-%s: dt = %.3f, prefix = %s, MJD = %.8f\n",
//          scan.Num, scan.Target.c_str(), scan.SampleTimeMicroSec,
//          scan.Prefix.c_str(), scan.MJD);

      mScans.insert({ scan.Num, scan });
    } else {
      throw std::runtime_error("Could not read file '" + path + "'");
    }
  }

  ifs.close();
}
