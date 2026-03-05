#pragma once

#include <string>

struct LoaderConfig {
  std::string host = "localhost";
  unsigned int port = 3000;
};

LoaderConfig LoadConfig();
