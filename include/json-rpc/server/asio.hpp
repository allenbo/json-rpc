#ifndef __JSONPRC_ASIO_HPP__
#define __JSONPRC_ASIO_HPP__

#include <cstdlib>
#include "json-rpc/server/request.hpp"

// base class for abstract service
class ASIO {
  public:
    virtual int on_request(Request*) = 0;
};

#endif
