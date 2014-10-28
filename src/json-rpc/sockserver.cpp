#include "json-rpc/sockserver.hpp"
#include "json-rpc/buffer.hpp"

#include "json-rpc/proto.hpp"

static const int MAX_QUEUE_SIZE = 128;
static const int POOL_SIZE = 16;

SockServer::SockServer(std::string port)
    : ServerConnector(), _host_info(nullptr), _sock(UNINIT_SOCKET) ,
      _stop(false) , _thread(this), _pool_size(POOL_SIZE) {
#ifdef JSONRPC_DEBUG
  CLASS_INIT_LOGGER("SockServer", L_DEBUG)
#endif

  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  int r = getaddrinfo(nullptr, port.c_str(), &hints, &_host_info);
  if (r != 0) {
    CLOG_DEBUG("getaddrinfo function error!\n");
    throw HostFailException("Function getaddrinfo error", "localhost", port);
  }
  _connections.clear();
}

SockServer::~SockServer() {
  freeaddrinfo(_host_info);
  _close_connections(); 
  _stop = true;

  if (_thread.is_active()) {
    _thread.join();
  }
}

void SockServer::_close_connections() {
  _mutex.lock();
  auto it = _connections.begin();
  for(; it != _connections.end(); it ++) {
    (*it)->shutdown();
  }
  _connections.clear();
  _mutex.unlock();
}

int SockServer::listen() {
  if (_sock != UNINIT_SOCKET)  {
    return -1;
  }

  _sock = ::socket(_host_info->ai_family, _host_info->ai_socktype, _host_info->ai_protocol);
  if (_sock == UNINIT_SOCKET) {
    CLOG_DEBUG("socket function error!\n");
    throw SocketFailException("socket");
  }

  int optval = 1;
  setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

  int r = bind(_sock, _host_info->ai_addr, _host_info->ai_addrlen);
  if (r != 0) {
    CLOG_DEBUG("bind function error!\n");
    throw SocketFailException("bind");
  }

  r = ::listen(_sock, MAX_QUEUE_SIZE);
  if (r != 0) {
    CLOG_DEBUG("listen function error!\n");
    throw SocketFailException("listen");
  }

  _thread.start();
  return 0;
}

int SockServer::stop() {
  _close_connections();
  _stop = true;

  if (_thread.is_active()) {
    _thread.join();
  }
  return 0;
}

void SockServer::loop() {
  while (!_stop) {
    struct sockaddr_in client_info;
    socklen_t client_len = sizeof(struct sockaddr_in);
    int client_sock = accept(_sock, (struct sockaddr*) &client_info, &client_len);

    CLOG_DEBUG("New connection\n");
    if (_connections.size() >= _pool_size) {
      CLOG_DEBUG("Remove oldest connection\n");
      auto oldconn = _connections.front();
      oldconn->shutdown();
      CLOG_DEBUG("Connection shutdown\n");
      _connections.pop_front();
      delete oldconn;
    }
    Connection* newconn = new Connection(this, client_sock);

    _mutex.lock();
    _connections.push_back(newconn);
    _mutex.unlock();

    newconn->start();
  }
}


void ConnectionThread::run() {
  while(_pconn->_connected) {
    size_t handler_id;
    try {
      Request request = _pconn->_build_request(handler_id);
      int rst = _pserver->_handler->on_request(handler_id, &request);
      if (rst == 0) {
        throw ServerMethodNotFoundException();
      }

      _pconn->_send_response(request.get_response());
      if (!_pconn->_connected)
        throw ServerCloseSocketException();

    } catch(ServerException& e) {
      CLOG_DEBUG("%s\n",e.what());
      _pconn->_mutex.lock();
      if (!_pconn->_connected || e.get_code() != SOCKET_CLOSED) {
        // server close connection
        _pconn->_send_error(e.get_code());
      } else {
        _pconn->_connected = false;
      }
      _pconn->_mutex.unlock();
      break;
    }
  }
}

std::string Connection::_recv() {
  WRONBuffer wr_buffer;
  const int MAX_BUFF_SIZE = 1024;
  char chunk[MAX_BUFF_SIZE];

  while( true) {
    int chunk_len = read(_client_sock, chunk, MAX_BUFF_SIZE);

    if (chunk_len == 0) {
      CLOG_DEBUG("connection closed\n");
      throw ServerCloseSocketException();
    }

    wr_buffer.write(chunk, chunk_len);

    if (chunk_len != MAX_BUFF_SIZE) {
      break;
    }
  }

  std::string msg(wr_buffer.c_str(), wr_buffer.size());
  return msg;
}


void Connection::_send(std::string msg) {
  const int MAX_CHUNK_SIZE = 1024;
  char chunk[MAX_CHUNK_SIZE];

  RDONBuffer rd_buffer(msg.c_str(), msg.size());

  while( true ) {
    int len = rd_buffer.read(chunk, MAX_CHUNK_SIZE);
    if (len != 0) {
      int real_len = write(_client_sock, chunk, len);
      if (real_len == 0) {
        throw ServerCloseSocketException();
      }

      if (real_len != len) {
        CLOG_FATAL("write function error\n");
      }
    } else {
      break;
    }
  }
}


Request Connection::_build_request(size_t& handler_id) {
  std::string msg = _recv();
  CLOG_DEBUG("build request for client_sock %d, %s\n", _client_sock, msg.c_str());

  PError err;
  JValue* req_json = loads(msg, err);

  if (req_json == nullptr || !req_json->isObject()) {
    CLOG_DEBUG("Json parsing error\n");
    throw ServerJsonNotParsedException();
  }

  JValue* item_json = req_json->get(METHOD);
  if (item_json == nullptr || !item_json->isInteger()) {
    CLOG_DEBUG("Can't get method\n");
    throw ServerJsonNotParsedException();
  }
  handler_id = item_json->getInteger();


  item_json = req_json->get(PARAM);
  if (item_json == nullptr || !item_json->isArray()) {
    CLOG_DEBUG("Can't get parameters\n");
    throw ServerJsonNotParsedException();
  }

  Request request(item_json);
  return request;
}

void Connection::_send_response(Response& resp) {
  JObject* obj = new JObject();
  obj->put(RESULT, resp.get_serializer().getContent());

  std::string jsonText = dumps(obj);
  CLOG_DEBUG("send_response %s\n", jsonText.c_str());
  
  _send(jsonText); 
}

void Connection::_send_error(int code) {
  JObject* obj = new JObject();
  obj->put(ERROR, code);

  std::string jsonText = dumps(obj);
  CLOG_DEBUG("send_response %s\n", jsonText.c_str());
  
  _send(jsonText); 
}
