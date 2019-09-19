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

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/system/system_error.hpp>

#include <iostream>

#include "src/args.h"
#include "src/discord.h"
#include "src/git.h"
#include "src/timer.h"

Timer::Timer(boost::asio::io_service *io, Args *args, Discord *discord,
             Git *git)
    : discord(discord), git(git), args(args), timer(*io) {
  prevent_shutdown = args->prevent_shutdown;
}

void Timer::add_before_update(const std::string &key,
                              boost::function<void()> callback) {
  before_update[key] = callback;
}

void Timer::remove_before_update(const std::string &key) {
  try {
    before_update.erase(key);
  } catch (const std::out_of_range &error) {
    // Do nothing
  }
}

void Timer::evoke_before_update() {
  for (auto i = before_update.begin(); i != before_update.end(); i++)
    i->second();
}

// Whats done during each loop
void Timer::loops_body() {
  if (!git->is_ready()) return;

  std::cout << "Recalculating " << git->get_clients_count()
            << " clients and updating presence…\n";

  // Subscribed callbacks to be called before update
  // Currently only holds Server::register_ping_callback
  evoke_before_update();

  // Don't continue, if $evoke_before_update() decided
  // its time to exit
  if (!keep_alive) return;

  if (args->auto_update) git->update();

  // Play around is_ready(), to prevent fake presence
  // when shutdown is prevented and there's clients remaining
  if (!git->is_ready() && prevent_shutdown) {
    discord->clear_presence();
  } else {
    discord->update();
  }
}

// Call discord->update() every 15 seconds
void Timer::loop() {
  // Repeat timer $seconds_interval seconds later
  if (keep_alive) {
    timer.expires_at(
        (first_loop ? boost::asio::time_traits<boost::posix_time::ptime>::now()
                    : timer.expires_at()) +
        boost::posix_time::seconds(seconds_interval));

    timer.async_wait([this](const boost::system::system_error &error) {
      if (error.code()) return discord->clear_presence();

      loop();
    });

    first_loop = false;
  }

  std::cout << "----  " << seconds_interval << " second mark\n";
  loops_body();  // Loops body
}

void Timer::start_loop() {
  // If called while already looping, assume something wants
  // to force 1 loop
  if (looping) {
    std::cout << "Already looping, forcing a call on loops body…\n";
    loops_body();

    return;
  }

  std::cout << "15 second loop started\n";
  looping = true;
  keep_alive = true;
  first_loop = true;

  loop();
}

void Timer::stop_loop() {
  std::cout << "Stopping 15 second timer\n";

  keep_alive = false;
  looping = false;

  timer.cancel();
}

bool Timer::is_looping() const { return looping; }
