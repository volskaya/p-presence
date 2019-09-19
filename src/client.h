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

#ifndef SRC_CLIENT_H_
#define SRC_CLIENT_H_

#include <boost/asio.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "json/single_include/nlohmann/json.hpp"
#include "src/client_socket.h"
#include "src/discord_presence.h"
#include "src/utils.h"

class Client {
  typedef std::shared_ptr<ClientSocket> ref_socket;
  static constexpr size_t RPC_FRAME_SIZE = 64 * 1024;

 public:
  enum class Opcode : uint32_t {
    Handshake = 0,
    Frame = 1,
    Close = 2,
    Ping = 3,
    Pong = 4,
  };

  struct MessageFrameHeader {
    Opcode opcode;
    uint32_t length;
  };

  struct MessageFrame : public MessageFrameHeader {
    char message[RPC_FRAME_SIZE - sizeof(MessageFrameHeader)];
  };

  explicit Client(boost::asio::io_service *io);

  void dispatch_presence(const DiscordPresence &discord_presence);
  void close_sockets();

 private:
  int socket_count = 1;
  std::string client_id = ID;
  std::string socket_name = "discord-ipc-";  // Followed by a number
  std::string tmp_path;

  boost::asio::io_service *io;

  std::unordered_map<int, bool> socket_statuses;
  std::unordered_map<int, ref_socket> sockets;

  void set_tmp_path();
  void open_socket();

  nlohmann::json create_authentication() const;
};

#endif  // SRC_CLIENT_H_
