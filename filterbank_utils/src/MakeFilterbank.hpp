/*
 * MakeFilterbank.hpp
 *
 *  Created on: Aug 29, 2016
 *      Author: jlippuner
 */

#ifndef SRC_SIGPROC_MAKEFILTERBANK_HPP_
#define SRC_SIGPROC_MAKEFILTERBANK_HPP_

#include <memory>
#include <vector>

#include "SigProc.hpp"
#include "MakeFilterbankConfig.hpp"

class MakeFilterbank {
public:
  MakeFilterbank(const MakeFilterbankConfig& config);

  ~MakeFilterbank();

  void ProcessDadaFile(const std::string& dadaFile,
      const std::string& outputPrefix);

  void ProcessDirectory(const std::string& directory,
      const std::string& outputPrefix, const std::string& inputPrefix,
      const int skip, const int num);

  void MonitorDirectory(const std::string& directory,
      const std::string& outputPrefix, const std::string& inputPrefix,
      const int skip, const int num);

private:
  std::vector<std::string> GetDadaFilesInDir(const std::string& directory,
      const std::string& filterPrefix, std::string * const prefixUsed) const;

  void MakeSigProcFiles(const std::string& inputPath,
      const std::string& outputPrefix);

  void CloseSigProcFiles();

  void DoProcessDadaFile(const std::string& dadaFile) {
    if (mConf.ChannelOffset_MHz < 0.0)
      Do1ProcessDadaFile<true>(dadaFile);
    else
      Do1ProcessDadaFile<false>(dadaFile);
  }

  template<bool FLIP>
  void Do1ProcessDadaFile(const std::string& dadaFile) {
    if (mConf.Endian == MakeFilterbankConfig::Endianness_t::BIG)
      Do2ProcessDadaFile<FLIP, true>(dadaFile);
    else
      Do2ProcessDadaFile<FLIP, false>(dadaFile);
  }

  template<bool FLIP, bool BIGENDIAN>
  void Do2ProcessDadaFile(const std::string& dadaFile);

  MakeFilterbankConfig mConf;

  std::vector<std::unique_ptr<SigProc>> mpFils;
  std::vector<SigProcHeader> mHeaders;
  std::vector<int> mNumChans, mStart;

  char * mInBuf, * mOutBuf;
};

#endif /* SRC_SIGPROC_MAKEFILTERBANK_HPP_ */
