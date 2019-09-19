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

#ifndef SRC_LANGUAGE_H_
#define SRC_LANGUAGE_H_

#include "json/single_include/nlohmann/json.hpp"

#include <string>
#include <unordered_map>

struct LangProps {
  std::string name = "";
  std::string *color = nullptr;
  int id = 0;
};

// This gets mapped to extensions, when lang json gets parsed
typedef std::shared_ptr<LangProps> PropsPointer;

class Language {
  // 1. group name, ie. JavaScript, 2. color ie. #FFFFFF
  typedef std::unordered_map<std::string, std::string> Colors;

  // 1. extension, 2. pointer to their lang props
  typedef std::unordered_map<std::string, PropsPointer> Extensions;

 public:
  explicit Language();

  std::string get_color(const std::string &ext) const;
  PropsPointer get_props(const std::string &ext) const;

 private:
  Colors colors;
  Extensions extension_map;
  nlohmann::json json;

  std::string find_group(const std::string &lang) const;

  void parse_lanaguages_json();
  void parse_languages();
  void parse_extensions();
  void parse_colors();
};

#endif  // SRC_LANGUAGE_H_
