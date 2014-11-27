#ifndef __JSONPRC_SERVICE_HPP__
#define __JSONPRC_SERVICE_HPP__

#include "json-rpc/server/asio.hpp"
#include "json-rpc/server/sconn.hpp"
#include <functional>
#include "json-rpc/errors.hpp"
#include "jconer/json.hpp"
#include <functional>

using namespace JCONER;

/**
 * Abstract service that will be inherited by any other concrete service.
 * The way it works is this service will need a server connector to handle
 * the connection and request. After a request has been built, the server
 * connector will trigger the callback function in this service. All
 * callback functions have to be register when service initializing itself.
 **/
template<class S>
class AbstractService : public ASIO {
  public:
    AbstractService(ServerConnector& s): ASIO(), _s(s) {
      _s.register_service(this);
    }

    int listen() {
      _s.start();
      return 0;
    }

    int stop() {
      _s.stop();
      return 0;
    }
  
  protected :
    int register_service() {
      S* s = dynamic_cast<S*>(this);
      s->_reg_wrappers();
      return 0;
    }

    int reg(size_t handler_id, std::function<void(S&, Request*)> handler) {
      int rst = 0;
      if (_handlers.count(handler_id) != 0) {
        LOG(INFO) << "handld has been registered " << handler_id << std::endl;
        rst = 1;
      }
      _handlers[handler_id] = handler;
      return rst;
    }


    int on_request(Request* request) {
      size_t handler_id = request->handlerid();
      if (_handlers.count(handler_id) == 0) {
        // No method found
        return 0;
      } else {
        try {
          _handlers[handler_id](*dynamic_cast<S*>(this), request);
        } catch (SerializeFailException& e){
          throw ServerParamMismatchException();
        }
        return 1;
      }
    }

  private:
    ServerConnector& _s;
    std::map<size_t, std::function<void(S&, Request*)> > _handlers;
};

#endif
