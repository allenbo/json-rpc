#ifndef __JSONRPC_CLIENT_HPP__
#define __JSONRPC_CLIENT_HPP__

#include "json-rpc/cconn.hpp"
#include "json-rpc/proto.hpp"
#include "json-rpc/errors.hpp"
#include "jconer/json.hpp"
#include <cstdlib>
#include <cstring>

#include <unistd.h>

using namespace JCONER;

class AbstractClient {
  public:
    AbstractClient(ClientConnector& client) : _client(client) {
#ifdef JSONRPC_DEBUG
      CLASS_INIT_LOGGER("AbstractClient", L_DEBUG)
#endif
      srand(getpid());
      _clientno = rand();
      _msg_id = 0;
    }

    virtual ~AbstractClient() { }
  private:
    ClientConnector& _client;
    int _msg_id;
    int _clientno;


  protected:
    void call(size_t method_hash, OutSerializer& sout);

    template<class R>
    void call(size_t method_hash, OutSerializer& sout, R* r);

  private:
    std::string _build_request(size_t method_hash, OutSerializer& sout);

    JValue* _parse_response_internal(std::string response);
    void _parse_response(std::string response);
    template<class R>
      void _parse_response(std::string response, R& r);
    
  CLASS_MAKE_LOGGER
};

void AbstractClient::call(size_t method_hash, OutSerializer& sout) {
  std::string msg = _build_request(method_hash, sout);
  CLOG_DEBUG("build request %s\n", msg.c_str());

  while (true) {
    try {
      std::string rst = "";
      _client.send_and_response(msg, rst);
      CLOG_DEBUG("result from connector %s\n", rst.c_str());

      _parse_response(rst);
      break;
    } catch (ServerCloseSocketException & e) {
      _client.reconnect();  
    }
  }
}

template<class R>
void AbstractClient::call (size_t method_hash, OutSerializer& sout, R* r) {
  std::string msg = _build_request(method_hash, sout);
  CLOG_DEBUG("build request %s\n", msg.c_str());

  while (true) {
    try {
      std::string rst;
      _client.send_and_response(msg, rst);
      CLOG_DEBUG("result from connector %s\n", rst.c_str());

      _parse_response(rst, *r);
      break;
    } catch(ServerCloseSocketException& e) {
      _client.reconnect();  
    }
  }
}


std::string AbstractClient::_build_request(size_t method_hash, OutSerializer& sout) {
  JObject* obj = new JObject();
  obj->put(CLIENTNO, _clientno);
  obj->put(MESSAGEID, _msg_id);
  _msg_id ++;
  obj->put(METHOD, method_hash);
  obj->put(PARAM, sout.getContent());

  std::string msg = dumps(obj);
  return msg;
}

JValue* AbstractClient::_parse_response_internal(std::string response) {
  PError err;
  JValue* json_resp = loads(response, err);
  if (json_resp == nullptr) {
    CLOG_FATAL("Json parse error, json text[%S], error text[%s]\n", response.c_str(), err.text.c_str());
  }

  if (! json_resp->isObject() ) {
    CLOG_FATAL("Json format error, response has to be an object. Json text[%s]\n", response.c_str());
  }

  if (json_resp->contain(ERROR) ) {
    int errcode = json_resp->get(ERROR)->getInteger();
    switch(errcode) {
      case SOCKET_CLOSED:
        CLOG_DEBUG("Server closed socket\n");
        throw ServerCloseSocketException();
        break;
      case  METHOD_NOT_FOUND:
        CLOG_DEBUG("Method not found on server\n");
        throw ServerMethodNotFoundException();
        break;
      case JSON_NOT_PARSED:
        CLOG_DEBUG("Server parse json failed\n");
        throw ServerJsonNotParsedException();
        break;
        throw ServerJsonNotParsedException();
      case PARAM_MISMATCH:
        CLOG_DEBUG("Parameters don't match with the method\n");
        throw ServerParamMismatchException();
        break;
      default:
        CLOG_FATAL("Unknown error code %d\n", errcode);
    }
  }
  return json_resp;
}

void AbstractClient::_parse_response(std::string response) {
    _parse_response_internal(response);
}

template<class R>
void AbstractClient::_parse_response(std::string response, R& r) {
  JValue* json_resp = _parse_response_internal(response);

  if (!json_resp->contain(RESULT)) {
    CLOG_DEBUG("Response text format erro, should have result, [%s]\n", response.c_str());
    throw NoResultException();
  }

  InSerializer sin(json_resp->get(RESULT));
  sin & r;
}

#endif
