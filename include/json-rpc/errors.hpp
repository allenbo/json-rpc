#ifndef __JSONRPC_ERRORS_HPP__
#define __JSONRPC_ERRORS_HPP__

#include "json-rpc/proto.hpp"
#include <exception>

/**
 * Exceptions that will be triggered on server
 **/
class ServerException: public std::exception {
  public:
    ServerException(int code) :_code(code) {}
    const int get_code() const { return _code; }
  private:
    int _code;
};

/**
 * Socket connecting client closed. Server will throw out this exception if
 * it detects client closes the connection.
 **/
class ServerCloseSocketException: public ServerException {
  public:
    ServerCloseSocketException() : ServerException(SOCKET_CLOSED) {}
    const char* what() const throw() {
      return "Server closes socket";
    }
};

/**
 * When a message to server doens't have a legal size, meaning the first four
 * bytes is broken and server can't know the size of the incomming message.
 **/
class ServerBadMessageException: public ServerException {
  public:
    ServerBadMessageException() : ServerException(BAD_MESSAGE) {}
    const char* what() const throw() {
      return "Bad message coming";
    }
};

/**
 * Server can't find registered method that match the method id in message.
 **/
class ServerMethodNotFoundException: public ServerException {
  public:
    ServerMethodNotFoundException(): ServerException(METHOD_NOT_FOUND) {}
    const char* what() const throw() {
      return  "Method not found in server";
    }
};

/**
 * Message is not a json-formatted text.
 **/
class ServerJsonNotParsedException : public ServerException {
  public:
    ServerJsonNotParsedException() : ServerException(JSON_NOT_PARSED) {}
    const char* what() const throw() {
      return "Json not parsed in server";
    }
};

/**
 * The parameters in the message do match with the arguments of the targetting
 * method.
 **/
class ServerParamMismatchException : public ServerException {
  public:
    ServerParamMismatchException(): ServerException(PARAM_MISMATCH) {}
    const char* what() const throw() {
      return "Parameters mismatch in server";
    }
};

/**
 * When client calls a method that has a return value but the response from
 * server doesn' have one. This will never happen if users don't change the
 * code generated by stubgen.
 **/
class NoResultException : public std::exception {
  public:
    const char* what() const throw() {
      return "No result return from server";
    }
};

/**
 * Socket read failed
 **/
class ReadFailException : public std::exception {
  public:
    const char* what() const throw() {
      return "Read function failed";
    }
};

/**
 * Socket write failed
 **/
class WriteFailException : public std::exception {
  public:
    const char* what() const throw() {
      return "Write function failed";
    }
};

/**
 * Socket function fails, socket, bind, listen
 **/
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

/**
 * Host function fails
 **/
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
