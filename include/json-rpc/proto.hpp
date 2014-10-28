#ifndef __JSONRPC_PROTO_HPP__
#define __JSONRPC_PROTO_HPP__

#include <iostream>

static const std::string METHOD = "method";
static const std::string PARAM  = "params";
static const std::string VERSION = "version";
static const std::string CLIENTNO = "clientno";
static const std::string MESSAGEID = "messageid";
static const std::string RESULT = "result";
static const std::string ERROR = "error";

//error from server
static const int SOCKET_CLOSED = 1;
static const int METHOD_NOT_FOUND = 2;
static const int JSON_NOT_PARSED = 3;
static const int PARAM_MISMATCH = 4;

//error in connection
static const int READ_FAIL = 5;
static const int WRITE_FAIL = 6;
static const int BIND_FAIL = 7;
static const int ACCEPT_FAIL = 8;
static const int SOCKET_FAIL = 9;

#endif
