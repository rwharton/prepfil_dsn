/*
 * SigProcHeader.hpp
 *
 *  Created on: Aug 15, 2016
 *      Author: jlippuner
 */

#ifndef SIGPROCHEADER_HPP_
#define SIGPROCHEADER_HPP_

#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

// from Heimdall
struct SigProcHeader {
  std::string source_name;
  std::string rawdatafile;
  double az_start;
  double za_start;
  double src_raj;
  double src_dej;
  double tstart;
  double tsamp;
//  double period;
  double fch1;
  double foff;
  int    nchans;
  int    telescope_id;
  int    machine_id;
  int    data_type;
  int    ibeam;
  int    nbeams;
  int    nbits;
  int    barycentric;
  int    pulsarcentric;
//  int    nbins;
  int    nsamples;
  int    nifs;
//  long int npuls;
//  double refdm;
//  unsigned char signed_data;

  size_t Input_size;

  SigProcHeader() :
    source_name(""),
    rawdatafile(""),
    az_start(0.0),
    za_start(0.0),
    src_raj(0.0),
    src_dej(0.0),
    tstart(0.0),
    tsamp(0.0),
//    period(0.0),
    fch1(0.0),
    foff(0.0),
    nchans(0),
    telescope_id(-1),
    machine_id(-1),
    data_type(0),
    ibeam(0),
    nbeams(0),
    nbits(0),
    barycentric(0),
    pulsarcentric(0),
//    nbins(0),
    nsamples(0),
    nifs(0),
//    npuls(0),
//    refdm(0.0),
//    signed_data(0),
    Input_size(0) {}

  static SigProcHeader Read(const int fd);

  void Write(const int fd) const;

  static SigProcHeader ReadStream(const std::istream& stm);

  void WriteStream(std::ostream& stm) const;

  size_t Data_size() const {
    if (nsamples == -1)
      return 0;

    return (size_t)nifs * (size_t)nchans * (size_t)nsamples
        * (size_t)(nbits / 8);
  }

  size_t Get_output_size() const;

  bool operator==(const SigProcHeader& other) const;

  bool operator!=(const SigProcHeader& other) const {
    return !(*this == other);
  }
};

#endif // SIGPROCHEADER_HPP_
