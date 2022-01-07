/*
 * Pulsars.cpp
 *
 *  Created on: Sep 12, 2016
 *      Author: jlippuner
 */

#include "PulsarCatalog.hpp"

#include <fstream>
#include <sstream>
#include <vector>

#include <boost/regex.hpp>

#include "utils.hpp"

namespace {

std::vector<std::string> split(const std::string& s, char delim) {
  std::vector<std::string> elems;

  std::stringstream ss;
  ss.str(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }

  return elems;
}

double GetRADec(const std::string& str, const boost::regex& expr) {
  boost::smatch match;
  if (!boost::regex_match(str, match, expr))
    return 0.0;

  double res = 10000.0 * std::stod(match[1].str());

  double minSec = 0.0;

  if (match[2].str().size() > 0) {
    minSec = 100.0 * std::stod(match[2].str());
    if (match[3].str().size() > 0)
      minSec += std::stod(match[3].str());
  }

  if (res >= 0.0)
    res += minSec;
  else
    res -= minSec;

  return res;
}

} // namespace [unnamed]

PulsarCatalog::PulsarCatalog(const std::string& path) {
  std::ifstream ifs(path);

  if (!ifs.good())
    throw std::invalid_argument("Cannot open file '" + path + "' for reading");

  std::string line;

  boost::regex RAExpr("^(\\d\\d)(?::(\\d\\d)(?::(\\d\\d[\\.\\d]*))?)?$");
  boost::regex DecExpr("^([+-]\\d\\d)(?::(\\d\\d)(?::(\\d\\d[\\.\\d]*))?)?$");

  // read each line
  while (std::getline(ifs, line)) {
    auto fs = split(line, ';');
    if (fs.size() != 4)
      throw std::runtime_error("Got wrong number of fields in '" + line + "'");

    if ((fs[0] == "#") || (fs[0] == " ")) {
      // skip first two lines
      continue;
    }

    Pulsar pulsar;

    pulsar.Name = ToUpper(fs[1]);
    pulsar.RA = GetRADec(fs[2], RAExpr);
    pulsar.Dec = GetRADec(fs[3], DecExpr);

//    printf("%s: %.3f %.3f\n", pulsar.Name.c_str(), pulsar.RA, pulsar.Dec);

    mPulsars.insert({ pulsar.Name, pulsar });
  }

  ifs.close();
}
