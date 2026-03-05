#pragma once

#include <future>
#include <string>

struct AppContext;

class ConnectingScreen {
public:
  ConnectingScreen();
  void Render(AppContext &ctx);

private:
  bool m_startedConnection = false;
  bool m_connectionFinished = false;
  bool m_connectionSuccess = false;
  std::string m_errorMessage;

  std::future<bool> m_futureConnection;
};
