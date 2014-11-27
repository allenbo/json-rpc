#include "json-rpc/server/pollserver.hpp"
#include "json-rpc/util.hpp"
#include <vector>

static const int MAX_BUFF_SIZE = 1024;

PollManager::PollManager(): _highest(0) {
  _watched_read_fds.clear();
  _watched_write_fds.clear();
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

  LOG(DEBUG) << VarString::format("watch fd %d as %d", fd, mod) << std::endl;

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

  LOG(DEBUG) << VarString::format("unwatch fd %d as %d", fd, mod) << std::endl;

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

  LOG(DEBUG) << "new highest fd is " << new_highest << std::endl;

  _highest = new_highest;
}

bool PollManager::poll(std::vector<int>& reads, std::vector<int>& writes) {
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
}

Channel::~Channel() {
  if (is_alive()) {
    close();
  }

  if (_read_buffer) delete _read_buffer;
  if (_write_buffer) delete _write_buffer;
}

Channel::State Channel::read() {
  while (_busy) {
    LOG(DEBUG) << "Busy, waiting for operatio to finish" << std::endl;
    _read_cond.wait();
  }

  if (_write_buffer == nullptr) {
    int size = 0;
    int len = ::read(_sock, (void*)&size, sizeof(int)); 

    if (len == 0) {
      return State::CLOSED;
    }

    if (len != sizeof(int) || size <= 0) {
      LOG(INFO) << "Error when read size of incoming message" << std::endl;
      _busy = true;
      return State::BROKEN;
    }

    LOG(DEBUG) << "The size of message is " << size << std::endl;
    _write_buffer = new SizedWRONBuffer(size);
  }

  char chunk[MAX_BUFF_SIZE];
  int len;
  len = ::read(_sock, chunk, MAX_BUFF_SIZE);

  _write_buffer->write(chunk, len);

  if (!_write_buffer->ready()) {
    return Channel::State::READ_PENDING;
  } else {
    _busy = true;
    LOG(DEBUG) << "Set to busy" << std::endl;
    return Channel::State::READ_READY;
  }
}

Channel::State Channel::write() {
  char chunk[MAX_BUFF_SIZE];
  int len;

  if (_read_buffer == nullptr) return State::WRITE_PENDING;
  len = _read_buffer->read(chunk, MAX_BUFF_SIZE);
  chunk[len] = '\0';

  ::write(_sock, chunk, len);

  if (_read_buffer->ready()) {
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
  LOG(DEBUG) << "get send lock" << std::endl;
  if (_alive == false) {
    _send_mutex.unlock();
    return;
  }

  if (_read_buffer != nullptr) {
    delete _read_buffer;
  }

  _read_buffer = new SizedRDONBuffer(msg, len);
  State state = write();
  if (state != State::WRITE_READY) {
    PollManager::get_instance().watch(_sock, FD_MODE::WRITE);
  } else {
    _send_mutex.unlock();
  }
}

void Channel::close() {
  ScopeLock _(&_send_mutex);
  shutdown(_sock, SHUT_RDWR);
  ::close(_sock);
  _alive = false;
}

bool Channel::is_alive() {
  return _alive;
}

std::string Channel::get_msg() {
  if (_write_buffer == nullptr) {
    return "";
  }
  std::string msg = "";
  msg.assign(_write_buffer->c_str());
  return msg;
}

void Channel::clear_msg() {
  if (_write_buffer != nullptr) {
    delete _write_buffer;
  }
  _write_buffer = nullptr;
  _busy = false;
  _read_cond.notify_all();
}

PollServer::PollServer(std::string port)
    :ServerConnector(port), _stop(false), _thread(this) {
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
      if (*it != _sock)  {
        // A watched connection finish reading a packet
        // Check if the channel is alive or not
        if (_channels.count(*it) == 0) {
          // Channel deleted
          LOG(DEBUG) << "Channel related to fd " << *it << " has been erased" << std::endl;
          continue;
        }
        Channel::State state = _channels[*it]->read();

        if (state == Channel::State::READ_READY) {
          // Finish read entire message
          LOG(DEBUG) << "Channel finish reading message" << std::endl;
          _add_job_wrapper(_channels[*it]);
        } else if (state == Channel::State::CLOSED || state == Channel::State::BROKEN) {
          // Remote client close the connection, so close channel
          LOG(DEBUG) << "Remote client closed the connection" << std::endl;
          _channels[*it]->close();
          PollManager::get_instance().unwatch(*it, FD_MODE::READ);
          PollManager::get_instance().unwatch(*it, FD_MODE::WRITE);
          delete _channels[*it];
          _channels.erase(*it);
          LOG(DEBUG) << *it << " fd, connection close" << std::endl;
        }
      } else {
        //handle new connection
        int new_sock = accept(_sock, (struct sockaddr*)& remote_client, &addrlen);
        if (new_sock == -1) {
          LOG(FATAL) << "Error when acceting" << std::endl;
        }

        LOG(DEBUG) << "A new connection " <<  new_sock << std::endl;
        PollManager::get_instance().watch(new_sock, FD_MODE::READ);
        _channels[new_sock] = new Channel(new_sock, this);
      }
    }

    for(it = write_fds.begin(); it != write_fds.end(); it ++ ) {
      if (_channels.count(*it) == 0) {
        continue;
      }
      Channel::State state = _channels[*it]->write();
      if (state == Channel::State::WRITE_READY) {
        LOG(DEBUG) << *it << " fd finish write" << std::endl;
        PollManager::get_instance().unwatch(*it, FD_MODE::WRITE);
      }
    }
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
    LOG(DEBUG) << "Start to handle request" << std::endl;
    std::string msg = chan->get_msg();
    chan->clear_msg();

    if (msg == "") {
      throw ServerBadMessageException();
    }

    LOG(DEBUG) << "get message " << msg.c_str() << std::endl;
    Request request = Proto::build_request(msg); // could throw json parse exception
    int r = _handler->on_request(&request);

    if (r == 0) {
      throw ServerMethodNotFoundException();
    }

    msg = Proto::build_response(request.get_response());
    LOG(DEBUG) << "send back msg " << msg.c_str() << std::endl;
    chan->send(msg);
  } catch(ServerException& e) {
    LOG(DEBUG) << e.what() << std::endl;
    std::string err_msg = Proto::build_error(e.get_code());
    chan->send(err_msg);
  }
}
