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

#ifndef SRC_CONNECTION_H_
#define SRC_CONNECTION_H_

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/system/error_code.hpp>

#include <iostream>
#include <string>

#include "json/single_include/nlohmann/json.hpp"

class Server;
class Connection : public std::enable_shared_from_this<Connection> {
 public:
  Connection(boost::asio::io_context *io, Server *server);

  void start();
  boost::asio::ip::tcp::socket &get_socket();

 private:
  Server *server;
  boost::asio::ip::tcp::socket socket;
  boost::beast::flat_buffer buffer;
  boost::beast::http::request<boost::beast::http::string_body> request;
  boost::beast::http::response<boost::beast::http::string_body> res;

  void await_read();
  void handle_read();
  void bad_request(const std::string &message);
  void handle_method(const nlohmann::json &json_req);
  void send_response(const boost::beast::http::status &status,
                     const std::string &message);

  void handle_set_path(const nlohmann::json &json_req);
  void handle_get_path();
  void handle_is_running();
  void handle_ping(const int id);
  void handle_git_change(const nlohmann::json &json_req);
  void handle_im_leaving(const int id);
  void handle_get_info();
  void handle_shutdown();
};

#endif  // SRC_CONNECTION_H_
