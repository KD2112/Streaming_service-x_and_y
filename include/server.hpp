#pragma once

#include "config.hpp"

namespace streaming {

// Blocks until SIGINT/SIGTERM or the main loop quits. Returns process exit code.
int run_server(const Config& config);

}  // namespace streaming
