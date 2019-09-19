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

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <iostream>
#include <memory>
#include <string>

#include "json/single_include/nlohmann/json.hpp"
#include "src/client.h"
#include "src/client_socket.h"
#include "src/discord_presence.h"
#include "src/utils.h"

Client::Client(boost::asio::io_service *io) : io(io) {
  set_tmp_path();
  open_socket();
}

void Client::set_tmp_path() {
  const char *tmp = getenv("XDG_RUNTIME_DIR");
  tmp = tmp ? tmp : getenv("TMPDIR");
  tmp = tmp ? tmp : getenv("TMP");
  tmp = tmp ? tmp : getenv("TEMP");
  tmp = tmp ? tmp : "/tmp";

  tmp_path = std::string(tmp) + "/";
}

void Client::open_socket() {
  std::string auth = create_authentication().dump();

  for (int i = 0; i < socket_count; i++) {
#if defined(BOOST_POSIX_API)
    std::string path = tmp_path + socket_name + std::to_string(i);
#elif defined(BOOST_WINDOWS_API)
    std::string path = "\\\\?\\pipe\\discord-ipc-" + std::to_string(i);
#endif

    std::shared_ptr<ClientSocket> socket =
        std::make_shared<ClientSocket>(io, auth, path);

    sockets[i] = socket;
  }
}

void Client::dispatch_presence(const DiscordPresence &discord_presence) {
  std::string presence = discord_presence.to_string();

  for (int i = 0; i < socket_count; i++) {
    ref_socket socket = sockets.at(i);
    socket->queue_presence(presence);
  }
}

void Client::close_sockets() {
  std::cout << "Closing sockets…\n";

  for (int i = 0; i < socket_count; i++) {
    sockets.at(i)->close();
  }
}

nlohmann::json Client::create_authentication() const {
  return nlohmann::json{{"client_id", ID},
                        {"v", 1},
                        {"nonce", "aa8241d4-8806-4542-9ed6-6d6cb5d239eb"}};
}
