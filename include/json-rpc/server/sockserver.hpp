#ifndef __JSONRPC_SOCKSERVER_HPP__
#define __JSONRPC_SOCKSERVER_HPP__

#include "json-rpc/util.hpp"
#include "json-rpc/server/sconn.hpp"
#include "json-rpc/server/request.hpp"
#include "json-rpc/server/asio.hpp"
#include "json-rpc/errors.hpp"
#include "common/all.hpp"

#include <list>


class Connection;
class ConnectionThread;

class SockServer : public ServerConnector {
  public:
    SockServer(std::string port);

    int start();
    int stop();

    void loop();

    virtual ~SockServer();

  private:

    void _close_connections();

    int _pool_size;
    bool _stop;

    Mutex _mutex;

    class ServerLoopThread : public Thread {
      public:
        ServerLoopThread(SockServer* pserver) :Thread(), _pserver(pserver) {
        }
        void run() {
          _pserver->loop();
        };

      private:
        SockServer* _pserver;
    };

    friend class ServerLoopThread;
    friend class ConnectionThread;
    ServerLoopThread _thread;

    std::list<Connection*> _connections;

  CLASS_MAKE_LOGGER
};



class ConnectionThread : public Thread {
  public:
    ConnectionThread(SockServer* server, Connection* pconn)
        : _pserver(server), _pconn(pconn) {
#ifdef JSONRPC_DEBUG
      CLASS_INIT_LOGGER("ConnectionThread", L_DEBUG);
#endif
    }

    void run();

  private:
    SockServer* _pserver;
    Connection* _pconn;

  CLASS_MAKE_LOGGER
};

class Connection {
  friend class ConnectionThread;
  public:
    Connection(SockServer* pserver, int sock)
        :_pserver(pserver), _client_sock(sock),
         _connected(true), _thread(pserver, this) {
#ifdef JSONRPC_DEBUG
      CLASS_INIT_LOGGER("Connection", L_DEBUG);
#endif
    }

    void start() {
      CLOG_DEBUG("Thread start\n");
      _thread.start();
    } 

    void shutdown() {
      _mutex.lock();
      _connected = false;
      ::shutdown(_client_sock, SHUT_RD);
      _mutex.unlock();

      if (_thread.is_active()){
        _thread.join();
      }
      CLOG_DEBUG("thread joined\n");
      close(_client_sock);
    }

    ~Connection() { shutdown(); }

  private:
    SockServer* _pserver;
    int _client_sock;
    bool _connected;

    Mutex _mutex;

    ConnectionThread _thread;

    std::string _recv();
    void _send(std::string msg);

    Request _build_request(size_t& handler_id);

    void _send_response(Response& resp);
    void _send_error(int code);
    

  CLASS_MAKE_LOGGER
};

#endif
