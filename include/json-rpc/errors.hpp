#ifndef __JSONRPC_ERRORS_HPP__
#define __JSONRPC_ERRORS_HPP__

#include "json-rpc/proto.hpp"
#include <exception>

class ServerException: public std::exception {
  public:
    ServerException(int code) :_code(code) {}
    const int get_code() const { return _code; }
  private:
    int _code;
};

class ServerCloseSocketException: public ServerException {
  public:
    ServerCloseSocketException() : ServerException(SOCKET_CLOSED) {}
    const char* what() const throw() {
      return "Server closes socket";
    }
};

class ServerBadMessageException: public ServerException {
  public:
    ServerBadMessageException() : ServerException(BAD_MESSAGE) {}
    const char* what() const throw() {
      return "Bad message coming";
    }
};

class ServerMethodNotFoundException: public ServerException {
  public:
    ServerMethodNotFoundException(): ServerException(METHOD_NOT_FOUND) {}
    const char* what() const throw() {
      return  "Method not found in server";
    }
};

class ServerJsonNotParsedException : public ServerException {
  public:
    ServerJsonNotParsedException() : ServerException(JSON_NOT_PARSED) {}
    const char* what() const throw() {
      return "Json not parsed in server";
    }
};

class ServerParamMismatchException : public ServerException {
  public:
    ServerParamMismatchException(): ServerException(PARAM_MISMATCH) {}
    const char* what() const throw() {
      return "Parameters mismatch in server";
    }
};


class NoResultException : public std::exception {
  public:
    const char* what() const throw() {
      return "No result return from server";
    }
};

class ReadFailException : public std::exception {
  public:
    const char* what() const throw() {
      return "Read function failed";
    }
};

class WriteFailException : public std::exception {
  public:
    const char* what() const throw() {
      return "Write function failed";
    }
};

class SocketFailException : public std::exception {
  public:
    SocketFailException(std::string funcname): _funcname(funcname){}

    const char* what() const throw() {
      _rst = _funcname + " function failed";
      return _rst.c_str();
    }

  private:
    std::string _funcname;
    mutable std::string _rst;
};


class HostFailException : public std::exception {
  public:
    HostFailException(std::string desc, std::string host, std::string port)
        :_desc(desc), _host(host), _port(port) {}
    const char* what() const throw() {
      _rst = _desc + "@" + _host +":" + _port;
      return _rst.c_str();
    }
  private:
    std::string _desc;
    std::string _host;
    std::string _port;
    mutable std::string _rst;
};

#endif
