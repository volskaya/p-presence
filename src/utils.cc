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

#include <boost/asio/windows/stream_handle.hpp>

#include <string>

#include "src/utils.h"

#if defined(BOOST_WINDOWS_API)
std::wstring string_to_wide_string(const std::string& str) {
  int len;
  int slength = str.length() + 1;

  len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), slength, 0, 0);

  wchar_t* buf = new wchar_t[len];

  MultiByteToWideChar(CP_ACP, 0, str.c_str(), slength, buf, len);

  std::wstring r(buf);

  delete[] buf;
  return r;
}
#endif
