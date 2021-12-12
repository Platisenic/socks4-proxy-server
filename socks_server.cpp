#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdexcept>
#include <boost/asio.hpp>
#include "socks4.hpp"

using boost::asio::ip::tcp;

class socks_server {
 public:
  socks_server(boost::asio::io_context& io_context, unsigned short port) :
    src_socket_(io_context),
    dest_socket_(io_context),
    acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
    resolver_(io_context),
    signal_(io_context, SIGCHLD),
    io_context_(io_context) {
      wait_for_signal();
      do_accept();
  }

 private:
  void wait_for_signal() {
    signal_.async_wait(
        [this](boost::system::error_code /*ec*/, int /*signo*/){
            if (acceptor_.is_open()) {
              int status = 0;
              while (waitpid(-1, &status, WNOHANG) > 0) {}
              wait_for_signal();
            }
        });
  }

  void do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket){
          if (!ec) {
            src_socket_ = std::move(socket);
            io_context_.notify_fork(boost::asio::io_context::fork_prepare);

            pid_t pid;
            while ((pid = fork()) < 0 ) { usleep(1000); }

            if (pid == 0) {  // child
              io_context_.notify_fork(boost::asio::io_context::fork_child);
              acceptor_.close();
              signal_.cancel();
              try {
                do_read_sock4();
              } catch (std::exception& e) {
                src_socket_.close();
                dest_socket_.close();
              }
            } else {  // parent
              io_context_.notify_fork(boost::asio::io_context::fork_parent);
              src_socket_.close();
              do_accept();
            }
          } else {
            do_accept();
          }
        });
  }

  void do_read_sock4() {
    std::size_t length = src_socket_.read_some(boost::asio::buffer(sock4_data_, 1024));

    if (length >= 9 && sock4_data_[0] == 0x4) {
      socks4::request socks_request(sock4_data_, length);
      auto dest_endpoint = resolver_.resolve(socks_request.getDestHost(), socks_request.getDestPort());
      std::cout << "<S_IP>: " << src_socket_.remote_endpoint().address().to_string() << std::endl;
      std::cout << "<S_PORT>: " << src_socket_.remote_endpoint().port() << std::endl;
      std::cout << "<D_IP>: " << dest_endpoint.begin()->endpoint().address().to_string() << std::endl;
      std::cout << "<D_PORT>: " << dest_endpoint.begin()->endpoint().port() << std::endl;

      // TODO(libos) Check the firewall (socks.conf), and send SOCKS4 REPLY to the SOCKS client if rejected

      if (socks_request.getcommand() == socks4::request::command_type::connect) {
        std::cout << "<Command>: CONNECT" << std::endl;
        boost::asio::connect(dest_socket_, dest_endpoint);
        socks4::reply socks_reply(socks4::reply::status_type::request_granted);
        boost::asio::write(src_socket_, socks_reply.buffers());
        do_read_from_dest();
        do_read_from_src();
      } else if (socks_request.getcommand() == socks4::request::command_type::bind) {
        std::cout << "<Command>: BIND" << std::endl;
        tcp::acceptor acceptor_appserver(io_context_, tcp::endpoint(tcp::v4(), 0));
        socks4::reply socks_reply_success(
          socks4::reply::status_type::request_granted,
          acceptor_appserver.local_endpoint().port(),
          acceptor_appserver.local_endpoint().address().to_v4().to_bytes());

        boost::asio::write(src_socket_, socks_reply_success.buffers());

        acceptor_appserver.accept(dest_socket_);

        if (dest_socket_.remote_endpoint().address().to_string() !=
            dest_endpoint.begin()->endpoint().address().to_string()) {
          socks4::reply socks_reply_fail(socks4::reply::status_type::request_failed);
          boost::asio::write(src_socket_, socks_reply_fail.buffers());
          throw std::runtime_error("Accept dest port is not as same as bind dest");
        } else {
          boost::asio::write(src_socket_, socks_reply_success.buffers());
          do_read_from_dest();
          do_read_from_src();
        }
      } else {
        throw std::runtime_error("Not connect or bind mode");
      }
    } else {
      throw std::runtime_error("Not sock4 format");
    }
  }

  void do_read_from_dest() {
    dest_socket_.async_read_some(boost::asio::buffer(data_from_dest_, MaxBufferk),
    [this](boost::system::error_code ec, std::size_t length){
      if (!ec) {
        do_write_to_src(length);
      } else {
        boost::asio::detail::throw_error(ec);
      }
    });
  }

  void do_write_to_src(std::size_t length) {
    boost::asio::async_write(src_socket_, boost::asio::buffer(data_from_dest_, length),
    [this](boost::system::error_code ec, std::size_t /*length*/){
      if (!ec) {
        do_read_from_dest();
      } else {
        boost::asio::detail::throw_error(ec);
      }
    });
  }

  void do_read_from_src() {
    src_socket_.async_read_some(boost::asio::buffer(data_from_src_, MaxBufferk),
    [this](boost::system::error_code ec, std::size_t length){
      if (!ec) {
        do_write_to_dest(length);
      } else {
        boost::asio::detail::throw_error(ec);
      }
    });
  }

  void do_write_to_dest(std::size_t length) {
    boost::asio::async_write(dest_socket_, boost::asio::buffer(data_from_src_, length),
    [this](boost::system::error_code ec, std::size_t /*length*/){
      if (!ec) {
        do_read_from_src();
      } else {
        boost::asio::detail::throw_error(ec);
      }
    });
  }

  unsigned char sock4_data_[1024];
  enum { MaxBufferk = 65536 };
  char data_from_src_[MaxBufferk];
  char data_from_dest_[MaxBufferk];
  tcp::socket src_socket_;
  tcp::socket dest_socket_;
  tcp::acceptor acceptor_;
  tcp::resolver resolver_;
  boost::asio::signal_set signal_;
  boost::asio::io_context& io_context_;
};

int main(int argc, char* argv[]) {
  try {
    if (argc != 2) {
      std::cerr << "Usage: socks_server <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;
    socks_server s(io_context, std::atoi(argv[1]));
    io_context.run();
  } catch (std::exception& e) {
    // std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
