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

#ifndef SRC_TIMER_H_
#define SRC_TIMER_H_

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/function.hpp>

#include <map>
#include <string>

#include "src/args.h"
#include "src/discord.h"
#include "src/git.h"

class Timer {
  Discord *discord;
  Git *git;
  Args *args;

 public:
  Timer(boost::asio::io_service *io, Args *args, Discord *discord, Git *git);

  void remove_before_update(const std::string &key);
  void add_before_update(const std::string &key,
                         boost::function<void()> callback);

  void stop_loop();
  bool is_looping() const;
  void start_loop();

 private:
  boost::asio::deadline_timer timer;

  std::map<std::string, boost::function<void()>> before_update;

  int seconds_interval = 15;
  bool keep_alive = true;
  bool first_loop = true;
  bool looping = false;
  bool prevent_shutdown = false;

  void loops_body();
  void loop();
  void evoke_before_update();
};

#endif  // SRC_TIMER_H_
