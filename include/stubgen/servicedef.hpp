#ifndef __JSONRPC_SERVICEDEF_HPP__
#define __JSONRPC_SERVICEDEF_HPP__

#include "common/all.hpp"
#include "jconer/json.hpp"

#include <sstream>

using namespace COMMON;
using JCONER::JValue;

#define BASE_SERVICE_NAME "AbstractService"
#define BASE_CLIENT_NAME "AbstractClient"
#define BASE_SERVER_CONNECTOR "ServerConnector"
#define BASE_CLIENT_CONNECTOR "ClientConnector"

class CppWriter;

class ServiceDef {
  public:


    class Function {
      public:
        Function(): _name(""), _rettype("void"), _cname("") {
          _params.clear();
        }

        Function(std::string name)
            : _name(name), _rettype("void"), _cname("") {
          _params.clear();
        }

        Function(std::string name, std::string cname)
            : _name(name), _rettype("void"), _cname(cname) {
          _params.clear();
        }

        Function(const Function& other)
            : _name(other._name), _rettype(other._rettype) , _cname(other._cname) {
          _params.clear();
          _params.insert(other._params.begin(), other._params.end());
        }

        Function(Function&& other)
            : _name(other._name), _rettype(other._rettype), _cname(other._cname) {
          _params = std::move(other._params);
        }

        Function& operator=(Function&& other) {
          _name = other._name;
          _rettype = other._rettype;
          _cname = other._cname;
          _params = std::move(other._params);
          return *this;
        }

        void add_param(std::string name, std::string type) {
          if (_params.count(name) != 0) {
            CLOG_INFO("Refine param %s\n", name.c_str());
          }
          _params[name] = type;
        }

        void set_name(std::string name) { _name = name; }
        void set_cname(std::string cname) { _cname = cname; }
        void set_rettype(std::string type) { _rettype = type; }

        const std::string get_rettype() const { return _rettype; }
        const std::string get_name() const { return _name; }
        const std::map<std::string, std::string>& get_params() const { return _params; }

        const std::string get_declaration(bool with_class = false) const {
          std::string decl = _rettype + " ";
          if (with_class) {
            decl += _cname +"::";
          }
          decl += _name + "(";

          auto it = _params.begin();
          for(; it != _params.end(); ) {
            decl += it->second + " " + it->first;
            if (++it != _params.end()) {
              decl += ", ";
            }
          }
          decl += ")";
          return decl;
        }

        static Function from_json(JValue* value);

      private:
        std::string _name;
        std::string _rettype;
        std::string _cname; // class name
        std::map<std::string, std::string> _params;

      CLASS_MAKE_LOGGER
    };


    ServiceDef(const std::string name): _name(name) {
      _functions.clear();
    }

    ServiceDef(ServiceDef&& other): _name(other._name) {
      _functions = std::move(other._functions);
    }

    void add_function(const Function& func) {
      _functions.push_back(func);
    }

    const std::string get_name() const { return _name; }

    static ServiceDef from_json(std::string name, JValue* value);
    friend class CppWriter;

  private:
    std::string _name;
    std::vector<Function> _functions;
};

ServiceDef::Function ServiceDef::Function::from_json(JValue* value) {
  assert(value->isObject());

  JValue* name = value->get("name");
  JValue* param = value->get("params");
  JValue* returns = value->get("return");

  assert(name != NULL && name->isString());
  ServiceDef::Function func(name->getString());

  assert(param != NULL); 
  if (param->isObject()) {
    std::vector<std::string> keys = param->getKeys();
    for(int i = 0; i < keys.size(); i ++ ) {
      func.add_param(keys[i], param->get(keys[i])->getString());
    }
  } else {
    assert(param->isNull());
  }

  if( returns != NULL ) {
    assert( returns->isString());
    func.set_rettype(returns->getString());
  }
  return func; 
}

ServiceDef ServiceDef::from_json(std::string name, JValue* value) {
  ServiceDef servicedef(name);

  for(int i = 0; i < value->size(); i ++ ) {
    ServiceDef::Function func = ServiceDef::Function::from_json(value->get(i));
    func.set_cname(servicedef.get_name());
    servicedef.add_function(func);
  }

  return servicedef;
}

#endif
