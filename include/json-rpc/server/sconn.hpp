#ifndef __JSONRPC_SCONN_HPP__
#define __JSONRPC_SCONN_HPP__

#include "json-rpc/server/asio.hpp"
#include "json-rpc/errors.hpp"
#include "json-rpc/util.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

static const int MAX_QUEUE_SIZE = 100;

class ServerConnector {
  public:
    ServerConnector(std::string port)
        : _handler(nullptr), _host_info(nullptr), _sock(UNINIT_SOCKET){
      struct addrinfo hints;
      memset(&hints, 0, sizeof(struct addrinfo));
      hints.ai_family = AF_INET;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_flags = AI_PASSIVE;

      int r = getaddrinfo(nullptr, port.c_str(), &hints, &_host_info);
      if (r != 0) {
        throw HostFailException("Function getaddrinfo error", "localhost", port);
      }
    }

    virtual ~ServerConnector() {
      freeaddrinfo(_host_info);
    }

    virtual int start() = 0;
    virtual int stop() = 0;

    int register_service(ASIO* handler) {
      _handler = handler;
      return 0;
    }
  
  protected:
    ASIO* _handler;
    struct addrinfo* _host_info;
    int _sock;

    int _listen() {
      if (_sock != UNINIT_SOCKET)  {
        return -1;
      }

      _sock = ::socket(_host_info->ai_family, _host_info->ai_socktype, _host_info->ai_protocol);
      if (_sock == UNINIT_SOCKET) {
        throw SocketFailException("socket");
      }

      int optval = 1;
      setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

      int r = bind(_sock, _host_info->ai_addr, _host_info->ai_addrlen);
      if (r != 0) {
        throw SocketFailException("bind");
      }

      r = ::listen(_sock, MAX_QUEUE_SIZE);
      if (r != 0) {
        throw SocketFailException("listen");
      }
      return 0;
    }

};

#endif
