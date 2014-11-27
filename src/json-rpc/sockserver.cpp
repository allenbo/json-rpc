#include "json-rpc/server/sockserver.hpp"
#include "json-rpc/buffer.hpp"

#include "json-rpc/proto.hpp"

static const int POOL_SIZE = 16;

SockServer::SockServer(std::string port)
    : ServerConnector(port),
      _pool_size(POOL_SIZE), _stop(false),  _thread(this) {
  _connections.clear();
}

SockServer::~SockServer() {
  _close_connections(); 
  _stop = true;

  if (_thread.is_active()) {
    _thread.join();
  }
}

/* Close all connections
 */
void SockServer::_close_connections() {
  _mutex.lock();
  auto it = _connections.begin();
  for(; it != _connections.end(); it ++) {
    (*it)->shutdown();
  }
  _connections.clear();
  _mutex.unlock();
}

int SockServer::start() {
  _listen();
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

    LOG(DEBUG) << "New connection" << std::endl;
    if (_connections.size() >= _pool_size) {
      LOG(DEBUG) << "Remove oldest connection" << std::endl;
      auto oldconn = _connections.front();
      oldconn->shutdown();
      LOG(DEBUG) << "Connection shutdown" << std::endl;
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
    try {
      std::string msg = _pconn->_recv();

      if (msg == "") {
        throw ServerBadMessageException();
      }
      Request request = Proto::build_request(msg);
      int rst = _pserver->_handler->on_request(&request);
      if (rst == 0) {
        throw ServerMethodNotFoundException();
      }

      _pconn->_send_response(request.get_response());
      if (!_pconn->_connected)
        throw ServerCloseSocketException();

    } catch(ServerException& e) {
      LOG(DEBUG) << e.what() << std::endl;
      _pconn->_mutex.lock();
      if (!_pconn->_connected || e.get_code() != Proto::SOCKET_CLOSED) {
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

  int size = 0;
  int curr_size = 0;
  int len = read(_client_sock, (void*)&size, sizeof(int));

  if (len == 0) {
    LOG(DEBUG) << "connection closed" << std::endl;
    throw ServerCloseSocketException();
  }

  if (len != sizeof(int) || size <= 0) {
    LOG(INFO) << "Error when read size of incoming message" << std::endl;
    return "";
  }

  LOG(DEBUG) << "The size of the message is " << size << std::endl;

  while(true) {
    int chunk_len = read(_client_sock, chunk, MAX_BUFF_SIZE);

    wr_buffer.write(chunk, chunk_len);
    curr_size += chunk_len;

    if (curr_size == size) {
      break;
    }
  }

  std::string msg(wr_buffer.c_str(), wr_buffer.size());
  return msg;
}


void Connection::_send(std::string msg) {
  const int MAX_CHUNK_SIZE = 1024;
  char chunk[MAX_CHUNK_SIZE];

  SizedRDONBuffer rd_buffer(msg.c_str(), msg.size());
  LOG(DEBUG) << "Send out message with " << msg.size() << " bytes" << std::endl;

  while( true ) {
    int len = rd_buffer.read(chunk, MAX_CHUNK_SIZE);
    if (len != 0) {
      int real_len = write(_client_sock, chunk, len);
      if (real_len == 0) {
        throw ServerCloseSocketException();
      }

      if (real_len != len) {
        LOG(FATAL) << "write function error" << std::endl;
      }
    } else {
      break;
    }
  }
}

void Connection::_send_response(Response& resp) {
  std::string jsonText = Proto::build_response(resp);
  _send(jsonText); 
}

void Connection::_send_error(int code) {
  std::string jsonText = Proto::build_error(code);
  _send(jsonText); 
}
