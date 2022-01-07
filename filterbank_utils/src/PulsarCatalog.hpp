/*
 * Pulsars.hpp
 *
 *  Created on: Sep 12, 2016
 *      Author: jlippuner
 */

#ifndef SRC_PULSARCATALOG_HPP_
#define SRC_PULSARCATALOG_HPP_

#include <map>
#include <string>
#include <vector>

struct Pulsar {
  std::string Name;
  double RA, Dec;
};

class PulsarCatalog {
public:
  PulsarCatalog(const std::string& path);

  const std::map<std::string, Pulsar>& Pulsars() const {
    return mPulsars;
  }

private:
  std::map<std::string, Pulsar> mPulsars;
};

#endif /* SRC_PULSARCATALOG_HPP_ */
