#ifndef __JSONRPC_POLLSERVER_HPP__
#define __JSONRPC_POLLSERVER_HPP__

#include "json-rpc/util.hpp"
#include "json-rpc/buffer.hpp"
#include "json-rpc/server/sconn.hpp"
#include "json-rpc/server/request.hpp"
#include "json-rpc/server/asio.hpp"
#include "json-rpc/errors.hpp"
#include "common/all.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <list>
#include <unordered_set>

enum class FD_MODE {
  READ = 0,
  WRITE = 1
};

class PollManager {
  CLASS_NOCOPY(PollManager)
  public:
    static PollManager& get_instance() { return _instance;}
    virtual ~PollManager();

    void watch(int fd, FD_MODE mod);
    void unwatch(int fd, FD_MODE mod);

    bool poll(std::vector<int>& read_fds, std::vector<int>& write_fds);
  private:
    int _highest;
    fd_set _read_set;
    fd_set _write_set;
    std::unordered_set<int> _watched_read_fds;
    std::unordered_set<int> _watched_write_fds;

    Mutex _mutex;

    static PollManager _instance;
    PollManager();
};


class Channel;

class PollServer : public ServerConnector {
  public:
    PollServer(std::string port);

    int start();
    int stop();

    void loop();

    virtual ~PollServer();

  private:
    ThreadPool _thread_pool;
    bool _stop;
    std::map<int, Channel*> _channels;

    class ServerLoopThread : public Thread {
      public:
        ServerLoopThread(PollServer* pserver):Thread(), _pserver(pserver) {
        }

        void run() {
          _pserver->loop();
        }
      private:
        PollServer* _pserver;
    };

    friend class ServerLoopThread;
    ServerLoopThread _thread;

    void _close_channels();
    void _add_job_wrapper(Channel* chan);
    void _handle_request(Channel*);
};

class Channel {
  public:
    enum State {
      BROKEN = -2,        // when message is broken
      CLOSED = -1,        // when client closes socket
      READ_PENDING = 0,   // when channel is readding
      READ_READY = 1,     // when channel finishes reading
      WRITE_PENDING = 3,  // when  channel is writing
      WRITE_READY = 4,    // when chanell finishes writing
    };

    Channel(int sock, PollServer* server);
    virtual ~Channel();

    State read();   // read callback
    State write();  // write callback

    void send(std::string msg);
    void send(const char* msg, size_t len);

    void close();

    bool is_alive();

    std::string get_msg(); // result the message channel got from client
    void clear_msg(); // clear write only buffer and send a read notification

  private:
    int _sock;

    SizedRDONBuffer *_read_buffer;
    SizedWRONBuffer *_write_buffer;

    Mutex _send_mutex;
    Mutex _read_mutex;
    Condition _read_cond;
    bool _alive;

    /* When channel finishes reading one message, it has to handle the message.
     * Now set busy to be true and blocking any other read callback
     */
    bool _busy;
};

#endif
