#include "json-rpc/server/pollserver.hpp"
#include "json-rpc/util.hpp"
#include <vector>

static const int MAX_BUFF_SIZE = 1024;

PollManager::PollManager(): _highest(0) {
  _watched_read_fds.clear();
  _watched_write_fds.clear();
#ifdef JSONRPC_DEBUG
  CLASS_INIT_LOGGER("PollManager", L_DEBUG)
#endif
}

PollManager::~PollManager() {
}

PollManager PollManager::_instance;

void PollManager::watch(int fd, FD_MODE mod) {
  ScopeLock _(&_mutex);
  nonblock_fd(fd);
  if (mod == FD_MODE::READ) {
    FD_SET(fd, &_read_set);
    _watched_read_fds.insert(fd);
  } else {
    FD_SET(fd, &_write_set);
    _watched_write_fds.insert(fd);
  }

  CLOG_DEBUG("watch fd %d as %d\n", fd, mod);

  if (fd > _highest) {
    _highest = fd;
  }
}

void PollManager::unwatch(int fd, FD_MODE mod) {
  ScopeLock _(&_mutex);
  if (mod == FD_MODE::READ) {
    FD_CLR(fd, &_read_set);
    _watched_read_fds.erase(fd);
  } else  {
    FD_CLR(fd, &_write_set);
    _watched_write_fds.erase(fd);
  }

  CLOG_DEBUG("unwatch fd %d as %d\n", fd, mod);

  int new_highest = 0;
  auto it = _watched_read_fds.begin();
  for(; it != _watched_read_fds.end(); it ++) {
    if (new_highest < *it) {
      new_highest = *it;
    }
  }

  it = _watched_write_fds.begin();
  for(; it != _watched_write_fds.end(); it ++) {
    if (new_highest < *it) {
      new_highest = *it;
    }
  }

  CLOG_DEBUG("new highest fd is %d\n", new_highest);

  _highest = new_highest;
}

bool PollManager::poll(std::vector<int>& reads, std::vector<int>& writes) {
  CLOG_DEBUG("seleting\n");
  while (true) {
    fd_set read_set;
    fd_set write_set;
    int highest;
    {
      ScopeLock _(&_mutex);
      read_set = _read_set;
      write_set = _write_set;
      highest = _highest;
    }

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000; // 1ms
    int retval = select(highest + 1, &read_set, &write_set, nullptr, &timeout);

    if (retval == -1) return false;

    for(int i = 0; i <= highest; i ++) {
      if (FD_ISSET(i, &read_set)) {
        reads.push_back(i);
      }

      if (FD_ISSET(i, &write_set)) {
        writes.push_back(i);
      }
    }

    if (reads.size() != 0 || writes.size() != 0) {
      break;
    }
  }
  return true;
}

Channel::Channel(int sock, PollServer* server)
    :_sock(sock),
     _read_buffer(nullptr), _write_buffer(nullptr),
     _send_mutex(), _read_mutex(), _read_cond(&_read_mutex),
     _alive(true), _busy(false) {
  nonblock_fd(_sock);
#ifdef JSONRPC_DEBUG
  CLASS_INIT_LOGGER("Channel", L_DEBUG)
#endif
}

Channel::~Channel() {
  if (is_alive()) {
    close();
    _alive = false;
  }

  if (_read_buffer) delete _read_buffer;
  if (_write_buffer) delete _write_buffer;
}

Channel::State Channel::read() {
  CLOG_DEBUG("enter read function in channel\n");
  char chunk[MAX_BUFF_SIZE];
  int len;
  len = ::read(_sock, chunk, MAX_BUFF_SIZE);
  CLOG_DEBUG("read %d bytes\n", len);

  if (len == 0) {
    return State::CLOSED;
  } else if (len < 0) {
    CLOG_FATAL("read function error\n");
  }

  while (_busy) {
    CLOG_DEBUG("Busy, waiting for operatio to finish\n");
    _read_cond.wait();
  }

  if (_write_buffer == nullptr) {
    _write_buffer = new WRONBuffer();
  }

  _write_buffer->write(chunk, len);

  if (len == MAX_BUFF_SIZE) {
    return Channel::State::READ_PENDING;
  } else {
    _busy = true;
    CLOG_DEBUG("Set to busy\n");
    return Channel::State::READ_READY;
  }
}

Channel::State Channel::write() {
  CLOG_DEBUG("enter write function in channel\n");
  char chunk[MAX_BUFF_SIZE];
  int len;

  if (_read_buffer == nullptr) return State::WRITE_PENDING;
  len = _read_buffer->read(chunk, MAX_BUFF_SIZE);
  chunk[len] = '\0';

  if (len == 0) {
    _send_mutex.unlock();
    return State::WRITE_READY;
  }

  CLOG_DEBUG("write %s\n", chunk);
  ::write(_sock, chunk, len);

  if (len != MAX_BUFF_SIZE) {
    _send_mutex.unlock();
    return State::WRITE_READY;
  }

  return State::WRITE_PENDING;
}

void Channel::send(std::string msg) {
  send(msg.c_str(), msg.size());
}

void Channel::send(const char* msg, size_t len) {
  _send_mutex.lock();
  CLOG_DEBUG("get send lock\n");
  if (_read_buffer != nullptr) {
    delete _read_buffer;
  }

  _read_buffer = new HRDONBuffer(msg, len);
  State state = write();
  if (state != State::WRITE_READY) {
    PollManager::get_instance().watch(_sock, FD_MODE::WRITE);
  }
}

void Channel::close() {
  ::close(_sock);
}

bool Channel::is_alive() {
  return _alive;
}

std::string Channel::get_msg() {
  std::string msg;
  msg.assign(_write_buffer->c_str());
  return msg;
}

void Channel::clear_msg() {
  _write_buffer->reset();
  _busy = false;
  _read_cond.notify_all();
}

PollServer::PollServer(std::string port)
    :ServerConnector(port), _stop(false), _thread(this) {
#ifdef JSONRPC_DEBUG
  CLASS_INIT_LOGGER("PollServer", L_DEBUG)
#endif

}

int PollServer::start() {
  _listen();

  PollManager::get_instance().watch(_sock, FD_MODE::READ);
  _thread.start();
  return 0;
}

void PollServer::loop() {
  struct sockaddr_storage remote_client;
  socklen_t addrlen = sizeof(remote_client);

  std::vector<int> read_fds;
  std::vector<int> write_fds;
  while( !_stop ) {
    read_fds.clear();
    write_fds.clear();
    PollManager::get_instance().poll(read_fds, write_fds);

    auto it = read_fds.begin();
    for(; it != read_fds.end(); it ++) {
      CLOG_DEBUG("%d fd selected in read\n", *it);
      if (*it != _sock)  {
        Channel::State state = _channels[*it]->read();
        if (state == Channel::State::READ_READY) {
          CLOG_DEBUG("%d fd ready to deal with request\n", *it);
          //PollManager::get_instance().unwatch(*it, FD_MODE::READ);
          _add_job_wrapper(_channels[*it]);
        } else if (state == Channel::State::CLOSED) {
          // close channel
          PollManager::get_instance().unwatch(*it, FD_MODE::READ);
          PollManager::get_instance().unwatch(*it, FD_MODE::WRITE);
          _channels[*it]->close();
          delete _channels[*it];
          _channels.erase(*it);
          CLOG_DEBUG("%d fd, connection close\n", *it);
        }
      } else {
        //handle new connection
        int new_sock = accept(_sock, (struct sockaddr*)& remote_client, &addrlen);
        if (new_sock == -1) {
          CLOG_FATAL("");
        }

        CLOG_DEBUG("a new connection %d\n", new_sock);
        PollManager::get_instance().watch(new_sock, FD_MODE::READ);
        _channels[new_sock] = new Channel(new_sock, this);
      }
    }

    for(it = write_fds.begin(); it != write_fds.end(); it ++ ) {
      CLOG_DEBUG("%d fd selected in write\n", *it);
      Channel::State state = _channels[*it]->write();
      if (state == Channel::State::WRITE_READY) {
        CLOG_DEBUG("%d fd finish write\n", *it);
        PollManager::get_instance().unwatch(*it, FD_MODE::WRITE);
        //PollManager::get_instance().watch(*it, FD_MODE::READ);
      }
    }

    CLOG_DEBUG("finish one select\n");
  }
}

void PollServer::_close_channels() {
  auto it = _channels.begin();
  for(; it != _channels.end(); it ++ ) {
    if (it->second->is_alive()) {
      it->second->close();
    }
  }
  _channels.clear();
}

int PollServer::stop() {
  _close_channels();
  _stop = true;
  return 0;
}

PollServer::~PollServer() {
  if (_stop == false) {
    stop();
  }
}

void PollServer::_add_job_wrapper(Channel* chan) {
  _thread_pool.add(&PollServer::_handle_request, this, chan);
}

void PollServer::_handle_request(Channel* chan) {
  try {
    CLOG_DEBUG("Start to handle request\n");
    std::string msg = chan->get_msg();
    CLOG_DEBUG("get message %s\n", msg.c_str());
    chan->clear_msg();

    size_t handler_id;
    Request request = Proto::build_request(msg, handler_id); // could throw json parse exception
    CLOG_DEBUG("build request\n");
    int r = _handler->on_request(handler_id, &request);
    CLOG_DEBUG("finish handling\n");

    if (r == 0) {
      throw ServerMethodNotFoundException();
    }

    msg = Proto::build_response(request.get_response());
    CLOG_DEBUG("send back msg %s\n", msg.c_str());
    chan->send(msg);
  } catch(ServerException& e) {
    CLOG_DEBUG("%s\n", e.what());
    std::string err_msg = Proto::build_error(e.get_code());
    chan->send(err_msg);
  }
}
