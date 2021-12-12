#pragma once

#include <array>
#include <string>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace socks4 {

const unsigned char version = 0x04;

class request{
 public:
  enum command_type{
    connect = 0x01,
    bind = 0x02
  };

  request(unsigned char command, unsigned short port, std::array<unsigned char, 4> address)
    : version_(version),
      command_(command),
      port_high_byte_((port >> 8) & 0xff),
      port_low_byte_(port & 0xff),
      address_(address),
      user_id_(),
      null_byte_(0) {
  }

  request(unsigned char* sock4_data, size_t length) {
    version_ = sock4_data[0];
    command_ = sock4_data[1];
    port_high_byte_ = sock4_data[2];
    port_low_byte_ = sock4_data[3];
    for (size_t i=4; i <= 7; i++) {
      address_[i-4] = sock4_data[i];
    }
    size_t t = 8;
    while (sock4_data[t] != 0) {
      user_id_ += static_cast<char>(sock4_data[t]);
      t++;
    }
    if (t != length - 1) {
      t++;
      while (sock4_data[t] != 0) {
        url_ += static_cast<char>(sock4_data[t]);
        t++;
      }
    }
  }

  std::array<boost::asio::const_buffer, 7> buffers() const {
    return{
      {
        boost::asio::buffer(&version_, 1),
        boost::asio::buffer(&command_, 1),
        boost::asio::buffer(&port_high_byte_, 1),
        boost::asio::buffer(&port_low_byte_, 1),
        boost::asio::buffer(address_),
        boost::asio::buffer(user_id_),
        boost::asio::buffer(&null_byte_, 1)
      }
    };
  }

  std::string getDestPort() {
    return std::to_string((static_cast<unsigned short> (port_high_byte_) << 8 ) | port_low_byte_);
  }

  std::string getDestHost() {
    if (check_address_valid()) {
      return boost::asio::ip::address_v4(address_).to_string();
    } else {
      return url_;
    }
  }

  unsigned char getcommand() {
    return command_;
  }

  std::string getcommandstr() {
    if (command_ == command_type::connect) return "CONNECT";
    return "BIND";
  }

  std::string getcommandchar() {
    if (command_ == command_type::connect) return "c";
    return "b";
  }

  bool check_address_valid() {
    return !(address_[0] == 0 && address_[1] == 0 && address_[2] == 0);
  }


 private:
  unsigned char version_;
  unsigned char command_;
  unsigned char port_high_byte_;
  unsigned char port_low_byte_;
  boost::asio::ip::address_v4::bytes_type address_;
  std::string user_id_;
  std::string url_;
  unsigned char null_byte_;
};

class reply{
 public:
  enum status_type{
    request_granted = 0x5a,
    request_failed = 0x5b,
    request_failed_no_identd = 0x5c,
    request_failed_bad_user_id = 0x5d
  };

  reply() {}

  reply(unsigned char status)
    : null_byte_(0),
      status_(status),
      port_high_byte_(0),
      port_low_byte_(0),
      address_(std::array<unsigned char, 4> {0}) {
      }

  reply(unsigned char status, unsigned short port, std::array<unsigned char, 4> address)
    : null_byte_(0),
      status_(status),
      port_high_byte_((port >> 8) & 0xff),
      port_low_byte_(port & 0xff),
      address_(address) {
      }

  std::array<boost::asio::mutable_buffer, 5> buffers() {
    return{
      {
        boost::asio::buffer(&null_byte_, 1),
        boost::asio::buffer(&status_, 1),
        boost::asio::buffer(&port_high_byte_, 1),
        boost::asio::buffer(&port_low_byte_, 1),
        boost::asio::buffer(address_)
      }
    };
  }

  bool success() const {
    return null_byte_ == 0 && status_ == request_granted;
  }

 private:
  unsigned char null_byte_;
  unsigned char status_;
  unsigned char port_high_byte_;
  unsigned char port_low_byte_;
  boost::asio::ip::address_v4::bytes_type address_;
};

}  // namespace socks4
