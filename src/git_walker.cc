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
#include <boost/filesystem/path.hpp>

#include <string>

#include "src/args.h"
#include "src/git_hook.h"
#include "src/git_walker.h"

namespace filesystem = boost::filesystem;

GitWalker::GitWalker(const Args* args, const std::string& path)
    : init_path(path), hooker(args) {
  setup();
}

GitWalker::~GitWalker() {
  if (head_ref) git_reference_free(head_ref);
  if (origin_ref) git_reference_free(origin_ref);
  if (walker) git_revwalk_free(walker);
  if (repository) git_repository_free(repository);
}

void GitWalker::setup() {
  // Find git is gonna iterate upwards the path from whatever file was
  // provided in PATH. If a valid git repo is found, valid is set to true
  git_path = find_git(init_path);
  valid = !git_path.empty();

  if (valid) {
    path = git_path.parent_path().parent_path();  // Strip "/.git/"
    name = path.filename().string();
  }
}

filesystem::path GitWalker::find_git(const filesystem::path& path) {
  filesystem::path found_path;
  git_buf buf = {0};
  bool found =
      git_repository_discover(&buf, path.string().c_str(), 0, nullptr) == 0;

  if (found) found_path = buf.ptr;

  git_buf_free(&buf);
  return found_path;
}

void GitWalker::update_stats() {
  if (!valid) setup();

  if (valid && !opened) {
    opened = git_repository_open(&repository, git_path.string().c_str()) == 0;
    find_head_and_origin();
    has_walker = git_revwalk_new(&walker, repository) == 0;
  }

  count_head_and_origin();
}

void GitWalker::find_head_and_origin() {
  git_branch_iterator* it;
  git_reference* branch;
  git_branch_t out_type = GIT_BRANCH_LOCAL;

  git_branch_iterator_new(&it, repository, out_type);

  while (!git_branch_next(&branch, &out_type, it)) {
    bool is_head = git_branch_is_head(branch) == 1 ? true : false;

    const char* name = nullptr;
    git_branch_name(&name, branch);

    if (is_head) {
      head_name = name;
      head_ref = branch;
      head_ref_name = git_reference_name(head_ref);
      has_origin = git_branch_upstream(&origin_ref, head_ref) == 0;

      if (has_origin) {
        const char* o_name;
        git_branch_name(&o_name, origin_ref);
        origin_name = o_name;
        origin_ref_name = git_reference_name(origin_ref);
      }

      break;  // Head found
    }
  }

  git_branch_iterator_free(it);
}

void GitWalker::count_head_and_origin() {
  if (has_walker) {
    head_commits = count_commits(head_ref_name);
    if (has_origin) origin_commits = count_commits(origin_ref_name);
  }
}

int GitWalker::count_commits(const std::string& ref_name) const {
  int commits = 0;
  git_oid oid = {0};

  git_revwalk_push_ref(walker, ref_name.c_str());

  while (!git_revwalk_next(&oid, walker)) commits++;

  git_revwalk_reset(walker);
  return commits;
}

std::string GitWalker::string() const { return path.string(); }
