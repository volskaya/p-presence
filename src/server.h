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

#ifndef SRC_SERVER_H_
#define SRC_SERVER_H_

#include <boost/asio.hpp>

#include <memory>
#include <queue>
#include <string>
#include <unordered_map>

#include "json/single_include/nlohmann/json.hpp"
#include "src/args.h"
#include "src/connection.h"
#include "src/discord.h"
#include "src/git.h"
#include "src/timer.h"

class Server : std::enable_shared_from_this<Server> {
 public:
  explicit Server(boost::asio::io_service *io, Args *args, Timer *timer,
                  Git *git, Discord *discord);

  ~Server();

  bool started = false;

  // RPC methods
  void start();
  void shutdown();
  void release_port();
  bool is_running();
  nlohmann::json ping(const int id);
  void git_change(const std::string &repo_name);
  bool im_leaving(const int id);
  std::string get_path();
  nlohmann::json get_info();
  nlohmann::json set_path(const ChangeProp &props);

 private:
  Timer *timer;
  Git *git;
  Discord *discord;
  Args *args;
  bool prevent_shutdown = false;
  bool exitting = false;  // True, when trying to shutdown;

  // Every client must periodically ping the server, to let it know
  // they're still alive. Ping will add them to the queue, which will
  // then be compared with the old queue. If an ID is missing, its
  // cached data will be erased
  std::queue<int> alive_clients;

  // List of clients, that were alive in the previous round of pings
  // std::unordered_map<int, bool> previous_clients { false };
  std::unordered_set<int> previous_clients;

  static nlohmann::json construct_json(const SessionPtr &session);

  void register_ping_callback();
  void try_to_shutdown();

  // Server
  boost::asio::io_service *io = nullptr;
  boost::asio::ip::tcp::acceptor *acceptor = nullptr;

  void start_accept();
  void stop_accept();
};

#endif  // SRC_SERVER_H_
