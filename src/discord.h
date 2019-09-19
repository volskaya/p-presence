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

#ifndef SRC_DISCORD_H_
#define SRC_DISCORD_H_

#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include "src/args.h"
#include "src/client.h"
#include "src/git.h"
#include "src/utils.h"

class Discord {
  const Git *git;
  const Args *args;

 public:
  //! Default constructor
  explicit Discord(const Args *args, const Git *git, Client *client);

  Discord(const Discord &other) = delete;
  virtual ~Discord() noexcept = default;
  Discord &operator=(const Discord &other) = delete;

  //! The rest
  void start();
  void stop();
  void update();
  void clear_presence();
  bool is_alive() const;
  bool has_no_presence() const;

 private:
  Client *client;

  int send_presence = 1;
  bool alive = false;
  bool dont_await_repo = false;
  bool has_presence = false;
  time_t start_time = 0;
  std::string id = std::string(ID);

  // Only used if args had --language
  std::string language = "";
  bool override_language = false;

  std::string build_state_string(const SessionPtr Session) const;
};

#endif  // SRC_DISCORD_H_
