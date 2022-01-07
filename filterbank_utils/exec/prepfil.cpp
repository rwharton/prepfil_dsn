/*
 * prepfil.cpp
 *
 *  Created on: Aug 18, 2016
 *      Author: jlippuner
 */

#include <string>

#include "SigProcUtil.hpp"
#include "RFIMask.hpp"
#include "utils.hpp"

// including this at the beginning gives a lot of warnings
#include <argp.h>

const char *argp_program_bug_address = "<jonas@lippuner.ca>";

/* Program documentation. */
static char doc[] = "prepfil -- Prepares sigproc filterbank files for "
    "further processing. Supported actions are averaging samples, correcting "
    "bandpass, removing the baseline, and barycentering.";

/* A description of the arguments we accept. */
static char args_doc[] = "INPUT OUTPUT\nFILE";

#define NO_GPU 1
#define MAX_MEM 2
#define MAX_MEM_FRAC 3
#define MASK 4
#define HEADER_RA 5
#define HEADER_DEC 6
#define HEADER_FCH1 7
#define HEADER_SRC_NAME 8

/* Used by main to communicate with parse_opt. */
struct arguments {
  char *args[2]; // INPUT and OUTPUT
  int arg_num;
  int avg;
  double bp_min;
  double bp_smooth;
  double baseline;
  char *obs;
  bool no_gpu;
  long int max_mem;
  double max_mem_frac;
  char * mask;

  double ra, dec, fch1;
  char * src_name;
  bool set_ra, set_dec, set_fch1, set_src_name;
};

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  arguments *args = (arguments*)state->input;

  switch (key) {
  case 'a':
    args->avg = parse_int(arg);
    break;
  case 'p':
    args->bp_min = arg ? parse_double(arg) : 2.0;
    break;
  case 's':
    args->bp_smooth = parse_double(arg);
    break;
  case 'b':
    args->baseline = parse_double(arg);
    break;
  case 'o':
    args->obs = arg;
    break;
  case NO_GPU:
    args->no_gpu = true;
    break;
  case MAX_MEM:
    args->max_mem = parse_long_int(arg);
    break;
  case MAX_MEM_FRAC:
    args->max_mem_frac = parse_double(arg);
    break;
  case MASK:
    args->mask = arg;
    break;
  case HEADER_RA:
    args->ra = parse_double(arg);
    args->set_ra = true;
    break;
  case HEADER_DEC:
    args->dec = parse_double(arg);
    args->set_dec = true;
    break;
  case HEADER_FCH1:
    args->fch1 = parse_double(arg);
    args->set_fch1 = true;
    break;
  case HEADER_SRC_NAME:
    args->src_name = arg;
    args->set_src_name = true;
    break;

  case ARGP_KEY_ARG:
    if (state->arg_num >= 2)
      argp_usage(state); // too many args
    args->args[state->arg_num] = arg;
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

/* The options we understand. */
static argp_option options[] = {
  {"average",  'a', "NUM", 0,  "Average NUM samples" },
  {"bandpass", 'p', "MIN", OPTION_ARG_OPTIONAL,
      "Do bandpass correction using MIN minutes of data to estimate bandpass "
      "(default MIN = 2)" },
  {"bandpass_smooth",  's', "NUM", 0,
      "Smooth bandpass with smoothing constant NUM (measured in samples)" },
  {"baseline", 'b', "SEC", 0,
      "Remove baseline by removing all frequencies lower than 1 / SEC seconds"},
  {"obs",      'o', "CODE", 0, "Observatory CODE for barycentering" },
  {"no-gpu",   NO_GPU, 0,     0, "Don't use GPU for baseline removal" },
  {"max-mem",  MAX_MEM, "SIZE_MB", 0, "Use at most SIZE_MB megabytes of memory" },
  {"max-mem-frac", MAX_MEM_FRAC, "PERCENT", 0,
      "Use at most PERCENT % of the total system memory" },
  {"mask",     MASK, "FILE", 0, "Use the RFI mask MASK" },
  {"ra",  HEADER_RA, "HHMMSS.SSS", 0, "Set the source RA in the new header "
      "to HHMMSS.SSS"},
  {"dec", HEADER_DEC, "DDMMSS.SSS", 0, "Set the source DEC in the new header "
      "to DDMMSS.SSS"},
  {"fch1",HEADER_FCH1, "MHz", 0, "Set fch1 in the new header to MHz"},
  {"src-name",HEADER_SRC_NAME, "NAME", 0, "Set source name in the new header "
      "to NAME (will be truncated to 79 characters"},
  { 0 }
};

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };

#pragma GCC diagnostic pop


int main(int argc, char **argv) {
  arguments args;

  args.arg_num = 0;
  args.avg = 1;
  args.bp_min = 0.0;
  args.bp_smooth = 0.0;
  args.baseline = 0.0;
  args.obs = nullptr;
  args.no_gpu = false;
  args.max_mem = 0;
  args.max_mem_frac = 0.0;
  args.mask = nullptr;
  args.ra = 0.0;
  args.dec = 0.0;
  args.fch1 = 0.0;
  args.src_name = nullptr;
  args.set_ra = false;
  args.set_dec = false;
  args.set_fch1 = false;
  args.set_src_name = false;

  argp_parse(&argp, argc, argv, 0, 0, &args);

//  printf("avg = %i, bp = %f, base = %f\n", args.avg, args.bp_min, args.baseline);
  
//  printf("args.mask == %s\n", args.mask);
  
  bool do_processing = !((args.avg == 1) && (args.bp_min == 0.0)
      && (args.baseline == 0.0) && (args.obs == nullptr) && (args.mask == nullptr));
  bool mod_header = args.set_ra || args.set_dec || args.set_fch1
      || args.set_src_name;

  if (!do_processing && !mod_header) {
    printf("No action specified, exiting.\n");
    return 0;
  }

  if (do_processing && (args.arg_num != 2)) {
    printf("Cannot do requested processing without an INPUT and OUTPUT file "
        "specified\n");
    return 1;
  }

  if ((args.bp_smooth > 0.0) && (args.bp_min == 0.0)) {
    printf("Cannot smooth bandpass if bandpass correction is not requested.\n");
    return 1;
  }

  SigProcUtil util(args.max_mem * 1024, args.max_mem_frac, !args.no_gpu);

  if (do_processing) {
    std::string in_file(args.args[0]);
    std::string out_file(args.args[1]);
    std::string mask_file = args.mask == nullptr ? "" : std::string(args.mask);
    std::string obs = "";

    printf(" Input file: %s\n", in_file.c_str());
    printf("Output file: %s\n", out_file.c_str());
    printf("  Mask file: %s\n", mask_file != "" ? mask_file.c_str() : "<none>");

    printf("\nPerforming the following actions:\n");
    if (args.avg > 1)
      printf("  Averaging %i samples\n", args.avg);
    if (args.bp_min > 0.0)
      printf("  Bandpass correction using %.2f minutes to measure bandpass\n",
          args.bp_min);
    if (args.bp_smooth > 0.0)
      printf("  Smoothing bandpass with constant %.2f\n", args.bp_smooth);
    if (args.baseline > 0.0)
      printf("  Removing baseline using smoothing length of %.2f seconds\n",
          args.baseline);
    if (args.obs != nullptr) {
      obs = std::string(args.obs);
      printf("  Barycentering using observatory code %s\n", args.obs);
    }
    printf("\n");

    const SigProc inp(in_file);

    size_t num_bp = args.bp_min * 60.0 / inp.Header().tsamp;

    if (mask_file != "") {
      RFIMask mask(mask_file, inp.Header());
      util.SetMask(mask);
    }

    util.Process(inp, out_file, args.avg, num_bp, args.bp_smooth,
        args.baseline, obs);
  }

  if (mod_header) {
    std::string in_file, out_file;

    if (args.arg_num == 1) {
      in_file = std::string(args.args[0]);
      out_file = in_file;
    } else {
      out_file = std::string(args.args[1]);

      if (do_processing)
        in_file = out_file;
      else
        in_file = std::string(args.args[0]);
    }

    const SigProc inp(in_file);
    auto header = inp.Header();

    if (args.set_ra)
      header.src_raj = args.ra;
    if (args.set_dec)
      header.src_dej = args.dec;
    if (args.set_fch1)
      header.fch1 = args.fch1;
    if (args.set_src_name)
      header.source_name = std::string(args.src_name);

    util.ModifyHeader(in_file, out_file, header);
  }

  return 0;
}
