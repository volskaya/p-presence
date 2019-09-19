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

#ifndef SRC_ARGS_H_
#define SRC_ARGS_H_

#include <boost/program_options.hpp>

#include <string>

class Args {
  enum ExitType { CLEAN, DIRTY };

 public:
  //! Constructors
  explicit Args(const int &argc, char *argv[]);

  Args(const Args &other) = delete;
  Args(Args &&other) noexcept = delete;
  virtual ~Args() noexcept = default;
  Args &operator=(const Args &other) = delete;
  Args &operator=(Args &&other) noexcept = delete;

  int errors = 0;
  bool exit_cleanly() const;

  //! Argument variables
  std::string language = "Omnicode";  // Language name override
  bool override_language = false;
  bool verbose = false;
  bool custom_icons = false;      // Don't use custom repository icons
  bool no_editor_icons = false;   // Disable editor icons
  bool auto_update = false;       // Rescan cached git sessions every 15 seconds
  bool replace = false;           // Replace running instance
  bool prevent_shutdown = false;  // Prevent shutdown, when 0 clients remaining
  int id = 0;                     // Discord app ID, with rich presence
  int port = 8080;                // RPC port

  // Immediately send out presence, but if there's no git repo locked
  // in, the presence will just say that you're working on a private repo
  bool dont_await_repo = false;

 private:
  boost::program_options::options_description *description = nullptr;
  boost::program_options::variables_map variables;
  ExitType exit_type = ExitType::DIRTY;

  //! Description
  std::string title = "Presence";
  std::string repository = "github.com/volskaya/presence\n";

  std::string note =
      "Note: When --auto-update is off, presence injects a curl post-commit "
      "hook into any repository it scans, which emits an update notice, to "
      "correctly update presence after a commit.\n";

  std::string example = "Example:\n./presence --prevent-shutdown --port 8080";

  //! Methods
  void construct_description();
  void create_arguments(const int &argc, char *argv[]);
  void parse_arguments();
};

#endif  // SRC_ARGS_H_
