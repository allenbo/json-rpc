#include "json-rpc/proto.hpp"
#include "json-rpc/errors.hpp"

/**
 * Parse message to return a request and handler id
 */
Request Proto::build_request(std::string msg) {
  PError err;
  JValue* req_json = loads(msg, err);

  // check json integrity
  if (req_json == nullptr || !req_json->isObject()) {
    throw ServerJsonNotParsedException();
  }
  
  // check clientno
  JValue* item_json = req_json->get(ClientNo);
  if (item_json == nullptr || !item_json->isInteger()) {
    throw ServerJsonNotParsedException();
  }
  int clientno = item_json->getInteger();

  // check serverno
  item_json = req_json->get(ServerNo);
  if (item_json == nullptr || !item_json->isInteger()) {
    throw ServerJsonNotParsedException();
  }
  int serverno = item_json->getInteger();

  // check version
  item_json = req_json->get(Version);
  if (item_json == nullptr || !item_json->isInteger()) {
    throw ServerJsonNotParsedException();
  }
  int version = item_json->getInteger();

  // check timestamp
  item_json = req_json->get(Timestamp);
  if (item_json == nullptr || !item_json->isInteger()) {
    throw ServerJsonNotParsedException();
  }
  long timestamp = item_json->getInteger();

  // check messageid
  item_json = req_json->get(MessageId);
  if (item_json == nullptr || !item_json->isInteger()) {
    throw ServerJsonNotParsedException();
  }
  int messageid = item_json->getInteger();

  // check handler id
  item_json = req_json->get(Method);
  if (item_json == nullptr || !item_json->isInteger()) {
    throw ServerJsonNotParsedException();
  }
  size_t handlerid = item_json->getInteger();

  // check parameters
  item_json = req_json->get(Param);
  if (item_json == nullptr || !item_json->isArray()) {
    throw ServerJsonNotParsedException();
  }

  Request request(clientno, serverno, version, timestamp, messageid, handlerid, item_json, req_json);
  return request;
}

/**
 * Build a response text based on result from server method
 */
std::string Proto::build_response(Response& resp) {
  JObject* obj = new JObject();
  obj->put(Result, resp.get_serializer().getContent());

  std::string jsonText = dumps(obj);
  delete obj;
  return jsonText; 
}

/**
 * Build an error message given error code
 */
std::string Proto::build_error(int code) {
  JObject* obj = new JObject();
  obj->put(Error, code);

  std::string jsonText = dumps(obj);
  delete obj;
  return jsonText;
}

JValue* Proto::parse_response(std::string response) {
  PError err;
  JValue* json_resp = loads(response, err);
  if (json_resp == nullptr) {
    LOG(FATAL) << VarString::format("Json parse error, json text[%s], error text[%s]",
                    response.c_str(), err.text.c_str()) << std::endl;
  }

  if (! json_resp->isObject() ) {
    LOG(FATAL) << VarString::format("Json format error, response has to be an object. Json text[%s]",
                    response.c_str()) << std::endl;
  }

  if (json_resp->contain(Error) ) {
    int errcode = json_resp->get(Error)->getInteger();
    switch(errcode) {
      case SOCKET_CLOSED:
        LOG(DEBUG) << "Server closed socket" << std::endl;
        delete json_resp;
        throw ServerCloseSocketException();
      case  METHOD_NOT_FOUND:
        LOG(DEBUG) << "Method not found on server" << std::endl;
        delete json_resp;
        throw ServerMethodNotFoundException();
      case JSON_NOT_PARSED:
        LOG(DEBUG) << "Server parse json failed" << std::endl;
        delete json_resp;
        throw ServerJsonNotParsedException();
      case PARAM_MISMATCH:
        LOG(DEBUG) << "Parameters don't match with the method" << std::endl;
        delete json_resp;
        throw ServerParamMismatchException();
        break;
      case BAD_MESSAGE:
        LOG(DEBUG) << "Message sent out was broken" << std::endl;
        delete json_resp;
        throw ServerBadMessageException();
      default:
        LOG(FATAL) << "Unknown error code " << errcode << std::endl;
    }
  }
  return json_resp;
}

std::string Proto::build_request(
                     int clientno, int serverno,
                     int messageid, long timestamp,
                     size_t method_hash, OutSerializer& sout) {
  JObject* obj = new JObject();
  obj->put(ClientNo, clientno);
  obj->put(ServerNo, serverno);
  obj->put(Version, 1);
  obj->put(Timestamp, timestamp);
  obj->put(MessageId, messageid);
  obj->put(Method, method_hash);
  obj->put(Param, sout.getContent());

  std::string msg = dumps(obj);
  delete obj;
  return msg;
}
