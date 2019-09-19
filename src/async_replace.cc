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

#include <string>

#include "src/async_replace.h"
#include "src/server.h"

namespace http = boost::beast::http;

AsyncReplace::AsyncReplace(boost::asio::io_service *io, Server *server)
    : resolver(*io),
      socket(*io),
      timer(*io, boost::posix_time::seconds(5)),
      server(server) {}

void AsyncReplace::begin_timeout() {
  timer.async_wait(std::bind(&AsyncReplace::close_socket, shared_from_this()));
}

void AsyncReplace::shutdown(const int port) {
  std::string address = "127.0.0.1:" + std::to_string(port);
  auto const results = resolver.resolve("127.0.0.1", std::to_string(port));

  boost::asio::connect(socket, results.begin(), results.end());
  begin_timeout();  // 5 seconds

  http::request<http::string_body> req;
  nlohmann::json shutdown_json{
      {"id", 0},
      {"method", "shutdown"},
  };

  req.version(11);
  req.target(address);
  req.method(http::verb::post);

  req.set(http::field::server, "Presence");
  req.set(http::field::content_type, "application/json");
  req.body() = shutdown_json.dump();

  req.prepare_payload();
  http::write(socket, req);

  http::async_read(socket, buffer, res,
                   std::bind(&AsyncReplace::handle_response, shared_from_this(),
                             std::placeholders::_1, std::placeholders::_2));
}

void AsyncReplace::handle_response(boost::system::error_code ec,
                                   std::size_t bytes_transferred) {
  std::cout << "Bytes transferred: " << bytes_transferred << "\n";

  boost::ignore_unused(bytes_transferred);
  timer.cancel();  // Invokes close_socket()
}

void AsyncReplace::close_socket() {
  try {
    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
  } catch (const boost::system::system_error &error) {
    // Assume its already closed
  }

  // All good, start the server
  server->start();
}
