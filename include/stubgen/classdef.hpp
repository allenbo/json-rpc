#ifndef __JSONRPC_CLASSDEF_HPP__
#define __JSONRPC_CLASSDEF_HPP__

#include "common/all.hpp"
#include "jconer/json.hpp"
#include <sstream>
#include <cassert>

using namespace COMMON;
using JCONER::JValue;

class CppWriter;

class ClassDef {
  public:
    ClassDef() : _name("") {
        _members.clear();
    }

    ClassDef(const ClassDef& other) : _name(other._name) {
        _members.clear();
        _members.insert(other._members.begin(), other._members.end());
    }

    ClassDef(ClassDef&& other) : _name(other._name), _members(std::move(other._members))  {
    }

    void add_member(std::string name, std::string type) {
      if (_members.count(name) != 0) {
        CLOG_INFO("Redefine member %s\n", name.c_str());
      }
      _members[name] = type;
    }

    void set_name(std::string name)  {
      _name = name;
    }

    const std::string get_name() const { return _name; }

    static ClassDef from_json(std::string name, JValue* value);

    friend class CppWriter;

  private:
    std::string _name;
    std::map<std::string, std::string> _members;

  CLASS_MAKE_LOGGER
};

ClassDef ClassDef::from_json(std::string name, JValue* value) {
  assert(value->isObject());

  ClassDef def;
  def.set_name(name);
  auto keys = value->getKeys();

  for(int i = 0; i < keys.size(); i ++) {
    def.add_member(keys[i], value->get(keys[i])->getString());
  }

  return def;
}

#endif
