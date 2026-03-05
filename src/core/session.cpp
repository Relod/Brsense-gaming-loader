
#include "session.h"

#include <cstdio>
#include <cstring>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dpapi.h>

static const char *SESSION_DIR = "C:\\temp";
static const char *SESSION_FILE = "C:\\temp\\cs_login.dat";

static bool ProtectAndWrite(const std::string &user,
                            const std::string &pass) {
  std::string joined = user + "\n" + pass;
  DATA_BLOB inBlob{0};
  inBlob.pbData = (BYTE *)joined.data();
  inBlob.cbData = (DWORD)joined.size();

  DATA_BLOB outBlob{0};
  if (!CryptProtectData(&inBlob, L"BRSense Session", nullptr, nullptr, nullptr,
                        CRYPTPROTECT_LOCAL_MACHINE, &outBlob)) {
    return false;
  }

  FILE *f = fopen(SESSION_FILE, "wb");
  if (!f) {
    LocalFree(outBlob.pbData);
    return false;
  }
  fwrite(&outBlob.cbData, sizeof(DWORD), 1, f);
  fwrite(outBlob.pbData, 1, outBlob.cbData, f);
  fclose(f);
  LocalFree(outBlob.pbData);
  return true;
}

static bool ReadAndUnprotect(std::string &user, std::string &pass) {
  FILE *f = fopen(SESSION_FILE, "rb");
  if (!f)
    return false;
  DWORD size = 0;
  if (fread(&size, sizeof(DWORD), 1, f) != 1 || size == 0 || size > 4096) {
    fclose(f);
    return false;
  }
  std::vector<BYTE> buf(size);
  if (fread(buf.data(), 1, size, f) != size) {
    fclose(f);
    return false;
  }
  fclose(f);

  DATA_BLOB inBlob{0};
  inBlob.pbData = buf.data();
  inBlob.cbData = size;
  DATA_BLOB outBlob{0};
  if (!CryptUnprotectData(&inBlob, nullptr, nullptr, nullptr, nullptr, 0,
                          &outBlob)) {
    return false;
  }

  std::string joined((char *)outBlob.pbData, outBlob.cbData);
  LocalFree(outBlob.pbData);

  auto pos = joined.find('\n');
  if (pos == std::string::npos)
    return false;
  user = joined.substr(0, pos);
  pass = joined.substr(pos + 1);
  return true;
}

void SessionSave(const char *user, const char *pass) {
  CreateDirectoryA(SESSION_DIR, nullptr);
  ProtectAndWrite(user ? user : "", pass ? pass : "");
}

bool SessionLoad(SessionData &out) {
  std::string u, p;
  if (!ReadAndUnprotect(u, p))
    return false;
  out.username = u;
  out.password = p;
  return true;
}

void SessionClear() { DeleteFileA(SESSION_FILE); }
