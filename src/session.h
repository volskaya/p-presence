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

#ifndef SRC_SESSION_H_
#define SRC_SESSION_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "src/args.h"
#include "src/git_hook.h"
#include "src/git_walker.h"
#include "src/language.h"

struct BranchProp {
  // For calculating commits against local branch
  bool local_commits_calculated = false;
  int local_comits_on_start = 0;
  int ahead_of_local = 0;

  // For calculating commits against remote branch
  bool remote_commits_calculated = false;
  int remote_commits_on_start = 0;
  int ahead_of_remote = 0;
  int pushed_to_remote = 0;
};

class Session {
  typedef std::unordered_map<std::string, BranchProp> BranchCache;

  const Args *args;
  GitHook hooker;
  const Language *lang_serv;

 public:
  explicit Session(const Args *args, GitWalker *walker,
                   const Language *lang_serv, const int client_id);

  int client_id;
  bool valid = false;
  std::string id = "";
  std::string name = "";
  std::string pretty_name = "";
  std::string branch = "";
  std::string dirty_path = "";
  std::string path = "";
  bool has_origin = false;
  PropsPointer language = nullptr;
  std::unordered_set<int> parents;
  BranchCache branches;

  void update();
  void update_path(const std::string &path);
  const BranchProp *get_active_branch_props() const;

 private:
  std::string get_pretty_name() const;
  void update_stats(const GitWalker *walker);
  void zero_stats();
};

#endif  // SRC_SESSION_H_
