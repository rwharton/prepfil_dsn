/*
 * Barycenter.hpp
 *
 *  Created on: Sep 20, 2016
 *      Author: jlippuner
 */

#ifndef SRC_BARYCENTER_HPP_
#define SRC_BARYCENTER_HPP_

#include <string>
#include <vector>

class Barycenter {
public:
  Barycenter(const double tsampInSec, const double tstartMJD,
      const size_t num, const double ra, const double dec,
      const std::string obs);

  void DoBarycenterCorrection(const float * const datIn, float * const datOut,
      const size_t num);

  static void GetBarycenterTimes(const double * const topoTimes,
      double * const baryTimes, const size_t N, const char * const raStr,
      const char * const decStr, const char * const obs,
      const char * const ephem);

  double BaryStartMJD() const {
    return mBaryStartMJD;
  }

private:
  std::vector<int> mDiffbins;
  double mBaryStartMJD;
};

#endif /* SRC_BARYCENTER_HPP_ */
