/*
 * mkfb_dir.cpp
 *
 *  Created on: Sep 07, 2016
 *      Author: jlippuner
 */

#include <cstdio>
#include <memory>
#include <stdexcept>

#include <boost/filesystem.hpp>

#include "MakeFilterbank.hpp"
#include "ScanFile.hpp"
#include "PulsarCatalog.hpp"
#include "utils.hpp"

// including this at the beginning gives a lot of warnings
#include <argp.h>

const char *argp_program_bug_address = "<jonas@lippuner.ca>";

/* Program documentation. */
static char doc[] = "mkfb -- Make filterbank files from dada files";

/* A description of the arguments we accept. */
static char args_doc[] = "CONF_FILE INPUT_DIR OUTPUT_DIR";

/* Used by main to communicate with parse_opt. */
struct arguments {
  char * conf_file;
  char * input_dir;
  char * output_dir;
  int arg_num;

  char * tag;
  char * scan_file;
  char * pulsars_file;

  bool batch, monitor;

  int scan_num;
  int num_skip;
  int num_proc;

  bool set_scan_num, set_num_skip, set_num_proc;
};

#define ARG_SKIP 1
#define ARG_NUM 2

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

/* The options we understand. */
static argp_option options[] = {
  {"tag",       't', "OUTPUT_PREFIX", 0,  "Prefix for the output files" },
  {"scan-file", 's', "SCAN_FILE",     0,  "Path to scan file" },
  {"pcat",      'p', "PULSARS_FILE",  0,  "Path to file containing pulsars" },
  {"batch",     'b', 0,               0,
      "Run in batch mode (process all dada files and exit)" },
  {"monitor",   'm', 0,               0,  "Run in monitor mode (monitor input "
      "directory and process dada files as they appear until terminated)" },
  {"scan-num",  'n', "SCAN_NUMBER",   0,  "The scan number to process" },
  {"skip", ARG_SKIP, "NUM",           0,  "Skip this many dada files" },
  {"num",   ARG_NUM, "NUM",           0,  "Process this many dada files" },
  { 0 }
};

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  arguments *args = (arguments*)state->input;

  switch (key) {
  case 't':
    args->tag = arg;
    break;
  case 's':
    args->scan_file = arg;
    break;
  case 'p':
    args->pulsars_file = arg;
    break;
  case 'b':
    args->batch = true;
    break;
  case 'm':
    args->monitor = true;
    break;
  case 'n':
    args->scan_num = parse_int(arg);
    args->set_scan_num = true;
    break;
  case ARG_SKIP:
    args->num_skip = parse_int(arg);
    args->set_num_skip = true;
    break;
  case ARG_NUM:
    args->num_proc = parse_int(arg);
    args->set_num_proc = true;
    break;

  case ARGP_KEY_ARG:
    if (state->arg_num >= 3)
      argp_usage(state); // too many args

    if (state->arg_num == 0)
      args->conf_file = arg;
    else if (state->arg_num == 1)
      args->input_dir = arg;
    else if (state->arg_num == 2)
      args->output_dir = arg;

    ++args->arg_num;
    break;

  case ARGP_KEY_END:
    if (state->arg_num < 1)
      argp_usage(state); // too few args
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };

#pragma GCC diagnostic pop

void ProcessScan(const int scan_num, const ScanFile * const pScans,
    const PulsarCatalog * const pPCat, const MakeFilterbankConfig& rootConfig,
    const std::string input, const std::string rootOutput,
    const std::string tag, const arguments args) {
  std::string prefix = "";
  std::string output = rootOutput;
  auto config = rootConfig;

  // use info from scan file, if available
  if (pScans != nullptr) {
    if (pScans->Scans().count(scan_num) == 0)
      throw std::invalid_argument(
          "There is no scan number " + std::to_string(scan_num));

    auto scan = pScans->Scans().at(scan_num);

    if (config.Observation.Start_MJD == 0.0)
      config.Observation.Start_MJD = scan.MJD;

    if (config.Observation.SampleTime_us == 0.0)
      config.Observation.SampleTime_us = scan.SampleTimeMicroSec;

    if (config.Source.Name == "")
      config.Source.Name = scan.Target;

    prefix = scan.Prefix;
    output = output + "/" + scan.OutDir();
  }

  if ((config.Source.Name != "") && (pPCat != nullptr)) {
    if (pPCat->Pulsars().count(config.Source.Name) == 0) {
      printf("Could not find source '%s' in pulsar list\n",
          config.Source.Name.c_str());
    } else {
      if (config.Source.RA == 0.0)
        config.Source.RA = pPCat->Pulsars().at(config.Source.Name).RA;
      if (config.Source.Dec == 0.0)
        config.Source.Dec = pPCat->Pulsars().at(config.Source.Name).Dec;
    }
  }

  MakeFilterbank make(config);
  output += "/" + tag;

  if (args.monitor)
    make.MonitorDirectory(input, output, prefix, args.num_skip, args.num_proc);
  else if (args.batch)
    make.ProcessDirectory(input, output, prefix, args.num_skip, args.num_proc);
  else
    throw std::runtime_error("Nothing to do");
}

int main(int argc, char ** argv) {
  arguments args;
  args.conf_file = nullptr;
  args.input_dir = nullptr;
  args.output_dir = nullptr;

  args.tag = nullptr;
  args.scan_file = nullptr;
  args.pulsars_file = nullptr;

  args.batch = false;
  args.monitor = false;

  args.scan_num = 0;
  args.num_skip = 0;
  args.num_proc = 0;

  args.set_scan_num = false;
  args.set_num_skip = false;
  args.set_num_proc = false;

  argp_parse(&argp, argc, argv, 0, 0, &args);

  if (args.batch == args.monitor)
    throw std::invalid_argument("Must specify either batch or monitor mode");

  if (args.conf_file == nullptr)
    throw std::invalid_argument("No config file specified");

  if (args.input_dir == nullptr)
    throw std::invalid_argument("No input directory specified");

  if (args.output_dir == nullptr)
    throw std::invalid_argument("No output directory specified");

  std::string conf(args.conf_file);
  std::string input(args.input_dir);
  std::string output(args.output_dir);

  if (!boost::filesystem::exists(conf)
      || !boost::filesystem::is_regular_file(conf))
    throw std::invalid_argument(
        "Configuration file '" + conf + "' does not exist");

  if (!boost::filesystem::exists(input)
      || !boost::filesystem::is_directory(input))
    throw std::invalid_argument(
        "Input directory '" + input + "' does not exist");

  if (boost::filesystem::exists(output)
      && !boost::filesystem::is_directory(output))
    throw std::invalid_argument(
        "Output directory '" + output + "' exists but it is not a directory");

  std::string tag = (args.tag == nullptr) ? "" : std::string(args.tag);
  std::string scanFile =
      (args.scan_file == nullptr) ? "" : std::string(args.scan_file);
  std::string pulsars =
      (args.pulsars_file == nullptr) ? "" : std::string(args.pulsars_file);

  if (scanFile != "") {
    if (!boost::filesystem::exists(scanFile)
        || !boost::filesystem::is_regular_file(scanFile))
      throw std::invalid_argument(
          "Scan file '" + scanFile + "' does not exist");
  }

  if (pulsars != "") {
    if (!boost::filesystem::exists(pulsars)
        || !boost::filesystem::is_regular_file(pulsars))
      throw std::invalid_argument(
          "Pulsars file '" + pulsars + "' does not exist");
  }

  if (args.set_scan_num && (scanFile == ""))
    throw std::invalid_argument("Scan number specified but no scan file given");

  if (args.set_num_skip && (args.num_skip < 0))
    throw std::invalid_argument("Skip number must be non-negative");

  if (args.set_num_proc && (args.num_proc <= 0))
    throw std::invalid_argument("Number of files to process must be positive");

  // done checking input parameters

  auto config = MakeFilterbankConfig::Read(conf);

  std::unique_ptr<PulsarCatalog> pPCat;
  if (pulsars != "")
    pPCat = std::unique_ptr<PulsarCatalog>(new PulsarCatalog(pulsars));

  std::unique_ptr<ScanFile> pScans;
  if (scanFile != "")
    pScans = std::unique_ptr<ScanFile>(new ScanFile(scanFile));

  std::string prefix = "";
  std::string thisOutput = "";

  if (args.set_scan_num) {
    // a scan number is given, process just this scan
    ProcessScan(args.scan_num, pScans.get(), pPCat.get(), config, input,
        output, tag, args);
  } else {
    if (args.batch && (pScans.get() != nullptr)) {
      // if this is a batch job and we have a scan file, process all scans
      printf("Processing all scans:\n");
      for (auto s : pScans->Scans())
        printf("% 2i: source: %16s, prefix: %s, output: %s\n", s.first,
            s.second.Target.c_str(), s.second.Prefix.c_str(),
            s.second.OutDir().c_str());
      printf("\n");

      for (auto s : pScans->Scans())
        ProcessScan(s.second.Num, pScans.get(), pPCat.get(), config, input,
            output, tag, args);
    } else {
      ProcessScan(-1, nullptr, pPCat.get(), config, input, output, tag, args);
    }
  }

  return 0;
}
