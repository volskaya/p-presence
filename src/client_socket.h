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

#ifndef SRC_CLIENT_SOCKET_H_
#define SRC_CLIENT_SOCKET_H_

#include <boost/asio.hpp>

#include <queue>
#include <string>

enum SocketStatus {
  OFFLINE = 0,
  CONNECTED = 1,
  AUTHENTICATED = 2,
};

class ClientSocket : public std::enable_shared_from_this<ClientSocket> {
 public:
  explicit ClientSocket(boost::asio::io_service *io,
                        const std::string &auth_json, const std::string &path);

  void try_connect();
  void authenticate();
  void send_presence();
  void queue_presence(const std::string &json);
  void dispatch();
  void close();
  bool is_open() const;
  void handle_auth();
  void handle_presence();  // Check is presence returned unauthorized

 private:
  boost::asio::io_service *io;
  std::string socket_path;

#if defined(BOOST_POSIX_API)
  boost::asio::local::stream_protocol::socket socket;
#elif defined(BOOST_WINDOWS_API)
  boost::asio::windows::stream_handle socket;
#endif

  size_t presence_buffer_size;
  boost::asio::streambuf presence_buffer;

  // Auth will always be the same, so I'm caching it,
  // so the socket can independently reconnect
  std::string auth_json;
  std::string presence = "";
  int reconnects = 0;

  SocketStatus connected = SocketStatus::OFFLINE;
  void handle_error(const boost::system::system_error &error);
};

#endif  // SRC_CLIENT_SOCKET_H_
