#include <boost/filesystem.hpp>

#include <fstream>
#include <iostream>
#include <string>

#include "src/args.h"
#include "src/git_hook.h"

namespace filesystem = boost::filesystem;

GitHook::GitHook(const Args *args) : port(args->port) {}

GitHook::~GitHook() {
  for (HookProp &hook : hooks) {
    if (!hook.implanted) continue;

    try {
      cleanup_hook(hook);
    } catch (const boost::system::system_error &error) {
      // Do nothing
    }
  }
}

void GitHook::update_path(const filesystem::path &git_path) {
  filesystem::path new_path = git_path / "hooks";

  if (new_path != hooks_dir) {
    hooks_dir = new_path;
    create_hooks();
  }

  implant();
}

void GitHook::create_hooks() {
  // If there are any hooks, first attempt cleaning them up
  if (!hooks.empty()) {
    for (const HookProp &hook : hooks)
      if (hook.implanted) cleanup_hook(hook);

    hooks.clear();
  }

  for (const std::string &name : hook_names)
    hooks.push_back(HookProp(this, name));
}

void GitHook::implant() {
  for (HookProp &hook : hooks) {
    if (hook.implanted) continue;

    try {
      implant_hook(hook);
      hook.implanted = true;
    } catch (const boost::system::system_error &error) {
      // Do nothing
    }
  }
}

void GitHook::implant_hook(const HookProp &hook) {
  std::cout << "Adding a hook to " << hook.path.string() << "\n";

  if (!filesystem::exists(hook.path)) {
    filesystem::ofstream stream(hook.path);

    stream << shebang << "\n\n";
    stream << hook.script;
    stream.close();

    return set_owner_exe(hook.path);
  }

  filesystem::ifstream input_stream(hook.path);
  std::stringstream script;
  std::string buffer;
  bool write = true;
  bool first_line = true;

  while (std::getline(input_stream, buffer)) {
    if (first_line) {  // Check if shebang exists, if not, insert one
      if (buffer.find("#!/") == std::string::npos) script << shebang << "\n\n";
      first_line = false;
    }

    if (buffer == header) write = false;
    if (write) script << buffer << "\n";
    if (buffer == footer) write = true;
  }

  script << hook.script;
  input_stream.close();

  filesystem::ofstream output_stream(hook.path);
  output_stream << script.str();
  output_stream.close();

  set_owner_exe(hook.path);
}

void GitHook::cleanup_hook(const HookProp &hook) {
  filesystem::ifstream input_stream(hook.path);
  bool write = true;
  std::stringstream contents;
  std::string buffer;

  while (std::getline(input_stream, buffer)) {
    if (buffer == "#+PRESENCE_BEGIN") write = false;
    if (write) contents << buffer << "\n";
    if (buffer == "#+PRESENCE_END") write = true;
  }

  input_stream.close();

  filesystem::ofstream output_stream(hook.path);

  output_stream << contents.str();
  output_stream.close();
}

void GitHook::set_owner_exe(const filesystem::path &path) const {
#if defined(BOOST_POSIX_API)
  filesystem::permissions(
      path, filesystem::perms::add_perms | filesystem::perms::owner_all);
#endif
}
