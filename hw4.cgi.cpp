#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "htmlmsg.hpp"
#include "socks4.hpp"

using boost::asio::ip::tcp;

class tcp_connection : public std::enable_shared_from_this<tcp_connection> {
 public:
  tcp_connection(boost::asio::io_context& io_context,
    int session,
    std::string input_file,
    unsigned short port,
    std::array<unsigned char, 4> address) :
    session(session),
    resolver_(io_context),
    socket_(io_context),
    file_ifs_(input_file.c_str()),
    socks_request_(socks4::request::command_type::connect, port, address),
    enable_socks_(false)
    {}

  void start(std::string host , std::string port, bool enable_socks) {
    try {
      auto self(shared_from_this());
      enable_socks_ = enable_socks;
      do_resolve(host, port);
    } catch (std::exception& e) {
      socket_.close();
    }
  }

  void do_resolve(std::string host , std::string port) {
    auto self(shared_from_this());
    resolver_.async_resolve(host, port,
    [this, self](boost::system::error_code ec, tcp::resolver::iterator endpoint_iterator){
      if (!ec) {
        endpoint_iterator_ = endpoint_iterator;
        do_connect();
      }
    });
  }

  void do_connect() {
    auto self(shared_from_this());
    socket_.async_connect(*endpoint_iterator_,
    [this, self](boost::system::error_code ec){
      if (!ec) {
        if (enable_socks_) {
          send_socks_request();
        } else {
          do_read();
        }
      }
    });
  }

  void send_socks_request() {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, socks_request_.buffers(),
    [this, self](boost::system::error_code ec, std::size_t /*length*/){
      if (!ec) {
        receive_socks_reply();
      }
    });
  }

  void receive_socks_reply() {
    auto self(shared_from_this());
    boost::asio::async_read(socket_, socks_reply_.buffers(),
    [this, self](boost::system::error_code ec, std::size_t /*length*/){
      if (!ec) {
        if (socks_reply_.success()) {
          do_read();
        }
      }
    });
  }

  void do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, MaxLengthk),
    [this, self](boost::system::error_code ec, std::size_t length){
      if (!ec) {
        std::string data(data_, data_+length);
        printShell(session, data);
        if (data.find("%") != std::string::npos) {
          do_write();
        } else {
          do_read();
        }
      }
    });
  }

  void do_write() {
    auto self(shared_from_this());
    std::getline(file_ifs_, write_buf_);
    write_buf_ += "\n";
    printCMD(session, write_buf_);
    boost::asio::async_write(socket_, boost::asio::dynamic_buffer(write_buf_),
    [this, self](boost::system::error_code ec, std::size_t /*length*/){
      if (!ec) {
        do_read();
      }
    });
  }

 private:
  int session;
  enum { MaxLengthk = 1024 };
  char data_[MaxLengthk];
  std::string write_buf_;
  tcp::resolver resolver_;
  tcp::resolver::iterator endpoint_iterator_;
  tcp::socket socket_;
  std::ifstream file_ifs_;
  socks4::request socks_request_;
  socks4::reply socks_reply_;
  bool enable_socks_;
};

struct connection_info{
  connection_info() {}
  void insert_element(char hpf, std::string value) {
    if (hpf == 'h') {
      host = value;
    } else if (hpf == 'p') {
      port = value;
    } else if (hpf == 'f') {
      filename = "test_case/" + value;
    }
  }

  bool is_valid() {
    return !host.empty() && !port.empty() && !filename.empty();
  }

  std::string host;
  std::string port;
  std::string filename;
};

struct socks_info{
  socks_info() {}
  void insert_element(char hp, std::string value) {
    if (hp == 'h') {
      host = value;
    } else if (hp == 'p') {
      port = value;
    }
  }

  bool is_valid() {
    return !host.empty() && !port.empty();
  }

  std::string host;
  std::string port;
};

int main() {
  try {
    boost::asio::io_context io_context;
    std::string query_string(getenv("QUERY_STRING"));
    std::stringstream ss(query_string);
    std::string parsed_string;
    connection_info conn_infos[5];
    tcp::resolver resolver(io_context);
    socks_info sk_info;

    while (std::getline(ss, parsed_string, '&')) {
      if (parsed_string[0] == 's') {  // socks proxy
        sk_info.insert_element(parsed_string[1], parsed_string.substr(3));
      } else {  // rwg info
        conn_infos[parsed_string[1]-'0'].insert_element(parsed_string[0], parsed_string.substr(3));
      }
    }

    printHtmltemplate();

    for (int i = 0 ; i < 5; i++) {
      if (conn_infos[i].is_valid()) {
        printTableTitle(i, conn_infos[i].host, conn_infos[i].port);
        auto dest_endpoint = resolver.resolve(conn_infos[i].host, conn_infos[i].port);
        auto tcp_conn_ptr = std::make_shared<tcp_connection>(
            io_context,
            i,
            conn_infos[i].filename,
            dest_endpoint.begin()->endpoint().port(),
            dest_endpoint.begin()->endpoint().address().to_v4().to_bytes());
        if (sk_info.is_valid()) {  // use proxy
          tcp_conn_ptr->start(sk_info.host, sk_info.port, true);
        } else {
          tcp_conn_ptr->start(conn_infos[i].host, conn_infos[i].port, false);
        }
      }
    }

    io_context.run();
  } catch (std::exception& e) {
    // std::cerr << "Exception: " << e.what() << "\n";
  }
  return 0;
}
