# Socks4 Proxy Server

Implement the SOCKS 4/4A protocol in the application layer of the OSI model using Boost.Asio library.  

SOCKS is similar to a proxy that acts as both server and client for the purposeof making requests on behalf of other clients.  Because the SOCKS protocol is independent of application protocols, it can be used for many different services:  telnet, ftp, WWW, etc. There are two types of the SOCKS operations, namely CONNECT and BIND. Both of them are covered in this project. 

## Prerequisite

- Install Boost library

```shell
sudo apt-get install libboost-all-dev
```

## How to use

1. Build from source

```shell
cd socks4-proxy-server
make
```

2. Start the SOCKS Server

```shell
./socks_server [port]
```

3. Modify setting in your browser (FireFox)
<img alt="image" src="https://user-images.githubusercontent.com/69299037/192149746-3cb413dc-a174-45f0-b7aa-89c989adfa53.png">

## Implementation detail

### SOCKS4 Server

1. After the SOCKS server starts listening, if a SOCKS client connects, use fork() to tackle with it.
2. Receive SOCKS4_REQUEST from the SOCKS client.
3. Get the destination IP and port from SOCKS4_REQUEST.
4. Check the firewall (socks.conf), and send SOCKS4_REPLY to the SOCKS client if rejected.
5. Check CD value and choose one of the operations.
   - CONNECT (CD=1)
     1. Connect to the destination.
     2. Send SOCKS4_REPLY to the SOCKS client.
     3. Start relaying traffic on both directions.
   - BIND (CD=2)
     1. Bind and listen a port.
     2. Send SOCKS4_REPLY to SOCKS client to tell which port to connect.
     3. Accept connection from destination and send another SOCKS4_REPLY to SOCKS client.
     4. Start relaying traffic on both directions.

More details refered to [SOCKS4](https://www.openssh.com/txt/socks4.protocol) [SOCKS4A](https://www.openssh.com/txt/socks4a.protocol)
