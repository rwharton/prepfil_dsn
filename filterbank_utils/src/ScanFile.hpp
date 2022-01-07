/*
 * ScanFile.hpp
 *
 *  Created on: Sep 12, 2016
 *      Author: jlippuner
 */

#ifndef SRC_SCANFILE_HPP_
#define SRC_SCANFILE_HPP_

#include <string>
#include <map>

struct Scan {
  std::string Target;
  int Num;
  double MJD;
  std::string Prefix;
  double SampleTimeMicroSec;

  std::string OutDir() const;
};

class ScanFile {
public:
  ScanFile(const std::string& path);

  const std::map<int, Scan>& Scans() const {
    return mScans;
  }

private:
  std::map<int, Scan> mScans;
};

#endif /* SRC_SCANFILE_HPP_ */
