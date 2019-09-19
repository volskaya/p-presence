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

#include <memory>
#include <string>

#include "src/args.h"
#include "src/client.h"
#include "src/discord.h"
#include "src/discord_presence.h"
#include "src/git.h"
#include "src/utils.h"

Discord::Discord(const Args *args, const Git *git, Client *client)
    : git(git), args(args), client(client) {
  if (args->id != 0) id = std::to_string(args->id);

  if (args->override_language) {
    override_language = true;
    language = args->language;
  }

  dont_await_repo = args->dont_await_repo;
}

std::string Discord::build_state_string(const SessionPtr session) const {
  const BranchProp *branch_props = session->get_active_branch_props();

  if (session->has_origin) {
    if (branch_props->pushed_to_remote > 0) {
      return "Pushed " + std::to_string(branch_props->pushed_to_remote) +
             " of " + std::to_string(branch_props->ahead_of_remote);
    } else if (branch_props->ahead_of_remote > 0) {
      return "Ahead by " + std::to_string(branch_props->ahead_of_remote);
    } else if (branch_props->ahead_of_remote < 0) {
      return "Behind by " + std::to_string(branch_props->ahead_of_remote * -1);
    }
  }

  if (branch_props->ahead_of_local > 0)
    return "Ahead by " + std::to_string(branch_props->ahead_of_local);
  else if (branch_props->ahead_of_local < 0)
    return "Behind by " + std::to_string(branch_props->ahead_of_local);

  return "";
}

void Discord::update() {
  if (!alive) {
    start();
    return;
  }

  if (send_presence) {
    DiscordPresence discord_presence;

    // Reset time stamp, every time presence is resumed
    if (!has_presence) {
      start_time = std::time(0);
    }

    const ClientProp *editor = git->get_active_client();
    bool editor_icon_set = false;

    if (editor && !args->no_editor_icons) {
      if (!editor->editor_icon.empty()) {
        discord_presence.large_image_key = editor->editor_icon;
        editor_icon_set = true;
      }

      if (!editor->editor_name.empty())
        discord_presence.large_image_text = editor->editor_name;
    }

    if (!git->is_waiting()) {
      SessionPtr session = git->get_active_session();

      if (session->valid) {
        std::string state = build_state_string(session);
        std::string working_on = "Working on " + session->pretty_name;

        discord_presence.details = working_on;
        discord_presence.state = state;
        discord_presence.pid = session->client_id;

        std::cout << " - " << working_on << "\n";

        if (args->custom_icons) {
          discord_presence.large_image_text = session->pretty_name;
          discord_presence.large_image_key = session->name;
        } else {
          // Editor icons set above
          if (args->no_editor_icons || !editor_icon_set) {
            discord_presence.large_image_key = ICON_NAME;
            discord_presence.large_image_text = NAME;
          }
        }

        if (session->language) {
          discord_presence.small_image_text = session->language->name;
          discord_presence.small_image_key = *session->language->color;
        }
      } else {
        discord_presence.details = WORKING_ON_PRIVATE;
        if (args->no_editor_icons) discord_presence.large_image_key = ICON_NAME;
      }
    } else {
      std::cout << "Git still waiting\n";
      discord_presence.details = WORKING_ON_PRIVATE;
      if (args->no_editor_icons) discord_presence.large_image_key = ICON_NAME;
    }

    discord_presence.time_stamp = start_time;

    client->dispatch_presence(discord_presence);
    has_presence = true;
  } else {
    clear_presence();
  }
}

void Discord::start() {
  if (alive) return;

  alive = true;

  if (dont_await_repo || git->is_ready())
    update();
  else
    std::cout << "Waiting for a repository, before sending presence…\n";
}

void Discord::stop() {
  if (!alive) return;
  alive = false;
}

void Discord::clear_presence() {
  if (!has_presence) return;

  std::cout << "Clearing discord presence…\n";
  client->close_sockets();

  has_presence = false;
}

bool Discord::is_alive() const { return alive; }
bool Discord::has_no_presence() const { return !has_presence; }
