#ifndef __JSONPRC_ASIO_HPP__
#define __JSONPRC_ASIO_HPP__

#include <cstdlib>
#include "json-rpc/server/request.hpp"

class ASIO {
  public:
    virtual int on_request(size_t handler_id, Request*) = 0;
};

#endif
