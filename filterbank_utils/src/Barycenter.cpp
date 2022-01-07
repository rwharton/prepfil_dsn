/*
 * Barycenter.cpp
 *
 *  Created on: Sep 20, 2016
 *      Author: jlippuner
 */

#include "Barycenter.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits.h>
#include <stdexcept>
#include <string>
#include <unistd.h>

// this is all copied from PRESTA and adapted a bit

namespace {

const double DayPerSec = 1.0 / (3600.0 * 24.0);

std::string RaDecToStr(const double val) {
  char str[128];

  bool negative = (val < 0.0);
  double absval = fabs(val);

  int deg = (int)absval / 10000;
  int min = ((int)absval / 100) % 100;
  double sec = absval - 100.0 * (double)min - 10000.0 * (double)deg;

  sprintf(str, "%1s%02i:%02i:%08.5f", negative ? "-" : " ", deg, min, sec);
  return std::string(str);
}

// copied from PRESTO
size_t chkfread(void *data, size_t type, size_t number, FILE * stream) {
  size_t num;

  num = fread(data, type, number, stream);
  if (num != number && ferror(stream)) {
    perror("\nError in chkfread()");
    printf("\n");
    exit(-1);
  }
  return num;
}

int read_resid_rec(FILE * file, double *toa, double *obsf)
/* This routine reads a single record (i.e. 1 TOA) from */
/* the file resid2.tmp which is written by TEMPO.       */
/* It returns 1 if successful, 0 if unsuccessful.       */
{
  static int firsttime = 1, use_ints = 0;
  static double d[9];

  // The default Fortran binary block marker has changed
  // several times in recent versions of g77 and gfortran.
  // g77 used 4 bytes, gfortran 4.0 and 4.1 used 8 bytes
  // and gfortrans 4.2 and higher use 4 bytes again.
  // So here we try to auto-detect what is going on.
  // The current version should be OK on 32- and 64-bit systems

  if (firsttime) {
    int ii;
    long long ll;
    double dd;

    chkfread(&ll, sizeof(long long), 1, file);
    chkfread(&dd, sizeof(double), 1, file);
    if (0)
      printf("(long long) index = %lld  (MJD = %17.10f)\n", ll, dd);
    if (ll != 72 || dd < 40000.0 || dd > 70000.0) { // 9 * doubles
      rewind(file);
      chkfread(&ii, sizeof(int), 1, file);
      chkfread(&dd, sizeof(double), 1, file);
      if (0)
        printf("(int) index = %d    (MJD = %17.10f)\n", ii, dd);
      if (ii == 72 && (dd > 40000.0 && dd < 70000.0)) {
        use_ints = 1;
      } else {
        throw std::runtime_error("Error: Can't read the TEMPO residuals "
            "correctly!");
      }
    }
    rewind(file);
    firsttime = 0;
  }
  if (use_ints) {
    int ii;
    chkfread(&ii, sizeof(int), 1, file);
  } else {
    long long ll;
    chkfread(&ll, sizeof(long long), 1, file);
  }
  //  Now read the rest of the binary record
  chkfread(&d, sizeof(double), 9, file);
  if (0) {                    // For debugging
    printf("Barycentric TOA = %17.10f\n", d[0]);
    printf("Postfit residual (pulse phase) = %g\n", d[1]);
    printf("Postfit residual (seconds) = %g\n", d[2]);
    printf("Orbital phase = %g\n", d[3]);
    printf("Barycentric Observing freq = %g\n", d[4]);
    printf("Weight of point in the fit = %g\n", d[5]);
    printf("Timing uncertainty = %g\n", d[6]);
    printf("Prefit residual (seconds) = %g\n", d[7]);
    printf("??? = %g\n\n", d[8]);
  }
  *toa = d[0];
  *obsf = d[4];
  if (use_ints) {
    int ii;
    return chkfread(&ii, sizeof(int), 1, file);
  } else {
    long long ll;
    return chkfread(&ll, sizeof(long long), 1, file);
  }
}

}

void Barycenter::GetBarycenterTimes(const double * const topoTimes,
    double * const baryTimes, const size_t N, const char * const raStr,
    const char * const decStr, const char * const obs,
    const char * const ephem) {
  char tmpdir[] = "/tmp/filterbank_utils_XXXXXX";
  if (mkdtemp(tmpdir) == nullptr) {
    throw std::runtime_error("Could not create temp dir");
  }
  auto origdir = getcwd(nullptr, 0);
  chdir(tmpdir);

  try {
    FILE * outfile = fopen("bary.in", "w");
    if (outfile == nullptr)
      throw std::runtime_error("Could not create /tmp/bary.in");

    fprintf(outfile, "C  Header Section\n"
        "  HEAD                    \n"
        "  PSR                 bary\n"
        "  NPRNT                  2\n"
        "  P0                   1.0 1\n"
        "  P1                   0.0\n"
        "  CLK            UTC(NIST)\n"
        "  PEPOCH           %19.13f\n"
        "  COORD              J2000\n"
        "  RA                    %s\n"
        "  DEC                   %s\n"
        "  DM                   0.0\n"
        "  EPHEM                 %s\n"
        "C  TOA Section (uses ITAO Format)\n"
        "C  First 8 columns must have + or -!\n"
        "  TOA\n", topoTimes[0], raStr, decStr, ephem);

    for (size_t i = 0; i < N; i++) {
      fprintf(outfile, "topocen+ %19.13f  0.00     0.0000  0.000000  %s\n",
          topoTimes[i], obs);
    }

    fprintf(outfile, "topocen+ %19.13f  0.00     0.0000  0.000000  %s\n",
        topoTimes[N - 1] + 10.0 / (3600.0 * 24.0), obs);
    fprintf(outfile, "topocen+ %19.13f  0.00     0.0000  0.000000  %s\n",
        topoTimes[N - 1] + 20.0 / (3600.0 * 24.0), obs);
    fclose(outfile);

    std::string cmd = "tempo bary.in > /dev/null";
    if (system(cmd.c_str()) == -1) {
      throw std::runtime_error("Running TEMPO failed");
    }

    FILE * fin = fopen("resid2.tmp", "rb");
    if (fin == nullptr)
      throw std::runtime_error("Could not read resid2.tmp");

    double dummy;
    for (size_t i = 0; i < N; i++) {
      read_resid_rec(fin, baryTimes + i, &dummy);
    }
    fclose(fin);
  } catch (std::exception& ex) {
    chdir(origdir);
    free(origdir);
    throw ex;
  }

  chdir(origdir);
  free(origdir);
}

/* Simple linear interpolation macro */
#define LININTERP(X, xlo, xhi, ylo, yhi) ((ylo)+((X)-(xlo))*((yhi)-(ylo))/((xhi)-(xlo)))

/* Round a double or float to the nearest integer. */
/* x.5s get rounded away from zero.                */
#define NEAREST_LONG(x) (long) (x < 0 ? ceil(x - 0.5) : floor(x + 0.5))

Barycenter::Barycenter(const double tsampInSec, const double tstartMJD,
    const size_t num, const double ra, const double dec,
    const std::string obs) {
  const double barycenterStep = 20.0;
  int numbarypts =
      (tsampInSec * (double)num * 1.1 / barycenterStep + 5.5) + 1;

  /* What ephemeris will we use?  (Default is DE405) */
  const char * const ephem = "DE405";

  /* Define the RA and DEC of the observation */
  auto RAStr = RaDecToStr(ra);
  auto DecStr = RaDecToStr(dec);

  /* Allocate some arrays */
  std::vector<double> btoa(numbarypts);
  std::vector<double> ttoa(numbarypts);

  const double secPerDay = 3600.0 * 24.0;
  for (int i = 0; i < numbarypts; ++i)
    ttoa[i] = tstartMJD + barycenterStep * i / secPerDay;

  /* Call TEMPO for the barycentering */
  GetBarycenterTimes(ttoa.data(), btoa.data(), numbarypts, RAStr.c_str(), 
      DecStr.c_str(), obs.c_str(), ephem);

  mBaryStartMJD = btoa[0];

  /* Convert the bary TOAs to differences from the topo TOAs in  */
  /* units of bin length (dsdt) rounded to the nearest integer.  */
  double dtmp = (btoa[0] - ttoa[0]);
  for (int i = 0; i < numbarypts; ++i)
    btoa[i] = ((btoa[i] - ttoa[i]) - dtmp) * secPerDay / tsampInSec;

  int numdiffbins = abs(NEAREST_LONG(btoa[numbarypts - 1])) + 1;
  mDiffbins.resize(numdiffbins);
  int * diffbinptr = nullptr;

  /* Find the points where we need to add or remove bins */

  int oldbin = 0, currentbin;
  double lobin, hibin, calcpt;

  diffbinptr = mDiffbins.data();

  for (int i = 1; i < numbarypts; ++i) {
    currentbin = NEAREST_LONG(btoa[i]);
    if (currentbin != oldbin) {
      if (currentbin > 0) {
        calcpt = oldbin + 0.5;
        lobin = (i - 1) * barycenterStep / tsampInSec;
        hibin = i * barycenterStep / tsampInSec;
      } else {
        calcpt = oldbin - 0.5;
        lobin = -((i - 1) * barycenterStep / tsampInSec);
        hibin = -(i * barycenterStep / tsampInSec);
      }
      while (fabs(calcpt) < fabs(btoa[i])) {
        /* Negative bin number means remove that bin */
        /* Positive bin number means add a bin there */
        *diffbinptr = NEAREST_LONG(
            LININTERP(calcpt, btoa[i - 1], btoa[i], lobin, hibin));
        diffbinptr++;
        calcpt = (currentbin > 0) ? calcpt + 1.0 : calcpt - 1.0;
      }
      oldbin = currentbin;
    }
  }
  *diffbinptr = (int)num; /* Used as a marker */
}

void Barycenter::DoBarycenterCorrection(const float * const datIn,
    float * const datOut, const size_t numOut) {
  int num = (int)numOut;
  /* The number of data points to work with at a time */
  int worklen = 8 * 1024;
  if (worklen > num)
    worklen = num;
  worklen = (worklen / 1024) * 1024;

  /* Allocate our data array */
  std::vector<float> outdata(worklen);

  int numread = 0;
  int numadded = 0;
  int numremoved = 0;

  int totwrote = 0;
  int datawrote = 0;
  int numtowrite = 0;

  int inputIdx = 0;
  int outputIdx = 0;

  int * diffbinptr = mDiffbins.data();

  do { /* Loop to read and write the data */
    int numwritten = 0;

    numread = worklen;
    if (inputIdx + numread > num)
      numread = num - inputIdx;
    memcpy(outdata.data(), datIn + inputIdx, numread * sizeof(float));
    inputIdx += numread;

    if (numread == 0)
      break;

    /* Determine the approximate local average */
    double block_avg = 0.0;
    for (int i = 0; i < numread; ++i)
      block_avg += outdata[i];
    block_avg /= (double)numread;

    /* Simply write the data if we don't have to add or */
    /* remove any bins from this batch.                 */
    /* OR write the amount of data up to cmd->numout or */
    /* the next bin that will be added or removed.      */

    numtowrite = abs(*diffbinptr) - datawrote;
    /* FIXME: numtowrite+totwrote can wrap! */
    if ((totwrote + numtowrite) > num)
      numtowrite = num - totwrote;
    if (numtowrite > numread)
      numtowrite = numread;

    memcpy(datOut + outputIdx, outdata.data(), numtowrite * sizeof(float));
    outputIdx += numtowrite;

    datawrote += numtowrite;
    totwrote += numtowrite;
    numwritten += numtowrite;

    if ((datawrote == abs(*diffbinptr)) && (numwritten != numread)
        && (totwrote < num)) { /* Add/remove a bin */
      float favg;
      int skip, nextdiffbin;

      skip = numtowrite;

      do { /* Write the rest of the data after adding/removing a bin  */
        if (*diffbinptr > 0) {
          /* Add a bin */
          favg = (float)block_avg;

          datOut[outputIdx] = favg;
          ++outputIdx;

          numadded++;
          totwrote++;
        } else {
          /* Remove a bin */
          numremoved++;
          datawrote++;
          numwritten++;
          skip++;
        }
        diffbinptr++;

        /* Write the part after the diffbin */

        numtowrite = numread - numwritten;
        if ((totwrote + numtowrite) > num)
          numtowrite = num - totwrote;
        nextdiffbin = abs(*diffbinptr) - datawrote;
        if ((int)numtowrite > nextdiffbin)
          numtowrite = nextdiffbin;

        memcpy(datOut + outputIdx, outdata.data() + skip,
            numtowrite * sizeof(float));
        outputIdx += numtowrite;

        numwritten += numtowrite;
        datawrote += numtowrite;
        totwrote += numtowrite;

        skip += numtowrite;

        /* Stop if we have written out all the data we need to */

        if (totwrote == num)
          break;
      } while (numwritten < numread);
    }
    /* Stop if we have written out all the data we need to */

    if (totwrote == num)
      break;

  } while (numread);

  // pad with 0's if necessary
  if (num > totwrote) {
    int numPad = num - totwrote;
    memset(datOut + outputIdx, 0, numPad * sizeof(float));
  }
}
