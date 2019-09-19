#ifndef SRC_GIT_HOOK_H_
#define SRC_GIT_HOOK_H_

#include <boost/filesystem/path.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "src/args.h"

struct HookProp;
class GitHook {
 public:
  explicit GitHook(const Args *args);
  ~GitHook();

  int port;
  boost::filesystem::path hooks_dir;
  const std::string header = "#+PRESENCE_BEGIN";
  const std::string footer = "#+PRESENCE_END";

  const std::string notice =
      "# This piece of code was automatically inserted by presence server\n"
      "#\n"
      "# It will automatically clean up itself, when the server exists,\n"
      "# please don't touch the PRESENCE_BEGIN/END lines\n"
      "#\n"
      "# https://github.com/volskaya/presence\n";

  void update_path(const boost::filesystem::path &git_path);
  void implant();

 private:
  // Assumes whatever is inside the script, can be executed with sh
  const std::string shebang = "#!/bin/sh";
  const std::vector<std::string> hook_names{
      "post-commit",  "post-rebase", "post-checkout", "post-push",
      "post-rewrite", "post-update", "post-merge"};

  std::vector<HookProp> hooks;

  void create_hooks();
  void implant_hook(const HookProp &hook);
  void cleanup_hook(const HookProp &hook);
  void set_owner_exe(const boost::filesystem::path &path) const;
};

struct HookProp {
  explicit HookProp(const GitHook *hook, const std::string &name)
      : name(name), path(hook->hooks_dir / name) {
    script =
        hook->header + "\n" + hook->notice +
        "DIR=$(cd $(dirname ${BASH_SOURCE[0]}) > /dev/null && pwd)\n"
        "GIT_NAME=$(basename $(dirname $(dirname $DIR)))\n"
        "curl -X POST -H \"Content-Type:application/json\" --data "
        "\"{\\\"id\\\":0,\\\"method\\\":\\\"git_change\\\",\\\"params\\\":{"
        "\\\"name\\\":\\\"$GIT_NAME\\\",\\\"signal\\\":\\\"" +
        name +
        "\\\"}}\""
        " http://127.0.0.1:" +
        std::to_string(hook->port) + " >/dev/null\n" + hook->footer + "\n";
  };

  std::string name;
  boost::filesystem::path path;
  std::string script = "";
  bool implanted = false;
};

#endif  // SRC_GIT_HOOK_H_
