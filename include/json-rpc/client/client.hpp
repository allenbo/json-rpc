#ifndef __JSONRPC_CLIENT_HPP__
#define __JSONRPC_CLIENT_HPP__

#include "json-rpc/client/cconn.hpp"
#include "json-rpc/proto.hpp"
#include "json-rpc/errors.hpp"
#include "jconer/json.hpp"
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <unistd.h>

using namespace JCONER;

/**
 * Abstract client that will be inherited by generated client. The generated
 * client has some wrappers that invoke "call" function of this class to send
 * parameters to and receive result from server.
 */
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
    /* Call function w/o return value
     */
    void call(size_t method_hash, OutSerializer& sout);

    /* Call function w/ return value
     * TODO: should find a way to merge there two functions
     */
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

  int tried = 0;
  static const int MAX_TRY = 8;

  while (true) {
    try {
      std::string rst = "";
      _client.send_and_response(msg, rst);
      CLOG_DEBUG("result from connector %s\n", rst.c_str());

      _parse_response(rst);
      break;
    } catch (ServerException & e) {
      _client.reconnect();  
    } catch (ReadFailException & e ) {
      tried ++;
    } catch (WriteFailException & e) {
      tried ++;
    }
  }

  if (tried > MAX_TRY) {
    throw std::runtime_error("PRC call failed");
  }
}

template<class R>
void AbstractClient::call (size_t method_hash, OutSerializer& sout, R* r) {
  std::string msg = _build_request(method_hash, sout);
  CLOG_DEBUG("build request %s\n", msg.c_str());

  int tried = 0;
  static const int MAX_TRY = 8;

  while (true) {
    try {
      std::string rst;
      _client.send_and_response(msg, rst);
      CLOG_DEBUG("result from connector %s\n", rst.c_str());

      _parse_response(rst, *r);
      break;
    } catch(ServerCloseSocketException& e) {
      _client.reconnect();  
    } catch (ReadFailException & e ) {
      tried ++;
    } catch (WriteFailException & e) {
      tried ++;
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
      case BAD_MESSAGE:
        CLOG_DEBUG("Message sent out was broken\n");
        throw ServerBadMessageException();
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
