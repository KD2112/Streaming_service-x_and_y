#pragma once

#include <cstdint>
#include <string>

namespace streaming {

struct Config {
  std::string file = "samples/15158346_3840_2160_60fps.mp4";
  std::string mount = "/stream";
  std::uint16_t port = 8554;
  int width = 3840;
  int height = 2160;
  int bitrate_kbps = 2000;
};

// Returns false if the user asked for help or parsing failed.
bool parse_args(int argc, char** argv, Config& config);

void print_usage(const char* argv0);

}  // namespace streaming
