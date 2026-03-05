
#pragma once

#include <string>

struct AppContext;

class LoginScreen {
public:
  LoginScreen();

  void Render(AppContext &ctx);

private:
  char m_username[64] = {};
  char m_password[128] = {};
  std::string m_errorMsg;
  float m_errorTimer = 0.0f;

  struct Particle {
    float x, y, vx, vy, r;
    int alpha;
  };
  Particle m_particles[40] = {};
  bool m_particlesInit = false;
};
