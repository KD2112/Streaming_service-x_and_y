#include "config.hpp"
#include "server.hpp"

#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
  std::cout << std::unitbuf;

  streaming::Config config;
  if (!streaming::parse_args(argc, argv, config)) {
    return 1;
  }

  if (!std::filesystem::exists(config.file)) {
    std::cerr << "Media file not found: " << config.file << '\n'
              << "Place an MP4 under samples/ or pass --file PATH\n";
    return 1;
  }

  return streaming::run_server(config);
}
