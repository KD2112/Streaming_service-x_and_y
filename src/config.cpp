#include "config.hpp"

#include <getopt.h>

#include <cstdlib>
#include <iostream>

namespace streaming {

void print_usage(const char* argv0) {
  std::cerr
      << "Usage: " << argv0 << " [options]\n"
      << "  --file PATH    MP4 file to stream (default: samples/15158346_3840_2160_60fps.mp4)\n"
      << "  --port PORT    RTSP listen port (default: 8554)\n"
      << "  --mount PATH   Mount point (default: /stream)\n"
      << "  -h, --help     Show this help\n";
}

bool parse_args(int argc, char** argv, Config& config) {
  static const option long_opts[] = {
      {"file", required_argument, nullptr, 'f'},
      {"port", required_argument, nullptr, 'p'},
      {"mount", required_argument, nullptr, 'm'},
      {"help", no_argument, nullptr, 'h'},
      {nullptr, 0, nullptr, 0},
  };

  int opt = 0;
  int long_index = 0;
  while ((opt = getopt_long(argc, argv, "f:p:m:h", long_opts, &long_index)) !=
         -1) {
    switch (opt) {
      case 'f':
        config.file = optarg;
        break;
      case 'p': {
        const long port = std::strtol(optarg, nullptr, 10);
        if (port <= 0 || port > 65535) {
          std::cerr << "Invalid port: " << optarg << '\n';
          print_usage(argv[0]);
          return false;
        }
        config.port = static_cast<std::uint16_t>(port);
        break;
      }
      case 'm':
        config.mount = optarg;
        if (config.mount.empty() || config.mount[0] != '/') {
          std::cerr << "Mount must start with '/': " << optarg << '\n';
          print_usage(argv[0]);
          return false;
        }
        break;
      case 'h':
        print_usage(argv[0]);
        return false;
      default:
        print_usage(argv[0]);
        return false;
    }
  }

  return true;
}

}  // namespace streaming
