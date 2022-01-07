/*
 * utils.hpp
 *
 *  Created on: Jul 13, 2016
 *      Author: jlippuner
 */

#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>

// TODO what about 0?
inline int log2(const std::size_t n) {
  // find highest bit set
  int b = 0;
  std::size_t v = n;
  while (v >>= 1) ++b;
  return b;
}

inline int log2(const uint n) {
  // find highest bit set
  int b = 0;
  std::size_t v = n;
  while (v >>= 1) ++b;
  return b;
}

inline int log2(const int n) {
  // find highest bit set
  int b = 0;
  std::size_t v = n;
  while (v >>= 1) ++b;
  return b;
}

inline std::size_t next_smaller_pow2(const std::size_t n) {
  return (std::size_t)1 << log2(n);
}

inline std::size_t next_larger_pow2(const std::size_t n) {
  // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
  std::size_t v = n;

  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v |= v >> 32;
  v++;

  if (v == 0)
    v = 1;

  return v;
}

// http://stackoverflow.com/a/17467/2998298
inline bool almost_equals(const float A, const float B, const int maxUlps = 4) {
  union FloatInt {
    float F;
    int I;
  };

  FloatInt fiA, fiB;
  fiA.F = A;
  fiB.F = B;

  // Make sure maxUlps is non-negative and small enough that the
  // default NAN won't compare as equal to anything.
  assert(maxUlps > 0 && maxUlps < 4 * 1024 * 1024);
  int aInt = fiA.I;
  // Make aInt lexicographically ordered as a twos-complement int
  if (aInt < 0)
    aInt = 0x80000000 - aInt;
  // Make bInt lexicographically ordered as a twos-complement int
  int bInt = fiB.I;
  if (bInt < 0)
    bInt = 0x80000000 - bInt;
  int intDiff = abs(aInt - bInt);
  if (intDiff <= maxUlps)
    return true;
  return false;
}

std::vector<float> read_1d(const std::string& file);

std::vector<std::vector<float>> read_2d(const std::string& file);

std::vector<std::vector<float>> read_columns(const std::string& file,
    const std::size_t num_columns);

void write_1d(const std::vector<float>& data, const std::string& file);

void write_2d(const std::vector<std::vector<float>>& data,
    const std::string& file);

std::vector<std::vector<float>> convert_to_2d(const std::vector<float>& dat,
    const std::size_t N, const std::size_t M);

std::vector<std::vector<float>> convert_to_2d_strided(
    const std::vector<float>& dat, const std::size_t N, const std::size_t M,
    const std::size_t stride);

bool compare_results(const std::vector<std::vector<float>>& A,
    const std::vector<std::vector<float>>& B);

bool compare_results(const std::vector<float>& A, const std::vector<float>& B);

template<typename T>
T median5(T a, T b, T c, T d, T e) {
  if (b < a) std::swap(a, b);
  if (e < d) std::swap(d, e);
  if (d < a) { std::swap(a, d); std::swap(e, b); }

  if (b < c) {
    if (b < d)
      return std::min(c, d);
    else
      return std::min(b, e);
  } else {
    if (d < c)
      return std::min(c, e);
    else
      return std::min(b, d);
  }
}

double MakeMJD(const std::string& str);

std::string ToUpper(const std::string& str);

inline int parse_int(const char * const str) {
  std::string string(str);
  try {
    return std::stoi(string);
  } catch (std::invalid_argument& ex) {
    throw std::invalid_argument("Cannot parse '" + string + "' as integer");
  } catch (std::exception& ex) {
    throw ex;
  }
}

inline double parse_double(const char * const str) {
  std::string string(str);
  try {
    return std::stod(string);
  } catch (std::invalid_argument& ex) {
    throw std::invalid_argument("Cannot parse '" + string + "' as double");
  } catch (std::exception& ex) {
    throw ex;
  }
}

inline int parse_long_int(const char * const str) {
  std::string string(str);
  try {
    return std::stol(string);
  } catch (std::invalid_argument& ex) {
    throw std::invalid_argument("Cannot parse '" + string
        + "' as long integer");
  } catch (std::exception& ex) {
    throw ex;
  }
}

#endif // UTILS_HPP_
