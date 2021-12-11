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
            std::cout << "New Accept" << std::endl;
            src_socket_ = std::move(socket);
            io_context_.notify_fork(boost::asio::io_context::fork_prepare);
            pid_t pid;
            while ((pid = fork()) < 0 ) { usleep(1000); }
            if (pid == 0) {  // child
              io_context_.notify_fork(boost::asio::io_context::fork_child);
              acceptor_.close();
              signal_.cancel();
              do_read_sock4();
            } else {  // parent
              io_context_.notify_fork(boost::asio::io_context::fork_parent);
              src_socket_.close();
              do_accept();
            }
          } else {
            boost::asio::detail::throw_error(ec);
            do_accept();
          }
        });
  }

  void do_read_sock4() {
    std::size_t length = src_socket_.read_some(boost::asio::buffer(sock4_data_, 1024));
    if (length >= 9) {
      socks4::request socks_request(sock4_data_, length);
      std::cout << socks_request.getDestHost() << std::endl;
      std::cout << socks_request.getDestPort() << std::endl;
      // TODO(libos) Check the firewall (socks.conf), and send SOCKS4 REPLY to the SOCKS client if rejected
      if (socks_request.getcommand() == socks4::request::command_type::connect) {
        std::cout << "Connect mode" << std::endl;
        auto endpoint = resolver_.resolve(socks_request.getDestHost(), socks_request.getDestPort());
        boost::asio::connect(dest_socket_, endpoint);
        socks4::reply socks_reply(socks4::reply::status_type::request_granted);
        boost::asio::write(src_socket_, socks_reply.buffers());
        std::cout << "write successfull" << std::endl;
        do_read_from_dest();
        do_read_from_src();
      } else {
        std::cout << "Bind mode" << std::endl;
        // TODO(libos) Bind mode
      }
    } else {
      throw std::invalid_argument("Not sock4 format");
    }
  }

  void do_read_from_dest() {
    // std::cout << "do_read_from_dest" << std::endl;
    dest_socket_.async_read_some(boost::asio::buffer(data_from_dest_, 65536),
    [this](boost::system::error_code ec, std::size_t length){
      if (!ec) {
        std::cout << "length" << length << std::endl;
        // std::cout << "do_read_from_dest_handler" << std::endl;
        do_write_to_src(length);
      } else {
        boost::asio::detail::throw_error(ec);
      }
    });
  }

  void do_write_to_src(std::size_t length) {
    // std::cout << "do_write_to_src" << std::endl;
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
    // std::cout << "do_read_from_src" << std::endl;
    src_socket_.async_read_some(boost::asio::buffer(data_from_src_, 65536),
    [this](boost::system::error_code ec, std::size_t length){
      if (!ec) {
        // std::cout << "do_read_from_src_handler" << std::endl;
        do_write_to_dest(length);
      } else {
        boost::asio::detail::throw_error(ec);
      }
    });
  }

  void do_write_to_dest(std::size_t length) {
    // std::cout << "do_write_to_dest" << std::endl;
    boost::asio::async_write(dest_socket_, boost::asio::buffer(data_from_src_, length),
    [this](boost::system::error_code ec, std::size_t /*length*/){
      if (!ec) {
        // std::cout << "do_write_to_dest_handler" << std::endl;
        do_read_from_src();
      } else {
        boost::asio::detail::throw_error(ec);
      }
    });
  }

  unsigned char sock4_data_[1024];
  char data_from_src_[65536];
  char data_from_dest_[65536];
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
