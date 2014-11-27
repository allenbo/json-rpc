#include "json-rpc/util.hpp"
#include "json-rpc/buffer.hpp"
#include "json-rpc/errors.hpp"

#include "json-rpc/client/sockclient.hpp"

SockClient::SockClient(std::string host, std::string port)
    :_sock(UNINIT_SOCKET), _server_info(nullptr) {
  _host = host;
  _port = port;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  int r = getaddrinfo(host.c_str(), port.c_str(), &hints, &_server_info);
  if (r != 0) {
    LOG(DEBUG) << "getaddrinfo error!" << std::endl;
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
      LOG(DEBUG) << "connect to server" << std::endl;
      break;
    }
    close(_sock);
  }

  if (ptr == nullptr) {
    LOG(DEBUG) << "Can't connect to server" << std::endl;
    throw HostFailException("Can't connect", _host, _port);
  }
}

void SockClient::send_and_response(std::string message, std::string &result) {
  if (_sock == UNINIT_SOCKET) {
    reconnect();
  }
  
  _send(message);
  LOG(DEBUG) << "send message " << message << " to server" << std::endl;
  _recv(result);
  LOG(DEBUG) << "get result " <<  result << std::endl;
  return;
}

void SockClient::_send(const std::string& msg) {
  const int MAX_CHUNK_SIZE = 1024;
  char chunk[MAX_CHUNK_SIZE];

  SizedRDONBuffer rd_buffer(msg.c_str(), msg.size());
  LOG(DEBUG) << "Send out message with " << msg.size() << " bytes" << std::endl;

  while( true ) {
    int len = rd_buffer.read(chunk, MAX_CHUNK_SIZE);
    if (len != 0) {
      if (write(_sock, chunk, len) != len) {
        LOG(INFO) << "write function error" << std::endl;
        throw WriteFailException();
      }
    } else {
      break;
    }
  }
}

void SockClient::_recv(std::string& result) {
  const int MAX_CHUNK_SIZE = 1024;
  char chunk[MAX_CHUNK_SIZE];

  int size = 0;
  int curr_size = 0;
  int len = read(_sock, &size, sizeof(int));

  if (len != sizeof(int) || size <= 0){
    LOG(INFO) << "Read " << len << " bytes of size of message" << std::endl;
    throw ReadFailException();
  }

  LOG(DEBUG) << "The size of the message is " << size << std::endl;

  WRONBuffer wr_buffer;

  while( true ) {
    int chunk_len = read(_sock, chunk, MAX_CHUNK_SIZE);
    if (chunk_len < 0) {
      LOG(INFO) <<  "read function error " << chunk_len << std::endl;
      throw ReadFailException();
    }

    wr_buffer.write(chunk, chunk_len);

    curr_size += chunk_len;
    if(curr_size == size)  {
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
