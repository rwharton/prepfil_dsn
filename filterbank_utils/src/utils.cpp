/*
 * utils.cpp
 *
 *  Created on: Jul 13, 2016
 *      Author: jlippuner
 */

#include "utils.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <boost/regex.hpp>

std::vector<float> read_1d(const std::string& file) {
  std::vector<float> dat;

  std::ifstream istm(file, std::ifstream::in);

  std::string line;
  std::getline(istm, line);
  std::size_t line_num = 1;

  while (line.size() > 0) {
    if (line[0] != '#') {
      float v;
      if (sscanf(line.c_str(), "%f", &v) != 1) {
        throw std::invalid_argument("Cannot line " + std::to_string(line_num));
      }
      dat.push_back(v);
    }

    std::getline(istm, line);
    ++line_num;
  }

  istm.close();

  return dat;
}

std::vector<std::vector<float>> read_2d(const std::string& file) {
  std::vector<std::vector<float>> rows;

  std::ifstream istm(file, std::ifstream::in);

  std::string line;
  std::getline(istm, line);
  std::size_t line_num = 1;

  while (line.size() > 0) {
    if (line[0] != '#') {
      // read line
      std::istringstream is(line);
      std::vector<float> row;

      std::size_t col_num = 1;
      while (!is.eof()) {
        float val;
        is >> val;

        if (is.fail()) {
//          printf("line: %s\n", line.c_str());
//          printf("val = %e", val);

          throw std::invalid_argument(
              "Cannot read column " + std::to_string(col_num) + " on line "
                  + std::to_string(line_num));
        }

        row.push_back(val);
        ++col_num;
      }

      if (rows.size() > 0) {
        if (row.size() != rows[0].size()) {
          throw std::invalid_argument(
              "Got different number of columns on line "
                  + std::to_string(line_num));
        }
      }

      rows.push_back(row);
    }

    std::getline(istm, line);
    ++line_num;
  }

  istm.close();

  return rows;
}

std::vector<std::vector<float>> read_columns(const std::string& file,
    const std::size_t num_columns) {
  std::vector<std::vector<float>> cols(num_columns);

  std::ifstream istm(file, std::ifstream::in);

  std::string line;
  std::getline(istm, line);
  std::size_t line_num = 1;

  while (line.size() > 0) {
    if (line[0] != '#') {
      // read line
      std::istringstream is(line);

      for (std::size_t i = 0; i < num_columns; ++i) {
        float val;
        is >> val;

        if (is.fail()) {
//          printf("line: %s\n", line.c_str());
//          printf("val = %e", val);

          throw std::invalid_argument(
              "Cannot read column " + std::to_string(i + 1) + " on line "
                  + std::to_string(line_num));
        }

        cols[i].push_back(val);
      }
    }

    std::getline(istm, line);
    ++line_num;
  }

  istm.close();

  return cols;
}

void write_1d(const std::vector<float>& data, const std::string& file) {
  FILE * fout = fopen(file.c_str(), "w");

  for (std::size_t i = 0; i < data.size(); ++i) {
    fprintf(fout, "%30.20e\n", data[i]);

  }

  fclose(fout);
}

void write_2d(const std::vector<std::vector<float>>& data,
    const std::string& file) {
  FILE * fout = fopen(file.c_str(), "w");

  for (std::size_t i = 0; i < data.size(); ++i) {
    for (std::size_t j = 0; j < data[i].size(); ++j) {
      fprintf(fout, "%30.20e", data[i][j]);
    }
    fprintf(fout, "\n");
  }

  fclose(fout);
}

std::vector<std::vector<float>> convert_to_2d(const std::vector<float>& dat,
    const std::size_t N, const std::size_t M) {
  if (dat.size() != N * M)
    throw std::invalid_argument("convert_to_2d: " + std::to_string(dat.size())
        + " != " + std::to_string(N) + " x " + std::to_string(M));

  std::vector<std::vector<float>> res(N, std::vector<float>(M));
  for (std::size_t i = 0; i < N; ++i)
    memcpy(res[i].data(), dat.data() + (i * M), M * sizeof(float));

  return res;
}

std::vector<std::vector<float>> convert_to_2d_strided(
    const std::vector<float>& dat, const std::size_t N, const std::size_t M,
    const std::size_t stride) {
  if (dat.size() != N * stride)
    throw std::invalid_argument("convert_to_2d: " + std::to_string(dat.size())
        + " != " + std::to_string(N) + " x " + std::to_string(stride));

  std::vector<std::vector<float>> res(N, std::vector<float>(M));
  for (std::size_t i = 0; i < N; ++i)
    memcpy(res[i].data(), dat.data() + (i * stride), M * sizeof(float));

  return res;
}

bool compare_results(const std::vector<std::vector<float>>& A,
    const std::vector<std::vector<float>>& B) {
  if (A.size() != B.size())
    return false;

  for (std::size_t i = 0; i < A.size(); ++i) {
    if (A[i].size() != B[i].size())
      return false;

    for (std::size_t j = 0; j < A[i].size(); ++j) {
//      if (2.0 * fabs(A[i][j] - B[i][j]) / (A[i][j] + B[i][j]) > 1.0e-4)
      if (!almost_equals(A[i][j], B[i][j])) {
//        printf("[%lu,%lu]: %.10e != %.10e\n", i, j, A[i][j], B[i][j]);
        return false;
      }
    }
  }

  return true;
}

bool compare_results(const std::vector<float>& A, const std::vector<float>& B) {
  if (A.size() != B.size())
    return false;

  for (std::size_t i = 0; i < A.size(); ++i) {
    if (!almost_equals(A[i], B[i])) {
//      printf("%lu: %.8e != %.8e\n", i, A[i], B[i]);
//    if (2.0 * fabs(A[i] - B[i]) / (A[i] + B[i]) > 1.0e-4)
      return false;
    }
  }

  return true;
}

double MakeMJD(const std::string& str) {
  // str = /some/path/2016-08-19-23:47:38
  boost::regex expr(
      ".*(\\d\\d\\d\\d)-(\\d\\d)-(\\d\\d)-(\\d\\d):(\\d\\d):(\\d\\d).*");

  boost::smatch match;
  if (!boost::regex_match(str, match, expr)) {
    return 0.0;
  } else {
    int year = atoi(match[1].str().c_str());
    int month = atoi(match[2].str().c_str());
    int day = atoi(match[3].str().c_str());
    int hour = atoi(match[4].str().c_str());
    int minute = atoi(match[5].str().c_str());
    int second = atoi(match[6].str().c_str());

    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;

    int jdn = day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400
        - 32045;

    double frac = (double)(hour - 12) / 24.0 + (double)minute / 1440.0
        + (double)second / 86400.0;

    return (double)jdn + frac - 2400000.5;
  }
}

std::string ToUpper(const std::string& str) {
  std::string res = str;
  std::transform(res.begin(), res.end(), res.begin(), toupper);
  return res;
}
