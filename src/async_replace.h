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

#ifndef SRC_ASYNC_REPLACE_H_
#define SRC_ASYNC_REPLACE_H_

#include <boost/asio.hpp>
#include <boost/beast.hpp>

class Server;
class AsyncReplace : public std::enable_shared_from_this<AsyncReplace> {
 public:
  explicit AsyncReplace(boost::asio::io_service* io, Server* server);

  void shutdown(const int port);

 private:
  boost::asio::ip::tcp::resolver resolver;
  boost::asio::ip::tcp::socket socket;
  boost::beast::flat_buffer buffer;
  boost::beast::http::response<boost::beast::http::string_body> res;
  boost::asio::deadline_timer timer;
  Server* server;

  void begin_timeout();
  void close_socket();
  void handle_response(boost::system::error_code ec,
                       std::size_t bytes_transferred);
};

#endif  // SRC_ASYNC_REPLACE_H_
