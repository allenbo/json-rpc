#ifndef __JSONRPC_WRITER_HPP__
#define __JSONRPC_WRITER_HPP__

#include <fstream>
#include <iostream>
#include <functional>
#include <algorithm>
#include "common/all.hpp"
#include "stubgen/servicedef.hpp"
#include "stubgen/classdef.hpp"

class AbstractWriter {
  public:
    AbstractWriter(std::string spacename) :_spacename(spacename) {
    }
    virtual void write() = 0;
    virtual ~AbstractWriter() = 0;
  protected:
    std::string _spacename;
};

AbstractWriter::~AbstractWriter() {}

class CppWriter : public AbstractWriter {
  public:
    static const int INDENT = 2;
    CppWriter(std::string spacename, std::vector<ClassDef>& classdefs, ServiceDef& servicedef)
        : AbstractWriter(spacename), _classdefs(classdefs), _servicedef(servicedef) {
    }

    void write() {
      std::string filename = _get_filename();
      std::ofstream fout(filename.c_str());

      // header protection
      fout << "#ifndef " << _get_header_def() << std::endl
           << "#define " << _get_header_def() << std::endl;

      // include header files
      fout << "\n\n#include \"json-rpc/all.hpp\"\n"
           << "#include \"jconer/json.hpp\"\n"
           << "#include <iostream>\n"
           << "#include <string>\n"
           << "#include <vector>\n"
           << "#include <map>\n"
           << "#include <list>\n"
           << "#include <set>\n"
           << std::endl;
      fout << "using namespace std;\n"
           << "using namespace JCONER;\n"
           << std::endl;

      // class definition
      for(int i = 0; i < _classdefs.size(); i ++ ) {
        ClassDef def = _classdefs[i];
        fout << "//Definition of class " << def._name << std::endl;
        fout << "class "  << def._name << " {\n\n";
        auto it = def._members.begin();
        for(; it != def._members.end(); it ++) {
          fout << "  " << it->second << " " << it->first<< ";\n";
        }
        fout << "\n\n";
        fout << "//Define serializer\n";
        fout << _get_indent(1) + "public:\n";
        fout << _get_serializer(def._members); 
        fout <<"\n\n};" << std::endl;
      }

      // protocol definition
      std::hash<std::string> hash_fn;
      fout << "//Protocol Definition\n"
           << "class " + _get_protocol_name() + "{\n"
           << _get_indent(1) + "public:\n"
           << _get_indent(2) + "enum {\n";

      std::vector<std::string> func_upper_names;
      std::vector<std::string> func_wrapper_names;
      for(int i = 0; i < _servicedef._functions.size(); i ++) {
        std::string upper_name = VarString::toupper(_servicedef._functions[i].get_name());
        func_upper_names.push_back(upper_name);
        fout << _get_indent(3) + upper_name << "=" << std::abs((int)hash_fn(upper_name)) << ",\n";
      }
      fout << _get_indent(2) + "};\n};\n";
 
      // service definition
      fout << "//Service Definition\n";
      fout << "class " << _get_service_name() << ": public "
           << BASE_SERVICE_NAME << "<" + _get_service_name() +"> {\n\n";

      // wrappers 
      auto it = _servicedef._functions.begin();
      for(int i = 0; i < _servicedef._functions.size(); i ++) {
        std::string name = _servicedef._functions[i].get_name();
        std::string wrapper_name = "_" + name + "_wrapper";

        func_wrapper_names.push_back(wrapper_name);
        auto params = _servicedef._functions[i].get_params();

        fout << _get_indent(1) + "void " + wrapper_name + "(Request* req) {\n";

        // body of wrapper
        std::string param_list_string = "";
        // if there are parameters
        if (params.size() != 0) {

          fout << _get_indent(2) + "InSerializer& sin = req->get_serializer();\n";

          int count = 0;
          auto param_it = params.begin();

          for(; param_it != params.end(); param_it ++) {
            std::string param_type = param_it->second;
            std::string param_name = "_arg" + VarString::itos(count);

            param_list_string += param_name;
            if (count != params.size() - 1) {
              param_list_string += ", ";
            }

            fout << _get_indent(2) + param_type + " " + param_name + ";\n";
            fout << _get_indent(2) + "sin & " + param_name + ";\n";

            count ++;
          }
          fout << "\n";
        }

        if (_servicedef._functions[i].get_rettype() == "void") {
          fout << _get_indent(2) + name + "(" + param_list_string + ");\n";
        } else {
          // if there is a return value
          std::string rettype = _servicedef._functions[i].get_rettype();
          fout << _get_indent(2) + rettype + " __r = "
               << name + "(" + param_list_string + ");\n";
          fout << _get_indent(2) + 
            "OutSerializer& sout = req->get_response().get_serializer();\n";
          fout << _get_indent(2) + "sout & __r;\n";
        }
        fout << _get_indent(1) + "}\n\n";
      }

      fout << "\n\n";

      fout << _get_indent(1) + "public:\n\n";

      // constructor
      fout << _get_indent(2) + _get_service_name() + "(" + BASE_SERVER_CONNECTOR
        + "& server):" + BASE_SERVICE_NAME + "<" + _get_service_name() + ">"
        + "(server) {\n";
      fout << _get_indent(3) + "register_service();\n";
      fout << _get_indent(2) + "}\n\n";

      // reg funtion
      fout << _get_indent(2) + "void _reg_wrappers() {\n";
      for(int i = 0; i < func_upper_names.size(); i ++){
        fout << _get_indent(3) + "reg(" + _get_protocol_name() + "::" +
          func_upper_names[i] + ", &" + _get_service_name() + "::" +
          func_wrapper_names[i] + ");\n";
      }
      fout << _get_indent(2) << "}\n\n";
      

      it = _servicedef._functions.begin();
      for(; it != _servicedef._functions.end(); it ++) {
        fout << _get_indent(2) + "virtual " << it->get_declaration() << "{\n"
             << _get_indent(3) + "//TODO: stub HERE\n"
             << _get_indent(2) + "}\n";
      }

      fout << "\n\n};\n";
      
      // client definition
      fout <<  "//Client Definition\n";
      fout << "class " << _get_client_name() << " : public "
           << BASE_CLIENT_NAME << " {\n\n";

      fout << _get_indent(1) << "public:\n";

      fout << _get_indent(2) + _get_client_name()  + "(" + BASE_CLIENT_CONNECTOR + "& client) : "
        + BASE_CLIENT_NAME + "(client) {}\n";

      it = _servicedef._functions.begin();
      int count = 0;
      for(; it != _servicedef._functions.end(); it ++) {
        fout << _get_indent(2) + "virtual " << it->get_declaration() << " {\n";
        fout << _get_indent(3) + "OutSerializer sout;\n";
        
        auto param_map = it->get_params();
        std::for_each(param_map.begin(), param_map.end(), 
            [&] (typename std::map<std::string, std::string>::value_type a) {
              fout << _get_indent(3) + "sout & " + a.first + ";\n";
            }
        );
        std::string rettype = it->get_rettype();
        if (rettype != "void") {
          fout << _get_indent(3)  + rettype + " __r;\n";
        }


        if (rettype != "void") {
          fout << _get_indent(3) + "call(" + _get_protocol_name() +  "::" +
            func_upper_names[count] + ", sout, &__r);\n";
          fout << _get_indent(3) + "return __r;\n";
        } else {
          fout << _get_indent(3) + "call(" + _get_protocol_name() + "::" +
            func_upper_names[count] + ", sout);\n";
        }

        fout << _get_indent(2) + "}\n\n"; 
        count ++;
      }

      fout << "\n\n};\n";


      fout << "#endif //" << _get_header_def();
    }

  private:
    inline std::string _get_filename() {
      return _spacename + "CppStub.hpp";
    }

    inline std::string _get_protocol_name() {
      return _spacename + "Protocol";
    }

    inline std::string _get_service_name() {
      return _spacename + "Service";
    }

    inline std::string _get_client_name() {
      return _spacename + "Client";
    }

    inline std::string _get_header_def() {
      return "__" + VarString::toupper(_spacename) + "_CPP_STUB_HPP__";
    }

    std::string _get_indent(int level = 0) {
      std::string indent = "";
      for(int i = 0; i < level; i ++) {
        indent += "  ";
      }
      return indent;
    }

    std::string _get_serializer(std::map<std::string, std::string>& variables) {
      std::string pattern = 
        _get_indent(2) + "template<class Serializer>\n" + 
        _get_indent(3) +   "void seralize(Serializer& serializer) {\n";
      auto it = variables.begin();
      for(; it != variables.end(); it ++ ) {
        pattern += 
         _get_indent(4) + "serializer & " + it->first + ";\n";
      }
      pattern +=
        _get_indent(3) + "}\n";
      return pattern;
    }

    std::vector<ClassDef>& _classdefs;
    ServiceDef& _servicedef;
};

#endif
