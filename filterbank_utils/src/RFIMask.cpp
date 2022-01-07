/*
 * RFIMask.cpp
 *
 *  Created on: Aug 23, 2016
 *      Author: jlippuner
 */

#include "RFIMask.hpp"

#include <cmath>
#include <fstream>
#include <stdexcept>

#include "utils.hpp"

RFIMask::RFIMask(const std::string& filename,
    const SigProcHeader& sigprocHeader) :
    mNumChannels(0),
    mNumIntervals(0),
    mIntervalSize(0) {
  std::ifstream istm(filename, std::ios::in | std::ios::binary);

  if (istm.fail())
    throw std::runtime_error(
        "Could not open RFI mask file '" + filename + "' for reading");

  double timeSigma, freqSigma, mjd, secPerInterval, lowF, dF;

  istm.read((char*)&timeSigma, sizeof(double));
  istm.read((char*)&freqSigma, sizeof(double));
  istm.read((char*)&mjd, sizeof(double));
  istm.read((char*)&secPerInterval, sizeof(double));
  istm.read((char*)&lowF, sizeof(double));
  istm.read((char*)&dF, sizeof(double));

  if (istm.fail())
    throw std::runtime_error("Failure when reading '" + filename + "'");

  if (mjd != sigprocHeader.tstart)
    throw std::runtime_error("Start MJDs don't agree");

  double foff = sigprocHeader.foff;
  double fch1 = sigprocHeader.fch1;

  if (fabs(dF) != fabs(foff))
    throw std::runtime_error("Frequency steps don't agree");

  double low = std::min(fch1, fch1 + (double)(sigprocHeader.nchans - 1) * foff);
//  printf("lowF = %.20e, low = %.20e\n", lowF, low);
  if (lowF != low)
    throw std::runtime_error("Lowest frequencies don't agree");

  bool invertChannels = (dF * foff < 0.0);

  istm.read((char*)&mNumChannels, sizeof(int));
  istm.read((char*)&mNumIntervals, sizeof(int));
  istm.read((char*)&mIntervalSize, sizeof(int));

  if (mNumChannels != sigprocHeader.nchans)
    throw std::runtime_error("Numbers of channels don't agree");

  if (!almost_equals((double)mIntervalSize * sigprocHeader.tsamp,
      secPerInterval))
    throw std::runtime_error("Times per interval don't agree");

  int num = (sigprocHeader.nsamples + mIntervalSize - 1) / mIntervalSize;
//  printf("%i, %i\n", mNumIntervals, num);
  if (mNumIntervals != num)
    throw std::runtime_error("Numbers of intervals don't agree");

  // read channels that are zapped at all times
  int n;
  istm.read((char*)&n, sizeof(int));
  std::vector<int> zapped(n);
  if (n > 0)
    istm.read((char*)zapped.data(), n * sizeof(int));

  for (int c : zapped)
    mZappedChannels.insert(invertChannels ? mNumChannels - c - 1 : c);

  // read intervals that are zapped for all channels
  istm.read((char*)&n, sizeof(int));
  std::vector<int> zappedIntervals(n);
  if (n > 0)
    istm.read((char*)zappedIntervals.data(), n * sizeof(int));

  // we record the zapped intervals per channel, first in a set to avoid
  // duplicates and then we'll convert the set of intervals to vectors
  std::vector<std::set<int>> zappedIntervalsPerChannel(mNumChannels);

  for (int c = 0; c < mNumChannels; ++c)
    zappedIntervalsPerChannel[c].insert(zappedIntervals.begin(),
        zappedIntervals.end());

  // read number of zapped channels per interval
  std::vector<int> numZap(mNumIntervals);
  istm.read((char*)numZap.data(), mNumIntervals * sizeof(int));

  mZappedChannelsPerInterval.resize(mNumIntervals);

  for (int i = 0; i < mNumIntervals; ++i) {
    for (int c : mZappedChannels)
      mZappedChannelsPerInterval[i].insert(c);

    if ((numZap[i] > 0) && (numZap[i] < mNumChannels)) {
      std::vector<int> zappedChannels(numZap[i]);
      istm.read((char*)zappedChannels.data(), numZap[i] * sizeof(int));

      for (int c : zappedChannels) {
        int chan = invertChannels ? mNumChannels - c - 1 : c;
        zappedIntervalsPerChannel[chan].insert(i);
        mZappedChannelsPerInterval[i].insert(chan);
      }
    } else if (numZap[i] == mNumChannels) {
      for (int c = 0; c < mNumChannels; ++c) {
        zappedIntervalsPerChannel[c].insert(i);
        mZappedChannelsPerInterval[i].insert(c);
      }
    }
  }

  mZappedIntervalsPerChannel.resize(mNumChannels);
  for (int c = 0; c < mNumChannels; ++c) {
    mZappedIntervalsPerChannel[c].assign(zappedIntervalsPerChannel[c].begin(),
        zappedIntervalsPerChannel[c].end());
  }

  // make sure we're at the end of the file
  istm.peek();
  if (!istm.eof())
    throw std::runtime_error("Expected EOF in mask");

  istm.close();
}

