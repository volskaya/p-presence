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
#include <boost/process/search_path.hpp>
#include <boost/process/system.hpp>

#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "src/args.h"
#include "src/git.h"
#include "src/git_walker.h"

namespace filesystem = boost::filesystem;
namespace process = boost::process;

Git::Git(const Args *args) : args(args) {
  // This doesn't actually mean it had a child, it just lets timer
  // to start updating presence as "working on a private repo", if
  // this is true
  dont_await_repo = args->dont_await_repo;
  waiting = true;
}

// Usually called from rpc
SessionPtr Git::change_git(const ChangeProp &props) {
  std::cout << "Changing git directory to - " << props.path << "\n";

  ready = true;
  active_id = props.id;

  if (!clients_map.count(active_id)) {
    std::cout << "Added ID: " << active_id << ", to clients\n";
    auto prop = clients_map.emplace(active_id, ClientProp(active_id));

    if (!props.editor_name.empty()) {  // Text editors language
      prop.first->second.editor_name = props.editor_name;
      prop.first->second.editor_icon =
          editor_icon.get_icon_name(props.editor_name);
    }
  }

  GitWalker *git_path = new GitWalker(args, props.path);

  if (!git_path->valid) {
    std::cout << "Error changing to an invalid git path: \""
              << git_path->string() << "\", skipping…\n";

    waiting = true;
    return nullptr;
  }

  SessionPtr session = get_session(git_path->string());

  if (!session) {
    std::cout << "Session \"" << git_path->name
              << "\" doesn't exist, creating a new one…\n";

    // git_path is deleted inside SessionPtr constructor
    SessionPtr session(new Session(args, git_path, &lang_serv, active_id));

    if (session->valid) {
      sessions[session->id] = session;

      std::cout << "Storing session ID " << active_id
                << ", Name:" << session->name << "\n";

      clients_map.at(active_id).sessions.insert(session);
      session->parents.insert(active_id);
      enque_session(active_id, session->id);
    }

    active_session = session;
    waiting = false;

    return session;
  } else if (session->valid) {
    std::cout << "Switching to a cached repo, new path " << props.path << "\n";

    clients_map.at(active_id).sessions.insert(session);
    session->parents.insert(active_id);
    session->update_path(props.path);

    active_session = session;
    waiting = false;

    enque_session(active_id, session->id);

    delete git_path;
    return session;
  } else {
    std::cout << "Git repository not found at " << props.path << "\n";
  }

  waiting = true;
  active_session = nullptr;

  delete git_path;
  return nullptr;
}

// Updates all sessions, client id -> name -> session
void Git::update() {
  if (waiting) return;  // Wait, when no repo path available

  // If session is invalid at the time, the path still gets
  // queried to see, if it has become a valid git repository
  for (auto session : sessions) session.second->update();
}

// Search trough all sessions and only update the ones with
// a matching name
void Git::update(const std::string &name) {
  if (waiting) return;

  int updated = 0;

  std::cout << "Updating repositories with the name " << name << "\n";

  for (auto session : sessions) {
    if (session.second->name == name) {
      session.second->update();
      updated++;
    }
  }

  std::cout << "Updated sessions: " << updated << "\n";
}

// not_ready() used when shutdown is prevented, to clear
// fake presence from the server
void Git::not_ready() { ready = false; }
bool Git::is_ready() const { return ready; }

SessionPtr Git::get_session(const std::string &name) const {
  try {
    return sessions.at(name);
  } catch (const std::out_of_range &error) {
    return nullptr;
  }
}

SessionPtr Git::get_active_session() const { return active_session; }
const ClientProp *Git::get_active_client() const {
  try {
    return &clients_map.at(active_id);
  } catch (const std::out_of_range &error) {
    return nullptr;
  }
}

bool Git::is_waiting() const { return waiting; }
bool Git::awaiting_repo() const {
  return dont_await_repo && !ready ? true : false;
}

void Git::cleanup_client(const int id) {
  try {
    for (SessionPtr session : clients_map.at(id).sessions) {
      session->parents.erase(id);

      // The session is no longer needed, if it has no parents
      if (session->parents.empty()) sessions.erase(session->id);
    }
  } catch (const std::out_of_range &error) {
    // Do nothing
  }
}

void Git::drop_client(const int id) {
  int size = clients_map.size();

  std::cout << "Erasing cached data for client ID: " << id;

  if (size >= 1)
    std::cout << ", clients remaining: " << size - 1 << "\n";
  else
    std::cout << ", no more clients remaining\n";

  cleanup_client(id);
  clients_map.erase(id);

  if (active_id == id) fallback_to_last_session();
}

void Git::drop_inactive_clients(const std::unordered_set<int> active_clients) {
  // Iterate over old clients, before the pings were processed
  for (auto it = clients_map.begin(); it != clients_map.end();) {
    // Don't delete id 0, its used to debug
    if (it->first == 0) {
      it++;
      continue;
    }

    // If old_id is no longer in active_clients (those who pinged)
    // erase its cache
    if (!active_clients.count(it->first)) {
      auto it_prev = it++;
      std::cout << "Erasing client ID: " << it_prev->first << "\n";

      cleanup_client(it_prev->first);
      clients_map.erase(it_prev);
    } else {
      it++;
    }
  }

  // If active ID was erased, try to fallback to previous session
  if (!active_clients.count(active_id)) fallback_to_last_session();
}

bool Git::has_clients() const { return clients_map.size() > 0 ? true : false; }

int Git::get_clients_count() const { return clients_map.size(); }

void Git::enque_session(const int id, const std::string &name) {
  // Delete previous entry, if its the same id, but different session
  if (!session_history.empty() && session_history.top().first == id)
    session_history.pop();

  // Add the session to previous sessions list
  session_history.push(std::pair(id, name));
}

// Returns the last valid session
void Git::fallback_to_last_session() {
  bool found = false;

  if (session_history.size() > 1) {
    std::cout << "Falling back to previous session, history size: "
              << session_history.size() << "\n";
  }

  while (session_history.size() >= 1 && !found) {
    // Client ID, repo name
    std::pair<int, std::string> top = session_history.top();
    SessionPtr session = get_session(top.second);

    if (session && session->valid) {
      std::cout << "Switching to a cached repo, skipping calculations\n";

      active_session = session;
      active_id = top.first;
      waiting = false;
      found = true;

      break;
    }

    session_history.pop();
  }

  // If no sessions in the queue, start waiting
  if (!found) {
    std::cout << "Fallback didn't find any valid session\n";

    waiting = true;
    active_id = 0;
    active_session = nullptr;
  }
}
