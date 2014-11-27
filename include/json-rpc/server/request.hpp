#ifndef __JSONRPC_REQUEST_HPP__
#define __JSONRPC_REQUEST_HPP__

#include "jconer/json.hpp"
using namespace JCONER;

/**
 * Response contains a out serializer that holds the message to client
 **/
class Response {
  public:
    Response() {};

    OutSerializer& get_serializer() { return _sout; }

  private:
    OutSerializer _sout;
};

/**
 * Request contains a in serializer and a response
 **/
class Request {
  public:
    Request(int clientno, int serverno, int version, long timestamp,
            int messageid, size_t handlerid, JValue* array, JValue* msg_json);
    Request(Request&& o);

    ~Request() {
      if (_msg_json) delete _msg_json;
    }

    InSerializer& get_serializer() { return _sin; }
    Response& get_response() { return _resp;}

    inline int clientno() { return _clientno; }
    inline int serverno() { return _serverno; }
    inline long timestamp() { return _timestamp; }
    inline int version() { return _version; }
    inline int messageid() { return _messageid; }
    inline size_t handlerid() { return _handlerid; }

  private:
    
    // information of message
    int _clientno;
    int _serverno;
    int _version;
    long _timestamp;
    int _messageid;
    size_t _handlerid; // in server, it's called handler id

    InSerializer _sin;
    JValue* _msg_json;
    Response _resp;

};

#endif
