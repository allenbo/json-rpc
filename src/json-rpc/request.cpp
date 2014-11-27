#include "json-rpc/server/request.hpp"

Request::Request(int clientno, int serverno, int version, long timestamp,
        int messageid, size_t handlerid, JValue* array, JValue* msg_json)
    :_clientno(clientno), _serverno(serverno), _version(version),
     _timestamp(timestamp), _messageid(messageid), _handlerid(handlerid),
     _sin(array), _msg_json(msg_json) {
}

Request::Request(Request&& o)
    :_clientno(o._clientno), _serverno(o._serverno), _version(o._version),
     _timestamp(o._timestamp), _messageid(o._messageid), _handlerid(o._handlerid),
     _sin(std::move(o._sin)), _msg_json(std::move(o._msg_json)) {
}

