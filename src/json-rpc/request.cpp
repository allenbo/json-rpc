#include "json-rpc/request.hpp"

Request::Request(JValue* array) : _sin(array) {
}

Request::Request(Request&& other)
    : _sin(std::move(other._sin) ) {
}
