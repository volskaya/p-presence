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

#ifndef SRC_GIT_WALKER_H_
#define SRC_GIT_WALKER_H_

#include <git2.h>
#include <boost/filesystem/path.hpp>
#include <string>

#include "src/args.h"
#include "src/git_hook.h"

class GitWalker {
 public:
  explicit GitWalker(const Args* args, const std::string& path);
  ~GitWalker();  // Free git resources

  static boost::filesystem::path find_git(const boost::filesystem::path& path);

  std::string string() const;
  bool valid = false;
  bool has_origin = false;
  boost::filesystem::path
      init_path;  // Path, with which the walker was instantiated
  boost::filesystem::path git_path;  // Plain path returned from find_git
  boost::filesystem::path path;
  std::string name;  // Repository name == path filename
  std::string head_name;
  std::string head_ref_name;
  std::string origin_name;
  std::string origin_ref_name;

  int head_commits = 0;
  int origin_commits = 0;

  void update_stats();
  void count_head_and_origin();

 private:
  GitHook hooker;
  bool opened = false;  // True if repository is open

  bool has_walker = false;

  git_repository* repository = nullptr;
  git_revwalk* walker = nullptr;
  git_reference* head_ref = nullptr;
  git_reference* origin_ref = nullptr;

  const char* head_rev;
  const char* origin_rev;

  void setup();
  void find_head_and_origin();
  int count_commits(const std::string& ref_name) const;
};

#endif  // SRC_GIT_WALKER_H_
