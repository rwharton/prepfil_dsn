/*
 * BaselineRemover_NO_CPU.cpp
 *
 *  Created on: Aug 17, 2016
 *      Author: jlippuner
 */

#include "BaselineRemover.hpp"

#include <stdexcept>

bool BaselineRemover::CPU_Available() {
  return false;
}

void BaselineRemover::CPU_Init(const size_t /*total_num_channels*/) {
  throw std::runtime_error("CPU_Init not implemented");
}

void BaselineRemover::CPU_Process_batch(float * const /*data*/,
    const size_t /*num_channels*/) {
  throw std::runtime_error("CPU_Process_batch not implemented");
}

size_t BaselineRemover::CPU_Ram_per_channel() const {
  throw std::runtime_error("CPU_Ram_per_channel not implemented");
}
