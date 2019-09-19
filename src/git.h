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

#ifndef SRC_GIT_H_
#define SRC_GIT_H_

#include <boost/filesystem.hpp>
#include <boost/process/search_path.hpp>

#include <iostream>
#include <memory>
#include <queue>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "src/args.h"
#include "src/editor_icon.h"
#include "src/language.h"
#include "src/session.h"
#include "src/utils.h"

typedef std::shared_ptr<Session> SessionPtr;

struct ClientProp {
  explicit ClientProp(const int id) : id(id){};

  const int id;
  std::string editor_name = "";
  std::string editor_icon = "";
  std::unordered_set<SessionPtr> sessions;
};

struct ChangeProp {
  explicit ChangeProp(const int id, const std::string &path)
      : id(id), path(path){};

  const int id;
  const std::string path;
  std::string editor_name = "";
};

class Git {
  typedef std::unordered_map<std::string, SessionPtr> SessionMap;
  typedef std::unordered_map<int, ClientProp> ClientsMap;

  const Args *args;
  const Language lang_serv;  // Lang server

 public:
  //! Default constructor
  explicit Git(const Args *args);
  virtual ~Git() noexcept = default;

  Git(const Git &other) = delete;
  Git(Git &&other) noexcept;
  Git &operator=(const Git &other) = delete;
  Git &operator=(Git &&other) noexcept = default;

  //! Methods
  bool is_ready() const;
  bool has_clients() const;

  void update();
  void update(const std::string &name);
  void not_ready();
  void drop_client(const int id);
  void drop_inactive_clients(const std::unordered_set<int> active_clients);

  SessionPtr change_git(const ChangeProp &props);
  SessionPtr get_active_session() const;
  const ClientProp *get_active_client() const;
  bool is_waiting() const;
  bool awaiting_repo() const;
  int get_clients_count() const;

 private:
  int active_id = 0;  // Id of the active session trees owner
  SessionPtr active_session;
  EditorIcon editor_icon;
  bool waiting = false;
  bool dont_await_repo = false;
  bool ready = false;

  ClientsMap clients_map;
  void cleanup_client(const int id);

  SessionMap sessions;
  SessionPtr get_session(const std::string &name) const;

  // Support for falling back to previous sessions
  std::stack<std::pair<int, std::string>> session_history;
  void enque_session(const int id, const std::string &name);
  void fallback_to_last_session();
};

#endif  // SRC_GIT_H_
