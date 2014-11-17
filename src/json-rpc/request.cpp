#include "json-rpc/server/request.hpp"

Request::Request(JValue* array, JValue* msg_json)
    : _sin(array), _msg_json(msg_json) {
}

Request::Request(Request&& other)
    : _sin(std::move(other._sin) ), _msg_json(std::move(other._msg_json)) {
}
