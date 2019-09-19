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

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/system/system_error.hpp>

#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "json/single_include/nlohmann/json.hpp"
#include "src/args.h"
#include "src/async_replace.h"
#include "src/connection.h"
#include "src/git.h"
#include "src/git_walker.h"
#include "src/server.h"
#include "src/timer.h"

namespace ip = boost::asio::ip;
namespace http = boost::beast::http;

Server::Server(boost::asio::io_service *io, Args *args, Timer *timer, Git *git,
               Discord *discord)
    : timer(timer), git(git), discord(discord), args(args), io(io) {
  if (!args->replace) {
    start();
    return;
  }

  // If replace required, start() is called after
  // async response or timeout reached
  std::cout << "Awaiting shutdown notice from port " << args->port << "\n";

  try {
    std::make_shared<AsyncReplace>(io, this)->shutdown(args->port);
  } catch (const boost::system::system_error &error) {
    std::cout << "Nothing replaceable found"
              << "\n";
    start();
  }
}

Server::~Server() { delete acceptor; }
bool Server::is_running() { return true; }

void Server::start() {
  if (started) {
    std::cout << "Something attempted to start the server, "
              << "while its already running\n";

    return;
  }

  started = true;
  acceptor =
      new ip::tcp::acceptor(*io, ip::tcp::endpoint(ip::tcp::v4(), args->port));

  std::cout << "Starting server on port " << args->port << "\n";

  prevent_shutdown = args->prevent_shutdown;

  register_ping_callback();
  start_accept();
}

nlohmann::json Server::set_path(const ChangeProp &props) {
  SessionPtr session = git->change_git(props);

  // If presence is cleared, force update without waiting for
  // the 15 second timer
  // Also automatically act as a ping
  alive_clients.push(props.id);

  if (discord->has_no_presence()) {
    timer->start_loop();  // If already looping, will call loops body
    alive_clients.push(props.id);
  }

  if (session) return construct_json(session);

  return nlohmann::json{{"valid", false}, {"path", props.path}};
}

std::string Server::get_path() { return git->get_active_session()->path; }

nlohmann::json Server::get_info() {
  SessionPtr session = git->get_active_session();
  return construct_json(session);
}

nlohmann::json Server::construct_json(const SessionPtr &session) {
  if (!session)
    return nlohmann::json{
        {"valid", false},
        {"name", ""},
        {"prettyName", ""},
        {"branch", ""},
        {"path", ""},
        {"hasOrigin", false},
        {"cache",
         {
             {"localCommistOnStart", 0},
             {"aheadOfLocal", 0},
             {"remoteCommitsOnStart", 0},
             {"aheadOfRemote", 0},
             {"pushedToRemote", 0},
         }},
    };

  const BranchProp *branch_props = session->get_active_branch_props();

  return nlohmann::json{
      {"valid", session->valid},
      {"name", session->name},
      {"prettyName", session->pretty_name},
      {"branch", session->branch},
      {"path", session->path},
      {"hasOrigin", session->has_origin},
      {"cache",
       {
           {"localCommistOnStart", branch_props->local_comits_on_start},
           {"aheadOfLocal", branch_props->ahead_of_local},
           {"remoteCommitsOnStart", branch_props->remote_commits_on_start},
           {"aheadOfRemote", branch_props->ahead_of_remote},
           {"pushedToRemote", branch_props->pushed_to_remote},
       }},
  };
}

nlohmann::json Server::ping(const int id) {
  std::cout << "Ping received from: " << id << "\n";

  alive_clients.push(id);

  // Also return the active session info
  // Used for updating status bar in vscode
  SessionPtr session = git->get_active_session();

  if (session && session->valid) return construct_json(session);

  return nlohmann::json{{"valid", false}};
}

void Server::git_change(const std::string &name) {
  std::cout << "Post commit notice recieved from git named " << name << "\n";

  if (!name.empty() && !args->auto_update) git->update(name);
}

bool Server::im_leaving(const int id) {
  std::cout << "Leave notice received from: " << id << "\n";

  if (!previous_clients.count(id)) {
    std::cout << "Client " << id
              << " has not been tracked as active, skipping…\n";

    return false;
  }

  git->drop_client(id);
  try_to_shutdown();

  return true;
}

// Called from Timer::loops_body()
void Server::register_ping_callback() {
  timer->add_before_update("ping", [this]() {
    std::unordered_set<int> active_clients;

    // Alive clients are the ones that sent out a ping
    while (!alive_clients.empty()) {
      active_clients.insert(alive_clients.front());
      alive_clients.pop();
    }

    // Id 0 is used to debug, let it stay alive without pings
    if (previous_clients.count(0)) active_clients.insert(0);

    previous_clients = active_clients;
    git->drop_inactive_clients(active_clients);

    // If cache is empty, after dropping inacitve clients
    // its time to exit
    try_to_shutdown();
  });
}

void Server::try_to_shutdown() {
  // Usually awaiting_repo() won't ever happen, because the
  // timer doesn't even evoke callbacks, if its stil waiting
  if (git->awaiting_repo()) {
    std::cout << "Still waiting for a repository…\n";
    return;
  }

  if (!git->has_clients()) {
    if (prevent_shutdown) {
      // Presence will be cleared, if discord tries to update,
      // while git is not ready
      git->not_ready();
      timer->stop_loop();

      return;
    }

    exitting = true;
    std::cout << "No more clients left, exitting…\n";

    timer->stop_loop();
    stop_accept();
  }
}

void Server::start_accept() {
  auto new_connection = std::make_shared<Connection>(io, this);

  acceptor->async_accept(
      new_connection->get_socket(),
      [this, new_connection](const boost::system::system_error &error) {
        if (!error.code()) new_connection->start();
        if (!exitting) start_accept();
      });
}

void Server::stop_accept() { acceptor->cancel(); }

void Server::release_port() {
  exitting = true;  // Stops start_accept() above
  acceptor->cancel();
  acceptor->close();
}

void Server::shutdown() {
  if (!exitting) release_port();

  discord->stop();
  timer->stop_loop();
}
