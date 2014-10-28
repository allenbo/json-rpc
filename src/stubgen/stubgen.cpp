#include <stdlib.h>
#include <iostream>
#include <cassert>
#include <map>

#include "stubgen/servicedef.hpp"
#include "stubgen/classdef.hpp"
#include "stubgen/writer.hpp"

#include "jconer/json.hpp"
using namespace JCONER;

void usage(char* progname) {
  std::cerr << "./" << progname << " spec.json" << std::endl;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    usage(argv[0]);
    return -1;
  }

  char* spec_filename = argv[1];

  PError err;
  JValue* spec = load(spec_filename, err);
  if(spec == NULL) {
    std::cerr << "Json file parsing error" << std::endl;
    std::cerr << err.text << std::endl;
    return -1;
  }
  if (! spec->isObject()) {
    std::cerr << "Spec json has to be an object" <<  std::endl
              << "please check out the grammar at file [stubgrammar]"
              << std::endl;
    return -1;
  }
  JValue* spacename = spec->get("namespace");
  JValue* classes = spec->get("classes");
  JValue* service = spec->get("service");

  if (spacename == NULL || ! spacename->isString() ||
      classes == NULL || ! classes->isObject() ||
      service == NULL || ! service->isArray() ) {
    std::cerr << "Grammar error" <<  std::endl
              << "please check out the grammar at file [stubgrammar]"
              << std::endl;
    return -1;
  }

  std::string service_name = spacename->getString();

  std::vector<ClassDef> classdefs;
  std::vector<std::string> classnames = classes->getKeys();
  for(int i = 0; i < classnames.size(); i ++ ) {
    classdefs.push_back(ClassDef::from_json(classnames[i], classes->get(classnames[i]) ) );
  }
  
  ServiceDef servicedef = ServiceDef::from_json(service_name, service);

  CppWriter writer(service_name, classdefs, servicedef);
  writer.write();
  return 0;
}
