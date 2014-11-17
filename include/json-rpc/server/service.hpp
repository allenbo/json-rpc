#ifndef __JSONPRC_SERVICE_HPP__
#define __JSONPRC_SERVICE_HPP__

#include "json-rpc/server/asio.hpp"
#include "json-rpc/server/sconn.hpp"
#include <functional>
#include "json-rpc/errors.hpp"
#include "jconer/json.hpp"
#include <functional>

using namespace JCONER;

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
      if (_handlers.count(handler_id) != 0) {
        CLOG_INFO("handld has been registered %d\n", handler_id);
      }
      _handlers[handler_id] = handler;
      return 0;
    }


    int on_request(size_t handler_id, Request* request) {
      if (_handlers.count(handler_id) == 0) {
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
  
    CLASS_MAKE_LOGGER
};

#endif
