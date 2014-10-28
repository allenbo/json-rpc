#include "json-rpc/util.hpp"
#include "json-rpc/buffer.hpp"
#include "json-rpc/errors.hpp"

#include "json-rpc/sockclient.hpp"

SockClient::SockClient(std::string host, std::string port)
    :_sock(UNINIT_SOCKET), _server_info(nullptr) {
#ifdef JSONRPC_DEBUG
  CLASS_INIT_LOGGER("SockClient", L_DEBUG)
#endif
  _host = host;
  _port = port;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  int r = getaddrinfo(host.c_str(), port.c_str(), &hints, &_server_info);
  if (r != 0) {
    CLOG_DEBUG("getaddrinfo error!\n");
    throw HostFailException("Function getaddrinfo fail", _host, _port);
  }
}

void SockClient::reconnect() {
  if (_sock != UNINIT_SOCKET) {
    close(_sock);
  }
  struct addrinfo * ptr = nullptr;
  _sock = UNINIT_SOCKET;
  for(ptr = _server_info; ptr != nullptr; ptr = ptr->ai_next) {
    _sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (_sock == UNINIT_SOCKET) {
      continue;
    }
    if (connect(_sock, ptr->ai_addr, (int)ptr->ai_addrlen) == 0) {
      CLOG_DEBUG("connect to server\n");
      break;
    }
    close(_sock);
  }

  if (ptr == nullptr) {
    CLOG_DEBUG("Can't connect to server\n");
    throw HostFailException("Can't connect", _host, _port);
  }
}

void SockClient::send_and_response(std::string message, std::string &result) {
  if (_sock == UNINIT_SOCKET) {
    reconnect();
  }
  
  _send(message);
  CLOG_DEBUG("send message %s to server\n", message.c_str());
  _recv(result);
  CLOG_DEBUG("get result %s\n", result.c_str());
  return;
}

void SockClient::_send(const std::string& msg) {
  const int MAX_CHUNK_SIZE = 1024;
  char chunk[MAX_CHUNK_SIZE];

  RDONBuffer rd_buffer(msg.c_str(), msg.size());

  while( true ) {
    int len = rd_buffer.read(chunk, MAX_CHUNK_SIZE);
    if (len != 0) {
      if (write(_sock, chunk, len) != len) {
        CLOG_FATAL("write function error\n");
      }
    } else {
      break;
    }
  }
}

void SockClient::_recv(std::string& result) {
  const int MAX_CHUNK_SIZE = 1024;
  char chunk[MAX_CHUNK_SIZE];

  WRONBuffer wr_buffer;

  while( true ) {
    int chunk_len = read(_sock, chunk, MAX_CHUNK_SIZE);
    if (chunk_len < 0) {
      CLOG_FATAL("read function error %d\n", chunk_len);
    }

    wr_buffer.write(chunk, chunk_len);

    if(chunk_len != MAX_CHUNK_SIZE)  {
      result.assign(wr_buffer.c_str());
      break;
    }
  }
}

SockClient::~SockClient() {
  freeaddrinfo(_server_info);
  if (_sock != UNINIT_SOCKET) {
    close(_sock);
  }
}
