#ifndef __JSONRPC_CCONN_HPP__
#define __JSONRPC_CCONN_HPP__

#include "jconer/json.hpp"

/**
 * Base client connector for sending and receiving data
 */
class ClientConnector {
  public:
    ClientConnector() {}
    virtual void send_and_response(std::string value, std::string& result) = 0;
    virtual void reconnect() = 0;
};

#endif
