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

#include <boost/filesystem/path.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "src/args.h"
#include "src/language.h"
#include "src/session.h"

Session::Session(const Args *args, GitWalker *walker, const Language *lang_serv,
                 const int client_id)
    : args(args), hooker(args), lang_serv(lang_serv), client_id(client_id) {
  walker->update_stats();  // GitWalker

  std::cout << "Caching a new repository at " << walker->path << "\n";

  if (!walker->valid) {
    delete walker;
    return;  // Means not a git repo
  }

  dirty_path = walker->string();
  // Add path to hooker and automatically implant the post-commit hook
  hooker.update_path(walker->git_path);
  update_stats(walker);

  // Walker is a pointer, to prevent unecessarry work in Git::change_path()
  delete walker;
}

void Session::update() {
  if (!pretty_name.empty()) std::cout << " -- Updating " << pretty_name << "\n";

  GitWalker walker(args, dirty_path);

  if (!walker.valid) {
    if (valid) std::cout << "Session " << name << " has become invalid\n";
    return zero_stats();
  }

  // Make sure dirty path always points to the repository,
  // after being used
  dirty_path = walker.string();

  // If repo previously was invalid, update the path for hooker
  if (!valid) hooker.update_path(walker.git_path);

  walker.update_stats();
  update_stats(&walker);
}

void Session::update_path(const std::string &path) {
  dirty_path = path;

  if (!args->auto_update)
    language = lang_serv->get_props(
        boost::filesystem::path(path).extension().string());
}

void Session::update_stats(const GitWalker *walker) {
  valid = walker->valid;
  id = walker->string();
  name = walker->name;
  path = walker->string();
  pretty_name = get_pretty_name();
  branch = walker->head_name;
  has_origin = walker->has_origin;

  if (walker->init_path != walker->path)
    language = lang_serv->get_props(walker->init_path.extension().string());

  // Prop inexplicitly created here
  BranchProp &branch_props = branches[branch];

  if (!branch_props.local_commits_calculated) {
    branch_props.local_comits_on_start = walker->head_commits;
    branch_props.local_commits_calculated = true;
  }

  branch_props.ahead_of_local =
      walker->head_commits - branch_props.local_comits_on_start;

  // Do some calculations, relative to remote origin
  if (has_origin) {
    if (!branch_props.remote_commits_calculated) {
      branch_props.remote_commits_on_start = walker->origin_commits;
      branch_props.pushed_to_remote =
          walker->origin_commits - branch_props.remote_commits_on_start;
      branch_props.ahead_of_remote =
          walker->head_commits - walker->origin_commits;

      branch_props.remote_commits_calculated = true;
    } else {
      branch_props.pushed_to_remote =
          walker->origin_commits - branch_props.remote_commits_on_start;
      branch_props.ahead_of_remote = walker->head_commits -
                                     walker->origin_commits +
                                     branch_props.pushed_to_remote;
    }
  } else {
    branch_props.remote_commits_calculated = false;

    branch_props.remote_commits_on_start = 0;
    branch_props.ahead_of_remote = 0;
    branch_props.pushed_to_remote = 0;
  }
}

void Session::zero_stats() {
  valid = false;
  has_origin = false;
  language = nullptr;

  branches.clear();
}

std::string Session::get_pretty_name() const {
  if (name.empty()) return name;  // Return early, if nothing to prettify

  // Don't alter the name, if it has underscores
  bool should_alter = true;
  int size = name.size();

  for (int i = 0; i < size; i++) {
    if (name[i] == '_') should_alter = false;
  }

  if (!should_alter) return name;  // Return early, if alter == false

  std::string new_name(name);
  new_name[0] = toupper(new_name[0]);

  for (int i = 0; i < size; i++) {
    if (new_name[i] == '-') {
      new_name[i] = ' ';

      // Also capitalize the next char after it
      if (i + 1 < size) new_name[i + 1] = toupper(new_name[i + 1]);
    }
  }

  return new_name;
}

const BranchProp *Session::get_active_branch_props() const {
  return &branches.at(branch);
}
