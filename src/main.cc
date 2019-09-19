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

#include <git2.h>
#include <boost/asio/error.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/system/system_error.hpp>

#include "src/args.h"
#include "src/client.h"
#include "src/discord.h"
#include "src/git.h"
#include "src/language.h"
#include "src/server.h"
#include "src/timer.h"
#include "src/utils.h"

int main(int argc, char *argv[]) {
  Args args(argc, argv);

  // Don't continue, if args aren't complete
  if (args.errors > 0) {
    if (args.exit_cleanly())
      return 0;
    else
      return 1;
  }

  git_libgit2_init();

  boost::asio::io_service io;
  Git git(&args);
  Client client(&io);

  Discord discord(&args, &git, &client);
  Timer timer(&io, &args, &discord, &git);

  try {
    Server server(&io, &args, &timer, &git, &discord);
    io.run();
  } catch (const boost::system::system_error &error) {
    if (error.code() == boost::asio::error::address_in_use) {
      if (args.replace) {
        std::cout << "Error: Replace failed, maybe its not taken by Presence?\n"
                  << "Port " << args.port << " still in use.\n"
                  << "Exitting…\n";
      } else {
        std::cout << "Error: Port " << args.port << " already in use."
                  << " If its Presence, use --replace\n"
                  << "Exitting…\n";
      }
    } else {
      std::cout << error.what() << "\n";
    }
  }

  git_libgit2_shutdown();
  return 0;
}
