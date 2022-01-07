/*
 * SigProcHeader.cpp
 *
 *  Created on: Aug 15, 2016
 *      Author: jlippuner
 */

#include "SigProcHeader.hpp"

#include <cstring>
#include <sstream>
#include <stdexcept>

#ifndef _LARGEFILE64_SOURCE
  #define _LARGEFILE64_SOURCE 1
#endif
#include <errno.h>
#include <unistd.h>

namespace {

void write_string(std::ostream& stm, const std::string str, const bool pad) {
  int in_len = str.size();

  if (in_len >= 80)
    in_len = 79;

  if ((in_len <= 0) && (!pad))
    throw std::runtime_error("Got string of invalid length in write_string "
        "(len = " + std::to_string(in_len) + ")");

  char buf[80];
  memset(buf, 0, 80 * sizeof(char));

  int out_len = in_len;

  if (pad)
    // always write strings of 80 character so that the string can grow later on
    out_len = 79;

  memcpy(buf, str.c_str(), in_len * sizeof(char));

  stm.write((char*)&out_len, sizeof(int));
  stm.write(buf, out_len * sizeof(char));
}

template<typename T>
T read_header(const int fd) {
  T val;
  if ((size_t)read(fd, &val, sizeof(T)) != sizeof(T)) {
    perror("Failure in read");
    throw std::runtime_error("Failed to read value");
  }
  return val;
}

std::string read_string(const int fd) {
  int len = read_header<int>(fd);
  char buf[80];

  if ((len <= 0) || (len >= 80))
    throw std::runtime_error("Got string of invalid length in read_string "
        "(len = " + std::to_string(len) + ")");

  if ((size_t)read(fd, buf, len * sizeof(char)) != len * sizeof(char)) {
    perror("Failure in read_string");
    throw std::runtime_error("Failed to read string");
  }

  buf[len] = '\0';
  return std::string(buf);
}

template<typename T>
void write_header(std::ostream& stm, const std::string name, const T val) {
  write_string(stm, name, false);
  stm.write((char*)&val, sizeof(T));
}

} // namespace [unnamed]

SigProcHeader SigProcHeader::Read(const int fd) {
  SigProcHeader header;

  // rewind
  if (lseek64(fd, 0, SEEK_SET) != 0) {
    perror("Failure in SigProcHeader::Read");
    throw std::runtime_error("Failed to seek");
  }

  std::string s = read_string(fd);
  if (s != "HEADER_START")
    throw std::invalid_argument("Not valid sigproc header");

  while (true) {
    s = read_string(fd);

    if (s == "HEADER_END")
      break;
    else if (s == "source_name")
      header.source_name = read_string(fd);
    else if (s == "rawdatafile")
      header.rawdatafile = read_string(fd);
    else if (s == "az_start")
      header.az_start = read_header<double>(fd);
    else if (s == "za_start")
      header.za_start = read_header<double>(fd);
    else if (s == "src_raj")
      header.src_raj = read_header<double>(fd);
    else if (s == "src_dej")
      header.src_dej = read_header<double>(fd);
    else if (s == "tstart")
      header.tstart = read_header<double>(fd);
    else if (s == "tsamp")
      header.tsamp = read_header<double>(fd);
    else if (s == "period")
      /*header.period =*/ read_header<double>(fd);
    else if (s == "fch1")
      header.fch1 = read_header<double>(fd);
    else if (s == "foff")
      header.foff = read_header<double>(fd);
    else if (s == "nchans")
      header.nchans = read_header<int>(fd);
    else if (s == "telescope_id")
      header.telescope_id = read_header<int>(fd);
    else if (s == "machine_id")
      header.machine_id = read_header<int>(fd);
    else if (s == "data_type")
      header.data_type = read_header<int>(fd);
    else if (s == "ibeam")
      header.ibeam = read_header<int>(fd);
    else if (s == "nbeams")
      header.nbeams = read_header<int>(fd);
    else if (s == "nbits")
      header.nbits = read_header<int>(fd);
    else if (s == "barycentric")
      header.barycentric = read_header<int>(fd);
    else if (s == "pulsarcentric")
      header.pulsarcentric = read_header<int>(fd);
    else if (s == "nbins")
      /*header.nbins =*/ read_header<int>(fd);
    else if (s == "nsamples")
      header.nsamples = read_header<int>(fd);
    else if (s == "nifs")
      header.nifs = read_header<int>(fd);
    else if (s == "npuls")
      /*header.npuls =*/ read_header<long int>(fd);
    else if (s == "refdm")
      /*header.refdm =*/ read_header<double>(fd);
    else if (s == "signed")
      /*header.signed_data =*/ read_header<unsigned char>(fd);
    else if ((s == "FREQUENCY_START") || (s == "FREQUENCY_END")
        || (s == "fchannel"))
      throw std::runtime_error("Frequency tables are not implemented");
    else
      throw std::runtime_error("Unknown header field '" + s + "'");
  }

  if ((header.data_type != 1) && (header.data_type != 2))
    throw std::invalid_argument("Can only read filterbank and time "
        "series sigproc files");

  if (header.data_type == 2)
    header.nchans = 1;

  if (header.nsamples == 0)
    header.nsamples = -1; // to be determined by file size

  off64_t size = lseek64(fd, 0, SEEK_CUR);
  if (errno != 0) {
    perror("Failure in SigProcHeader::Read");
    throw std::runtime_error("Failed to seek end of header");
  }

  header.Input_size= size;

  return header;
}

void SigProcHeader::Write(const int fd) const {
  std::stringstream stm(std::string(2048, ' '));
  WriteStream(stm);
  size_t len = stm.tellp();

  stm.seekg(0, std::ios::beg);
  char buf[len];
  stm.read(buf, len);

  if ((size_t)write(fd, buf, len) != len) {
    perror("Failure in SigProcHeader::Write");
    throw std::runtime_error("Failed to write header");
  }
}

void SigProcHeader::WriteStream(std::ostream& stm) const {
  write_string(stm, "HEADER_START", false);
  write_string(stm, "source_name", false);
  write_string(stm, source_name, true);
  write_string(stm, "rawdatafile", false);
  write_string(stm, rawdatafile, true);
  write_header(stm, "az_start", az_start);
  write_header(stm, "za_start", za_start);
  write_header(stm, "src_raj", src_raj);
  write_header(stm, "src_dej", src_dej);
  write_header(stm, "tstart", tstart);
  write_header(stm, "tsamp", tsamp);
//  write_header(fd, "period", period);
  write_header(stm, "fch1", fch1);
  write_header(stm, "foff", foff);
  write_header(stm, "nchans", nchans);
  write_header(stm, "telescope_id", telescope_id);
  write_header(stm, "machine_id", machine_id);
  write_header(stm, "data_type", data_type);
  write_header(stm, "ibeam", ibeam);
  write_header(stm, "nbeams", nbeams);
  write_header(stm, "nbits", nbits);
  write_header(stm, "barycentric", barycentric);
  write_header(stm, "pulsarcentric", pulsarcentric);
//  write_header(fd, "nbins", nbins);
  write_header(stm, "nsamples", nsamples);
  write_header(stm, "nifs", nifs);
//  write_header(fd, "npuls", npuls);
//  write_header(fd, "refdm", refdm);
//  write_header(fd, "signed", signed_data);
  write_string(stm, "HEADER_END", false);
}

size_t SigProcHeader::Get_output_size() const {
  std::ostringstream stm(std::string(2048, ' '));
  WriteStream(stm);
  return stm.tellp();
}

bool SigProcHeader::operator==(const SigProcHeader& other) const {
  if (source_name != other.source_name) return false;
  if (rawdatafile != other.rawdatafile) return false;
  if (az_start != other.az_start) return false;
  if (za_start != other.za_start) return false;
  if (src_raj != other.src_raj) return false;
  if (src_dej != other.src_dej) return false;
  if (tstart != other.tstart) return false;
  if (tsamp != other.tsamp) return false;
//  if (period != other.period) return false;
  if (fch1 != other.fch1) return false;
  if (foff != other.foff) return false;
  if (nchans != other.nchans) return false;
  if (telescope_id != other.telescope_id) return false;
  if (machine_id != other.machine_id) return false;
  if (data_type != other.data_type) return false;
  if (ibeam != other.ibeam) return false;
  if (nbeams != other.nbeams) return false;
  if (nbits != other.nbits) return false;
  if (barycentric != other.barycentric) return false;
  if (pulsarcentric != other.pulsarcentric) return false;
//  if (nbins != other.nbins) return false;
  if (nsamples != other.nsamples) return false;
  if (nifs != other.nifs) return false;
//  if (npuls != other.npuls) return false;
//  if (refdm != other.refdm) return false;
//  if (signed_data != other.signed_data) return false;
  return true;
}
