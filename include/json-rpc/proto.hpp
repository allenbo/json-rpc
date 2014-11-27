#ifndef __JSONRPC_PROTO_HPP__
#define __JSONRPC_PROTO_HPP__

#include <iostream>
#include "jconer/json.hpp"
#include "json-rpc/server/request.hpp"

using namespace JCONER;

static const std::string ClientNo = "clientno";
static const std::string ServerNo = "serverno";
static const std::string MessageId = "messageid";
static const std::string Version = "version";
static const std::string Timestamp = "timestamp";
static const std::string Method = "method";
static const std::string Param  = "params";
static const std::string Result = "result";
static const std::string Error = "error";

//error in connection
static const int READ_FAIL = 5;
static const int WRITE_FAIL = 6;
static const int BIND_FAIL = 7;
static const int ACCEPT_FAIL = 8;
static const int SOCKET_FAIL = 9;

// Handle with protocol
class Proto {
  public:
    enum {
      SOCKET_CLOSED=1,
      METHOD_NOT_FOUND,
      JSON_NOT_PARSED,
      PARAM_MISMATCH,
      BAD_MESSAGE,
      BAD_RESPONSE,
    };


    // server side protocol functions
    static Request build_request(std::string msg);
    static std::string build_response(Response& resp);
    static std::string build_error(int code);
    // client side protolcol functions
    static JValue* parse_response(std::string response);
    static std::string build_request(
                         int clientno, int serverno,
                         int messageid, long timestamp,
                         size_t method_hash, OutSerializer& sout);
};

#endif
