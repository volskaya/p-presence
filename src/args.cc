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
#include <boost/program_options.hpp>

#include <iostream>
#include <string>

#include "src/args.h"
#include "src/utils.h"

namespace po = boost::program_options;
namespace filesystem = boost::filesystem;

Args::Args(const int &argc, char *argv[]) {
  std::string intro =
      title + " - " + repository + +"\n" + note + "\n" + example;

  description = new po::options_description(intro);
  construct_description();

  try {
    create_arguments(argc, argv);
    parse_arguments();
  } catch (const po::invalid_option_value &error) {
    std::cout << "Error: " << error.what() << "\n";
    errors++;
  } catch (const po::unknown_option &error) {
    std::string message = error.what();
    message[0] = std::toupper(message[0]);  // Uppercase

    std::cout << message << "\n";
    errors++;
  }

  delete description;
}

void Args::construct_description() {
  description->add_options()("id,i", po::value<int>(&id)->value_name(ID),
                             "Custom presence ID")

      // ("verbose,v", po::bool_switch(&verbose), "Print progress to stdout")

      ("replace,r", po::bool_switch(&replace), "Replace existing instance")

          ("port,p", po::value<int>(&port)->value_name(std::to_string(port)),
           "Listening port for the RPC")

              ("language,l,omnicode",
               po::value<std::string>(&language)->value_name(language),
               "Language to display")

                  ("custom-icons,I", po::bool_switch(&custom_icons),
                   "Use repository name as picture name for the big "
                   "presence icon. "
                   "For this feature to work, you must point the server to "
                   "your own application ID and upload your assets there")

                      ("disable-editor-icons",
                       po::bool_switch(&no_editor_icons),
                       "Disables editor specific icons for big presence icon")

                          ("auto-update,U", po::bool_switch(&auto_update),
                           "Automatically update cached git session every "
                           "15 seconds, "
                           "without waiting for a client to call \"saved\" "
                           "method")

                              ("dont-await-repo",
                               po::bool_switch(&dont_await_repo),
                               "Don't wait for any client to submit a "
                               "repository, but "
                               "send out a presence, indicating you're "
                               "working on something private."
                               "This will prevent the server from shutting "
                               "down, until initial"
                               "commit is found")

                                  ("prevent-shutdown",
                                   po::bool_switch(&prevent_shutdown),
                                   "Prevent the server from shutting down, "
                                   "when 0 clients remaining")

                                      ("help,h", "Print arguments")(
                                          "version", "Print version number");
}

void Args::create_arguments(const int &argc, char *argv[]) {
  po::store(po::parse_command_line(argc, argv, *description), variables);
  po::notify(variables);
}

void Args::parse_arguments() {
  if (variables.count("help")) {
    std::cout << *description << "\n";

    // Pretend theres an error, so main doesn't continue spawning
    // the server, etc…
    errors++;
    exit_type = ExitType::CLEAN;

    return;
  }

  if (variables.count("version")) {
    std::cout << VERSION << "\n";

    errors++;
    exit_type = ExitType::CLEAN;

    return;
  }

  if (variables.count("id")) {
    id = variables["id"].as<int>();
    std::cout << "Custom presence ID set to: " << id << "\n";
  }

  if (variables.count("verbose")) {
    verbose = variables["verbose"].as<bool>();

    if (verbose) std::cout << "Verbose output enabled\n";
  }

  if (variables.count("replace")) {
    replace = variables["replace"].as<bool>();

    if (replace)
      std::cout << "Replacing existing instances, "
                << "upon spawning the server…\n";
  }

  if (variables.count("port")) {
    port = variables["port"].as<int>();
    std::cout << "RPC port changed to: " << port << "\n";
  }

  if (variables.count("language")) {
    language = variables["language"].as<std::string>();
    override_language = true;

    std::cout << "Language overriden to: " << language << "\n";
  }

  if (variables.count("custom-icons")) {
    custom_icons = variables["custom-icons"].as<bool>();

    if (custom_icons) std::cout << "Custom repository icons enabled\n";
  }

  if (variables.count("disable-editor-icons")) {
    no_editor_icons = variables["disable-editor-icons"].as<bool>();

    if (no_editor_icons) std::cout << "Disabling custom editor icons\n";
  }

  if (variables.count("auto-update")) {
    auto_update = variables["auto-update"].as<bool>();

    if (auto_update)
      std::cout << "Git sessions will be rescaned every 15 seconds\n";
  }

  if (variables.count("dont-await-repo")) {
    dont_await_repo = variables["dont-await-repo"].as<bool>();

    if (dont_await_repo)
      std::cout << "Immediately sending out a presence, "
                << "without awaiting a valid repository\n";
  }

  if (variables.count("prevent-shutdown")) {
    prevent_shutdown = variables["prevent-shutdown"].as<bool>();

    if (prevent_shutdown)
      std::cout << "Server won't shutdown, when 0 clients remaining\n";
  }
}

bool Args::exit_cleanly() const { return exit_type == ExitType::CLEAN; }
