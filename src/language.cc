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

#include <boost/algorithm/string.hpp>
#include "json/single_include/nlohmann/json.hpp"

#include <iostream>
#include <string>

#include "src/language.h"
#include "src/utils.h"

void fix_color_name(std::string &color) {
  if (color[0] == '#') color[0] = '_';

  // Lowercase
  std::transform(color.begin(), color.end(), color.begin(), ::tolower);
}

Language::Language() {
  std::cout << "Parsing language json\n";
  json = nlohmann::json::parse(LANGUAGE_JSON);

  parse_colors();

  for (auto it = json.begin(); it != json.end(); it++) {
    auto key = it.key();
    auto value = it.value();
    bool has_group = value["group"].is_string();
    bool has_color = value["color"].is_string();
    bool has_extensions = value["extensions"].is_array();

    //  Don't continue, if the langauge is not supposed to have any colors
    if (!has_group && !has_color) continue;

    // Group indicates a child language, which doesn't have its own color,
    // but uses its groups color, which is a major language, like Java
    std::string group = find_group(key);

    // Some rare entries may point to groups with no colors. Skip them
    if (!has_color && !colors.count(group)) continue;

    PropsPointer props = std::make_shared<LangProps>();
    props->color = &colors[group];
    props->id = value["language_id"];

    // Discord doesn't seem to update presence, with 1 character key
    props->name = key.size() == 1 ? key + " " : key;

    if (has_extensions) {
      for (auto ext : value["extensions"])  // Don't overwrite
        if (!extension_map.count(ext)) extension_map[ext] = props;
    }
  }

  std::cout << "Parsed " << extension_map.size() << " extensions, "
            << colors.size() << " language colors in total\n";

  if (colors.size() > ASSET_LIMIT)
    std::cout << "Note: " << colors.size() << " is over Discord's "
              << ASSET_LIMIT << " assets limit\n";
}

void Language::parse_colors() {
  for (auto it = json.begin(); it != json.end(); it++) {
    auto key = it.key();
    auto value = it.value();
    bool has_group = value["group"].is_string();
    bool has_color = value["color"].is_string();

    // Color keys are their language names
    if (!has_group && has_color) {
      colors[key] = value["color"];
      fix_color_name(colors[key]);
    }
  }
}

std::string Language::find_group(const std::string &lang) const {
  std::string target = lang;

  for (;;) {
    if (json[target].is_object() && json[target]["group"].is_string())
      target = json[target]["group"];
    else
      break;
  }

  return target;
}

std::string Language::get_color(const std::string &ext) const {
  try {
    return *extension_map.at(ext)->color;
  } catch (const std::out_of_range &error) {
    return "";
  }
}

PropsPointer Language::get_props(const std::string &ext) const {
  try {
    return extension_map.at(ext);
  } catch (const std::out_of_range &error) {
    return nullptr;
  }
}
