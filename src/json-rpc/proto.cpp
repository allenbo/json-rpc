#include "json-rpc/proto.hpp"
#include "json-rpc/errors.hpp"

/**
 * Parse message to return a request and handler id
 */
Request Proto::build_request(std::string msg, size_t& handler_id) {
  PError err;
  JValue* req_json = loads(msg, err);

  if (req_json == nullptr || !req_json->isObject()) {
    throw ServerJsonNotParsedException();
  }

  JValue* item_json = req_json->get(METHOD);
  if (item_json == nullptr || !item_json->isInteger()) {
    throw ServerJsonNotParsedException();
  }
  handler_id = item_json->getInteger();


  item_json = req_json->get(PARAM);
  if (item_json == nullptr || !item_json->isArray()) {
    throw ServerJsonNotParsedException();
  }

  Request request(item_json, req_json);
  return request;
}

/**
 * Build a response text based on result from server method
 */
std::string Proto::build_response(Response& resp) {
  JObject* obj = new JObject();
  obj->put(RESULT, resp.get_serializer().getContent());

  std::string jsonText = dumps(obj);
  delete obj;
  return jsonText; 
}

/**
 * Build an error message given error code
 */
std::string Proto::build_error(int code) {
  JObject* obj = new JObject();
  obj->put(ERROR, code);

  std::string jsonText = dumps(obj);
  delete obj;
  return jsonText;
}
