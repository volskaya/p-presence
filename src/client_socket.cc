/*
 * Copyright (C) github.com/volskaya
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/filesystem.hpp>
#include <boost/system/system_error.hpp>

#include <iostream>
#include <queue>
#include <string>

#include "src/client.h"
#include "src/client_socket.h"
#include "src/utils.h"

ClientSocket::ClientSocket(boost::asio::io_service *io,
                           const std::string &auth_json,
                           const std::string &path)
    : io(io), socket_path(path), socket(*io), auth_json(auth_json) {
  try_connect();
}

void ClientSocket::try_connect() {
  try {
#if defined(BOOST_POSIX_API)
    if (!boost::filesystem::exists(socket_path)) {
      // Socket doesn't exist, do nothing
      std::cout << "Socket: " << socket_path << " does not exist\n";
      return;
    }

    socket.connect(socket_path);
    std::cout << "Socket: " << socket_path << " opened\n";
#elif defined(BOOST_WINDOWS_API)
    // Discords RPC had it hardcoded to index 0 aswell, so I'll
    // just follow their example
    std::wstring a = string_to_wide_string(socket_path);

    HANDLE handle =
        ::CreateFileW(a.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                      OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);

    socket.assign(handle);

    std::cout << "Socket: " << socket_path << " opened\n";
#endif
  } catch (const boost::system::system_error &error) {
    return handle_error(error);
  }

  connected = SocketStatus::CONNECTED;
}

void ClientSocket::authenticate() {
  if (connected == SocketStatus::OFFLINE) try_connect();

  if (connected == SocketStatus::OFFLINE ||
      connected == SocketStatus::AUTHENTICATED)
    return;

  std::cout << "Authenticating Socket ID: " << socket_path << "\n";

  Client::MessageFrame handshakeFrame;
  handshakeFrame.opcode = Client::Opcode::Handshake;
  handshakeFrame.length = (uint32_t)auth_json.size();
  memcpy(handshakeFrame.message, auth_json.data(), auth_json.size());

  const size_t kFrameSize =
      sizeof(Client::MessageFrameHeader) + handshakeFrame.length;

  char b[kFrameSize];
  memcpy(b, &handshakeFrame, kFrameSize);

  try {
    const size_t kWritten =
        socket.write_some(boost::asio::buffer(b, kFrameSize));

    std::cout << "Awaiting echo back for Socket ID: " << socket_path << "\n";
    char buffer[kWritten];

    boost::asio::async_read(
        socket, boost::asio::buffer(buffer, kWritten),
        [this, _ = shared_from_this()](const boost::system::system_error &error,
                                       const long unsigned int whats_this) {
          if (error.code()) return handle_error(error);
          handle_auth();
        });

    connected = SocketStatus::AUTHENTICATED;
  } catch (const boost::system::system_error &error) {
    return handle_error(error);
  }
}

void ClientSocket::handle_auth() { send_presence(); }

void ClientSocket::handle_presence() {
  // Size could also be 0, if discord client timed out against their
  // own servers. Don't know if I should ignore it or not.
  if (presence_buffer.size() == presence_buffer_size) {
    // Echo back with same size means the request was successful
    reconnects--;
  } else {
    std::cout << "Invalid echo back detected for socket " << socket_path
              << "\n";

    connected = SocketStatus::CONNECTED;
  }

  // Clear buffer, so it can be reused
  presence_buffer.consume(presence_buffer.size());
  presence_buffer_size = 0;
}

void ClientSocket::handle_error(const boost::system::system_error &error) {
  if (error.code() == boost::asio::error::broken_pipe) {
    std::cout << "Socket ID: " << socket_path << " no longer available\n";
  } else if (error.code() == boost::asio::error::connection_refused) {
    std::cout << "Socket ID: " << socket_path << " connection refused\n";

    // Connection refused didn't have a connection to
    // reconnect to, to begin with
    reconnects--;
  } else {
    std::cout << "Socket error: " << error.what() << "\n";
  }

  connected = SocketStatus::OFFLINE;
  close();

  // Reconnects are added for each queueu_presence();
  if (reconnects > 0) {
    std::cout << "Reconnecting " << socket_path << "…\n";
    reconnects--;
    send_presence();
  }
}

void ClientSocket::send_presence() {
  // If authentication is required, authenticate and asynchronously
  // queue up presence, after recieving echo back from auth
  if (connected < AUTHENTICATED) return authenticate();

  if (connected != AUTHENTICATED)
    return;  // If still not authenticated, don't do anything

  Client::MessageFrame presenceFrame;
  presenceFrame.opcode = Client::Opcode::Frame;
  presenceFrame.length = static_cast<uint32_t>(presence.size());
  memcpy(presenceFrame.message, presence.data(), presence.size());

  const size_t kFrameSize =
      sizeof(Client::MessageFrameHeader) + presenceFrame.length;

  char b[kFrameSize];
  memcpy(b, &presenceFrame, kFrameSize);

  try {
    presence_buffer_size =
        socket.write_some(boost::asio::buffer(b, kFrameSize));
  } catch (const boost::system::system_error &error) {
    return handle_error(error);
  }

  std::cout << "Socket: " << socket_path << " updated\n";

  boost::asio::async_read(
      socket, presence_buffer,
      boost::asio::transfer_exactly(presence_buffer_size),
      [this, _ = shared_from_this()](const boost::system::system_error &error,
                                     const long unsigned int whats_this) {
        if (error.code()) return handle_error(error);
        handle_presence();
      });
}

// For every queue let there be 1 reconnect @handle_error
void ClientSocket::queue_presence(const std::string &json) {
  reconnects++;
  presence = json;
  send_presence();
}

// Presence is wiped automatically, when the socket is closed, as
// far as I know
void ClientSocket::close() { socket.close(); }

bool ClientSocket::is_open() const {
  return connected == AUTHENTICATED ? true : false;
}
