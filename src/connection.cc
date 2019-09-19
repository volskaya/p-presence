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

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/system/error_code.hpp>

#include <iostream>
#include <sstream>
#include <string>

#include "json/single_include/nlohmann/json.hpp"
#include "src/connection.h"
#include "src/git.h"
#include "src/server.h"

namespace ip = boost::asio::ip;
namespace http = boost::beast::http;

Connection::Connection(boost::asio::io_context *io, Server *server)
    : server(server), socket(*io) {
  std::cout << "New connection spawned\n";
}

void Connection::start() {
  std::cout << "Awaiting input\n";
  await_read();
}

void Connection::await_read() {
  request = {};

  http::async_read(
      socket, buffer, request,
      [this, _ = shared_from_this()](const boost::system::error_code &error,
                                     const long unsigned int whats_this) {
        if (error == http::error::end_of_stream) return;  // TODO: Close socket

        handle_read();
      });
}

void Connection::handle_read() {
  try {
    if (request.method() != http::verb::post ||
        request.at(http::field::content_type) != "application/json") {
      return bad_request("Server only accepts application/json POST requests");
    }
  } catch (const std::exception &error) {
    // request.at() fails if field doesn't exist
    return bad_request("Server only accepts application/json POST requests");
  }

  std::stringstream body(request.body());
  std::cout << "Receiving a POST request\n"
            << "Body contains: " << body.str() << "\n";

  nlohmann::json json_req;

  try {
    json_req = nlohmann::json::parse(request.body().data());
  } catch (const nlohmann::detail::parse_error &error) {
    std::cout << "Failed to parse body, skipping\n";
    return bad_request("JSON parse failed");
  }

  if (json_req.is_null()) {
    std::cout << "JSON is empty or invalid\n";
    return bad_request("JSON is empty or invalid");
  }

  if (!json_req["method"].is_string() || !json_req["id"].is_number_integer()) {
    return bad_request(
        "Presence server expects all JSON requests"
        " to contain a string 'method' and int 'id' fields");
  }

  return handle_method(json_req);
}

void Connection::handle_method(const nlohmann::json &json_req) {
  std::string name = json_req["method"];
  int id = json_req["id"];

  std::cout << "Handling method: " << name << ", from client ID: " << id
            << "\n";

  if (name == "set_path") {
    handle_set_path(json_req);
  } else if (name == "get_path") {
    handle_get_path();
  } else if (name == "is_running") {
    handle_is_running();
  } else if (name == "ping") {
    handle_ping(id);
  } else if (name == "git_change") {
    handle_git_change(json_req);
  } else if (name == "im_leaving") {
    handle_im_leaving(id);
  } else if (name == "handle_get_info") {
    handle_get_info();
  } else if (name == "shutdown") {
    handle_shutdown();
  }
}

ip::tcp::socket &Connection::get_socket() { return socket; }

void Connection::bad_request(const std::string &message) {
  send_response(http::status::bad_request, "{\"error\": \"" + message + "\"}");
}

void Connection::send_response(const boost::beast::http::status &status,
                               const std::string &message) {
  res = http::response<http::string_body>(status, request.version());

  res.set(http::field::server, "Presence");
  res.set(http::field::content_type, "application/json");

  res.body() = message;
  res.prepare_payload();

  http::async_write(
      socket, res,
      [_ = shared_from_this()](const boost::system::error_code &error,
                               const long unsigned int whats_this) {});
}

// Actions
void Connection::handle_set_path(const nlohmann::json &json_req) {
  std::string error_message =
      "'set_path' method requires 'params', holding 'path' string";

  if (!json_req["params"].is_object()) return bad_request(error_message);

  nlohmann::json params = json_req["params"];

  if (!params["path"].is_string()) return bad_request(error_message);

  // Procceed…
  ChangeProp props(json_req["id"], params["path"]);

  // Optional editor name parameter
  if (json_req["editor"].is_string()) props.editor_name = json_req["editor"];

  nlohmann::json res = server->set_path(props);

  send_response(http::status::ok, res.dump());
}

void Connection::handle_get_path() {
  send_response(http::status::ok,
                nlohmann::json{{"path", server->get_path()}}.dump());
}

void Connection::handle_is_running() {
  send_response(http::status::ok, "{\"status\": true}");
}

void Connection::handle_ping(const int id) {
  send_response(http::status::ok, server->ping(id).dump());
}

void Connection::handle_git_change(const nlohmann::json &json_req) {
  std::string error_message =
      "'git_change' method requires 'params', holding 'name' of the git repo, "
      "the hook script was ran from and 'signal', holding the name of the hook";

  if (!json_req["params"].is_object()) return bad_request(error_message);

  nlohmann::json params = json_req["params"];

  if (!params["name"].is_string()) return bad_request(error_message);

  send_response(http::status::ok, "{\"status\": true}");
  server->git_change(params["name"]);
}

void Connection::handle_im_leaving(const int id) {
  // If client wasn't previously tracked, it will return false.
  // Don't have any use for this atm, but gonna leave it here anway
  send_response(http::status::ok,
                nlohmann::json{{"status", server->im_leaving(id)}}.dump());
}

// Returns data for the active github session
void Connection::handle_get_info() {
  send_response(http::status::ok, server->get_info().dump());
}

void Connection::handle_shutdown() {
  std::cout << "Shutdown request received…'n";

  server->release_port();
  send_response(http::status::ok, server->get_info().dump());

  // Program will exit here
  server->shutdown();
}
