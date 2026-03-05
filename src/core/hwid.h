
#pragma once

#include <string>

struct HardwareInfo {
  std::string hwid;
  std::string mac;
  std::string ip;
};

HardwareInfo CollectHardwareInfo();
