#ifndef __JSONRPC_SOCKCLIENT_HPP__
#define __JSONRPC_SOCKCLIENT_HPP__

#include "json-rpc/client/cconn.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
 
/**
 * A tpc socket client connector
 */
class SockClient : public ClientConnector {
  public:
    SockClient(std::string host, std::string port);
    ~SockClient();

    void send_and_response(std::string value, std::string& result);
    void reconnect();
  private:
  
    void _send(const std::string &msg);
    void _recv(std::string &result);

    int _sock;
    std::string _host;
    std::string _port;
    struct addrinfo* _server_info;
  CLASS_MAKE_LOGGER
};

#endif
