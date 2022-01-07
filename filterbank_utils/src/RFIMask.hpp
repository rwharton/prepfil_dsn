/*
 * RFIMask.hpp
 *
 *  Created on: Aug 23, 2016
 *      Author: jlippuner
 */

#ifndef SRC_SIGPROC_RFIMASK_HPP_
#define SRC_SIGPROC_RFIMASK_HPP_

#include <cstddef>
#include <set>
#include <string>
#include <vector>

#include "SigProcHeader.hpp"

class RFIMask {
public:
  RFIMask(const std::string& filename, const SigProcHeader& sigprocHeader);

  int NumChannels() const {
    return mNumChannels;
  }

  int NumIntervals() const {
    return mNumIntervals;
  }

  int IntervalSize() const {
    return mIntervalSize;
  }

  const std::set<int>& ZappedChannels() const {
    return mZappedChannels;
  }

  const std::vector<std::vector<int>>& ZappedIntervalsPerChannel() const {
    return mZappedIntervalsPerChannel;
  }

  const std::vector<std::set<int>>& ZappedChannelsPerInterval() const {
    return mZappedChannelsPerInterval;
  }

private:
  int mNumChannels;
  int mNumIntervals;
  int mIntervalSize;

  std::set<int> mZappedChannels;
  std::vector<std::vector<int>> mZappedIntervalsPerChannel;
  std::vector<std::set<int>> mZappedChannelsPerInterval;
};

#endif /* SRC_SIGPROC_RFIMASK_HPP_ */
