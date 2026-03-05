
#pragma once

#include <string>

struct SessionData {
  std::string username;
  std::string password;
};

void SessionSave(const char *user, const char *pass);

bool SessionLoad(SessionData &out);

void SessionClear();
