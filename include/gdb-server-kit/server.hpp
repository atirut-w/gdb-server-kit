#pragma once
#include <exception>
#include <string>

namespace gsk {

class Error : public std::exception {
  std::string message;

public:
  Error(const std::string& message) : message(message) {}

  const char* what() const noexcept override {
    return message.c_str();
  }
};

class Server {
  int socket_fd = -1;

public:
  ~Server();

  void start(std::string address, int port);
};

}
