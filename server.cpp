#include "gdb-server-kit/server.hpp"
#include <arpa/inet.h>
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

  sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  int connection_fd = accept(socket_fd, (sockaddr *)&client_addr, &client_addr_len);
  if (connection_fd < 0) {
    throw Error("Failed to accept connection");
  }

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

    if (packet[0] != '$') {
      send(connection_fd, "-", 1, 0); // Request retransmission
      continue;
    }

    std::string data = packet.substr(1, packet.find('#') - 1);
    std::string checksum_str = packet.substr(packet.find('#') + 1);
    unsigned int checksum = 0;
    for (char c : data) {
      checksum += static_cast<unsigned char>(c);
    }
    checksum %= 256;
    unsigned int received_checksum;
    try {
      received_checksum = std::stoul(checksum_str, nullptr, 16);
    } catch (const std::invalid_argument&) {
      send(connection_fd, "-", 1, 0); // Invalid checksum format
      continue;
    } catch (const std::out_of_range&) {
      send(connection_fd, "-", 1, 0); // Checksum out of range
      continue;
    }
    if (checksum != received_checksum) {
      send(connection_fd, "-", 1, 0); // Checksum mismatch
      continue;
    }

    std::cout << "Received packet: " << data << std::endl;
  }
}

} // namespace gsk
