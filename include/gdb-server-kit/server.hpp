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

  void setup_socket(const std::string& address, int port);
  bool validate_packet(const std::string& packet, std::string& data);
  void handle_client_communication(int connection_fd);

public:
  ~Server();

  void start(std::string address, int port);

protected:
  virtual std::string receive(std::string data);
};

}
