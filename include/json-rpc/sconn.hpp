#ifndef __JSONRPC_SCONN_HPP__
#define __JSONRPC_SCONN_HPP__

#include "json-rpc/asio.hpp"

class ServerConnector {
  public:
    ServerConnector(): _handler(nullptr) {}

    virtual int listen() = 0;
    virtual int stop() = 0;

    int register_service(ASIO* handler) {
      _handler = handler;
      return 0;
    }
  
  protected:
    ASIO* _handler;
};

#endif
