/*
 * sigproc_util.cpp
 *
 *  Created on: Aug 16, 2016
 *      Author: jlippuner
 */

#include <cmath>

#include "SigProc.hpp"
#include "SigProcUtil.hpp"

#include "utils.hpp"

int main(int, char**) {
  SigProcUtil util(0.1);

  // test averaging
  {
    const SigProc original("input");
    util.AverageSamples(original, "input_dec_6", 6);

    const SigProc my_dec6("input_dec_6");
    const SigProc dec6("dec_6");

    auto vec0 = my_dec6.GetData();
    auto vec1 = dec6.GetData();
    for (size_t i = 0; i < vec0.size(); ++i) {
      if (fabsf(vec0[i] - vec1[i] / 6.0) > 1.0e-6) {
        printf("%.10e != %.10e\n", vec0[i], vec1[i] / 6.0);
        printf("Wrong results in decimate\n");
        return 1;
      }
    }
  }

  // test bandpass correction
  {
    const SigProc original("bandpass");

//    write_2d(convert_to_2d(original.GetData(), original.Header().nsamples,
//        original.Header().nchans), "bandpass.orig");

    util.CorrectBandpass(original, "bandpass_corr", 256 * 1024, 0.0);

    auto corr = read_2d("bandpass_corrected");

    const SigProc my_corr("bandpass_corr");
    auto my = convert_to_2d(my_corr.GetData(), original.Header().nsamples,
        original.Header().nchans);

//    write_2d(my, "bandpass_corr.dat");

    for (size_t i = 0; i < my.size(); ++i) {
      for (size_t j = 0; j < my[i].size(); ++j) {
        if (fabsf(my[i][j] - corr[i][j]) > 2.0e-7) {
          printf("Wrong results in bandpass correction\n");
          return 1;
        }
      }
    }

    // check bandpass values
    auto my_bp = read_columns("bandpass_corr.bandpass", 3)[2];
    auto bp = read_1d("bp.dat");

    if (!compare_results(bp, my_bp)) {
      printf("Wrong bandpass values\n");
      return 1;
    }
  }

  // test baseline removal
  {
    double baseline = 0.1;

    const SigProc inp("input.filterbank");

    util.RemoveBaseline(inp, "temp", baseline);
    const SigProc temp("temp");

    const SigProc out("output.filterbank");

//    for (size_t i = 0; i < inp.Data().size(); ++i) {
//      float diff = fabsf(inp.Data()[i] - out.Data()[i]);
//      if (diff > 3.0e-7) {
//        printf("%lu: %.6e != %.6e, diff = %.6e\n", i, inp.Data()[i],
//            out.Data()[i], diff);
//      }
//    }

    auto vec0 = temp.GetData();
    auto vec1 = out.GetData();

    for (size_t i = 0; i < vec0.size(); ++i) {
      if (fabsf(vec0[i] - vec1[i]) > 5.0e-7) {
//        printf("%lu: %.6e != %.6e\n", i, vec0[i], vec1[i]);
        printf("Wrong results in filterbank\n");
        return 1;
      }
    }
  }

  // test barycentering
  {
    std::string obs = "GS";

    const SigProc inp("bary_test.fil");

    util.BarycenterCorrect(inp, "temp", obs);
    const SigProc temp("temp");

//    for (size_t i = 0; i < inp.Data().size(); ++i) {
//      float diff = fabsf(inp.Data()[i] - out.Data()[i]);
//      if (diff > 3.0e-7) {
//        printf("%lu: %.6e != %.6e, diff = %.6e\n", i, inp.Data()[i],
//            out.Data()[i], diff);
//      }
//    }

    auto vec0 = temp.GetData();
    std::vector<float> vec1(vec0.size());

    FILE * f = fopen("bary_test_presto.dat", "rb");
    if (f == nullptr)
      throw std::runtime_error("Could not open bary_test_presto.dat");

    fread(vec1.data(), sizeof(float), vec0.size(), f);
    fclose(f);

    for (size_t i = 0; i < 1048475; ++i) {
      if (vec0[i] != vec1[i]) {
        printf("%lu: %.6e != %.6e\n", i, vec0[i], vec1[i]);
        printf("Wrong results in barycentering\n");
        return 1;
      }
    }
  }

  // test everything
  {
    const SigProc original("bandpass");

    util.AverageSamples(original, "out_avg", 3);

    const SigProc out_avg("out_avg");
    auto my_avg = convert_to_2d(out_avg.GetData(), out_avg.Header().nsamples,
        out_avg.Header().nchans);
    auto avg = read_2d("avg");

    if (!compare_results(my_avg, avg)) {
      printf("Wrong results in average\n");
      return 1;
    }

    util.Process(original, "out_bp", 3, 511, 0.0, 0.0, "");

    const SigProc out_bp("out_bp");
    auto my_bp = convert_to_2d(out_bp.GetData(), out_bp.Header().nsamples,
        out_bp.Header().nchans);
    auto bp = read_2d("bp");

    for (size_t i = 0; i < my_bp.size(); ++i) {
      for (size_t j = 0; j < my_bp[i].size(); ++j) {
        if (fabsf(my_bp[i][j] - my_bp[i][j]) > 2.0e-7) {
          printf("Wrong results in average and bandpass\n");
          return 1;
        }
      }
    }

    util.Process(original, "out_base", 3, 511, 0.0, 0.01, "");

    const SigProc out_base("out_base");
    auto my_base = convert_to_2d(out_base.GetData(), out_base.Header().nsamples,
        out_base.Header().nchans);
    auto base = read_2d("base");

    for (size_t i = 0; i < base.size(); ++i) {
      for (size_t j = 0; j < base[i].size(); ++j) {
        if (fabsf(my_base[i][j] - base[i][j]) > 1.0e-6) {
          printf("Wrong results in everything\n");
          return 1;
        }
      }
    }
  }

  return 0;
}
