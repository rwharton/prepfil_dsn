/*
 * BaselineRemover_NO_GPU.cpp
 *
 *  Created on: Aug 17, 2016
 *      Author: jlippuner
 */

#include "BaselineRemover.hpp"

#include <stdexcept>

bool BaselineRemover::GPU_Available() {
  return false;
}

void BaselineRemover::GPU_Init(const size_t /*total_num_channels*/) {
  throw std::runtime_error("GPU_Init not implemented");
}

void BaselineRemover::GPU_Process_batch(float * const /*data*/,
    const size_t /*num_channels*/) {
  throw std::runtime_error("GPU_Process_batch not implemented");
}

size_t BaselineRemover::GPU_Ram_per_channel() const {
  throw std::runtime_error("GPU_Ram_per_channel not implemented");
}
