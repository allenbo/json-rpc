#ifndef __JSONRPC_CLIENT_HPP__
#define __JSONRPC_CLIENT_HPP__

#include "json-rpc/client/cconn.hpp"
#include "json-rpc/proto.hpp"
#include "json-rpc/errors.hpp"
#include "jconer/json.hpp"
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <time.h>

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
    void _parse_response(std::string response);
    template<class R>
      void _parse_response(std::string response, R& r);
};

void AbstractClient::call(size_t method_hash, OutSerializer& sout) {
  std::string msg = Proto::build_request(_clientno, 0, _msg_id ++, time(0), method_hash, sout);

  int tried = 0;
  static const int MAX_TRY = 8;

  while (true) {
    try {
      std::string rst = "";
      _client.send_and_response(msg, rst);

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
  std::string msg = Proto::build_request(_clientno, 0, _msg_id ++, time(0), method_hash, sout);

  int tried = 0;
  static const int MAX_TRY = 8;

  while (true) {
    try {
      std::string rst;
      _client.send_and_response(msg, rst);

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
  if (tried > MAX_TRY) {
    throw std::runtime_error("PRC call failed");
  }
}


void AbstractClient::_parse_response(std::string response) {
  JValue* rst = Proto::parse_response(response);
  delete rst;
}

template<class R>
void AbstractClient::_parse_response(std::string response, R& r) {
  JValue* json_resp = Proto::parse_response(response);

  if (!json_resp->contain(Result)) {
    LOG(DEBUG) << VarString::format("Response text format erro, should have result, [%s]\n", response.c_str()) << std::endl;
    delete json_resp;
    throw NoResultException();
  }

  InSerializer sin(json_resp->get(Result));
  sin & r;
  delete json_resp;
}

#endif
