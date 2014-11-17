#ifndef __JSONRPC_REQUEST_HPP__
#define __JSONRPC_REQUEST_HPP__

#include "jconer/json.hpp"
using namespace JCONER;

class Response {
  public:
    Response() {};

    OutSerializer& get_serializer() { return _sout; }

  private:
    OutSerializer _sout;
};

class Request {
  public:
    Request(JValue*, JValue*);
    Request(Request&& other);
    ~Request() {
      if (_msg_json) delete _msg_json;
    }

    InSerializer& get_serializer() { return _sin; }
    Response& get_response() { return _resp;}

  private:
    InSerializer _sin;
    Response _resp;
    JValue* _msg_json;
};

#endif