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

#ifndef SRC_DISCORD_PRESENCE_H_
#define SRC_DISCORD_PRESENCE_H_

#include <iostream>
#include <string>

#include "json/single_include/nlohmann/json.hpp"
#include "src/utils.h"

struct DiscordPresence {
  // Words
  std::string state = "";    // "Working on …"
  std::string details = "";  // "Pushed 3 of 5"
  int time_stamp = 0;
  int pid = 0;

  // Images
  std::string large_image_key = "";
  std::string large_image_text = "";

  std::string small_image_key = "";
  std::string small_image_text = "";

  std::string to_string() const {
    nlohmann::json root;
    nlohmann::json args;
    nlohmann::json activity;

    root["cmd"] = "SET_ACTIVITY";

    if (!details.empty()) activity["details"] = details;
    if (!state.empty()) activity["state"] = state;

    activity["timestamps"] = {{"start", time_stamp}};

    if (!large_image_key.empty() || !small_image_key.empty()) {
      nlohmann::json assets;

      if (!large_image_key.empty()) {
        assets["large_image"] = large_image_key;

        if (!large_image_text.empty()) assets["large_text"] = large_image_text;
      }

      if (!small_image_key.empty()) {
        assets["small_image"] = small_image_key;

        if (!small_image_text.empty()) assets["small_text"] = small_image_text;
      }

      activity["assets"] = assets;
    }

    activity["instance"] = true;

    if (pid != 0)
      args["pid"] = pid;
    else
      args["pid"] = 9999;

    args["activity"] = activity;

    root["args"] = args;
    root["nonce"] = "aa8241d4-8806-4542-9ed6-6d6cb5d239eb";

    return root.dump();
  }
};

#endif  // SRC_DISCORD_PRESENCE_H_
