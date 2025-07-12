#include "gdb-server-kit/server.hpp"
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace gsk {

Server::~Server() {
  if (socket_fd != -1) {
    close(socket_fd);
  }
}

void Server::start(std::string address, int port) {
  setup_socket(address, port);

  sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  int connection_fd = accept(socket_fd, (sockaddr *)&client_addr, &client_addr_len);
  if (connection_fd < 0) {
    throw Error("Failed to accept connection");
  }

  handle_client_communication(connection_fd);
}

void Server::setup_socket(const std::string& address, int port) {
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    throw Error("Failed to create socket");
  }

  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(address.c_str());
  server_addr.sin_port = htons(port);

  if (bind(socket_fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    throw Error("Failed to bind socket");
  }
  if (listen(socket_fd, 5) < 0) {
    throw Error("Failed to listen on socket");
  }
}

bool Server::validate_packet(const std::string& packet, std::string& data) {
  if (packet[0] != '$') {
    return false;
  }

  size_t hash_pos = packet.find('#');
  if (hash_pos == std::string::npos) {
    return false;
  }

  data = packet.substr(1, hash_pos - 1);
  std::string checksum_str = packet.substr(hash_pos + 1);
  
  unsigned int checksum = 0;
  for (char c : data) {
    checksum += static_cast<unsigned char>(c);
  }
  checksum %= 256;
  
  unsigned int received_checksum;
  try {
    received_checksum = std::stoul(checksum_str, nullptr, 16);
  } catch (const std::invalid_argument&) {
    return false;
  } catch (const std::out_of_range&) {
    return false;
  }
  
  return checksum == received_checksum;
}

void Server::handle_client_communication(int connection_fd) {
  while (true) {
    std::string packet;
    char recv_buffer[1024];
    ssize_t bytes_received = recv(connection_fd, recv_buffer, sizeof(recv_buffer) - 1, 0);
    if (bytes_received < 0) {
      throw Error("Failed to receive data");
    } else if (bytes_received == 0) {
      std::cout << "Client disconnected" << std::endl;
      break;
    }
    recv_buffer[bytes_received] = '\0'; // Null-terminate the received data
    packet.append(recv_buffer, bytes_received);

    std::string data;
    if (!validate_packet(packet, data)) {
      send(connection_fd, "-", 1, 0); // Request retransmission
      continue;
    }

    std::string reply = receive(data);
    unsigned int checksum = 0;
    for (char c : reply) {
      checksum += static_cast<unsigned char>(c);
    }
    checksum %= 256;
    char checksum_hex[3];
    snprintf(checksum_hex, sizeof(checksum_hex), "%02x", checksum);
    std::string response = "+$" + reply + "#" + checksum_hex;

    send(connection_fd, response.c_str(), response.size(), 0);
  }
}

std::string Server::receive(std::string data) {
  return "";
}

} // namespace gsk
