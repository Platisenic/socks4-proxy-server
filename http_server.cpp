#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class http_connection : public std::enable_shared_from_this<http_connection>{
 public:
  explicit http_connection(tcp::socket socket):
    socket_(std::move(socket)) {}

  void start() {
    read_request();
  }

 private:
  void read_request() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, MaxLengthk),
        [this, self](boost::system::error_code ec, std::size_t length){
          if (!ec){
            process_request(length);
          }
        });
  }

  void process_request(std::size_t length) {
    auto self(shared_from_this());
    std::string data(data_, data_+length);
    // std::cout << data;
    std::stringstream ss(data);
    std::string line_str;
    while (std::getline(ss, line_str)) {
      parsed_data_.push_back(line_str);
    }
    ss.str("");
    ss.clear();
    ss << parsed_data_.front();
    ss >> request_method >> target >> http_version;
    std::size_t foundq = target.find("?");
    if (foundq == std::string::npos) {
      request_uri = target;
      exec_cgi = "." + target;
      query_string = "";
    } else {
      request_uri = target;
      exec_cgi = "." + target.substr(0, foundq);
      query_string = target.substr(foundq+1);
    }
    ss.str("");
    ss.clear();
    ss << parsed_data_.at(1);
    ss >> un_used >> http_host;
    server_addr = socket_.local_endpoint().address().to_string();
    server_port = std::to_string(socket_.local_endpoint().port());
    remote_addr = socket_.remote_endpoint().address().to_string();
    remote_port = std::to_string(socket_.remote_endpoint().port());

    write_response();
  }

  void write_response() {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(success_status, strlen(success_status)),
        [this, self](boost::system::error_code ec, std::size_t /*length*/){
          if (!ec){
            while ((pid = fork()) < 0) { usleep(1000);}
            if (pid == 0) {  // child
              setenv("REQUEST_METHOD", request_method.c_str(), 1);
              setenv("REQUEST_URI", request_uri.c_str(), 1);
              setenv("QUERY_STRING", query_string.c_str(), 1);
              setenv("SERVER_PROTOCOL", http_version.c_str(), 1);
              setenv("HTTP_HOST", http_host.c_str(), 1);
              setenv("SERVER_ADDR", server_addr.c_str(), 1);
              setenv("SERVER_PORT", server_port.c_str(), 1);
              setenv("REMOTE_ADDR", remote_addr.c_str(), 1);
              setenv("REMOTE_PORT", remote_port.c_str(), 1);

              dup2(socket_.native_handle(), STDIN_FILENO);
              dup2(socket_.native_handle(), STDOUT_FILENO);
              dup2(socket_.native_handle(), STDERR_FILENO);
              socket_.close();

              int ret = execlp(exec_cgi.c_str(), exec_cgi.c_str(), NULL);
              if (ret < 0) {}  // TODO(libos) Handle Error
            } else {  // parent
              socket_.close();
            }
          }
        });
  }
  pid_t pid;
  tcp::socket socket_;
  enum { MaxLengthk = 1024 };
  char data_[MaxLengthk];
  std::vector<std::string> parsed_data_;
  std::string request_method;
  std::string request_uri;
  std::string query_string;
  std::string http_host;
  std::string server_addr;
  std::string server_port;
  std::string remote_addr;
  std::string remote_port;
  std::string exec_cgi;
  std::string target;
  std::string http_version;
  std::string un_used;
  char success_status[200] = "HTTP/1.1 200 OK\r\n";
};

class http_server{
 public:
  http_server(boost::asio::io_context& io_context, unsigned short port) :
    acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
    signal_(io_context, SIGCHLD) {
      wait_for_signal();
      do_accept();
  }

 private:
  void wait_for_signal() {
    signal_.async_wait(
        [this](boost::system::error_code /*ec*/, int /*signo*/){
            int status = 0;
            while (waitpid(-1, &status, WNOHANG) > 0) {}
            wait_for_signal();
        });
  }

  void do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket){
          if (!ec) {
            std::make_shared<http_connection>(std::move(socket))->start();
          }
          do_accept();
        });
  }

  tcp::acceptor acceptor_;
  boost::asio::signal_set signal_;
};

int main(int argc, char* argv[]) {
  try {
    if (argc != 2) {
      std::cerr << "Usage: http_server <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;
    http_server s(io_context, std::atoi(argv[1]));
    io_context.run();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
