// A minimal JSON value (parse + pretty print) for the run file on the SD card. Numbers are doubles, objects keep
// their key order.
#pragma once
#include <string>
#include <utility>
#include <vector>

namespace sd {

class Json {
 public:
  enum Type { Null, Bool, Number, String, Array, Object };

  Json() : t_(Null) {}
  Json(bool b) : t_(Bool), b_(b) {}
  Json(int n) : t_(Number), n_(n) {}
  Json(long long n) : t_(Number), n_((double)n) {}
  Json(double n) : t_(Number), n_(n) {}
  Json(const char *s) : t_(String), s_(s) {}
  Json(const std::string &s) : t_(String), s_(s) {}
  static Json array() { Json j; j.t_ = Array; return j; }
  static Json object() { Json j; j.t_ = Object; return j; }

  Type type() const { return t_; }
  bool isNull() const { return t_ == Null; }
  bool isObject() const { return t_ == Object; }
  bool isArray() const { return t_ == Array; }

  bool asBool(bool def = false) const { return t_ == Bool ? b_ : def; }
  double asNumber(double def = 0) const { return t_ == Number ? n_ : def; }
  int asInt(int def = 0) const { return t_ == Number ? (int)n_ : def; }
  long long asInt64(long long def = 0) const { return t_ == Number ? (long long)n_ : def; }
  std::string asString(const std::string &def = "") const { return t_ == String ? s_ : def; }

  // object access (missing key → a shared null)
  const Json &operator[](const char *key) const;
  Json &set(const char *key, Json v);
  bool has(const char *key) const;
  const std::vector<std::pair<std::string, Json>> &items() const { return obj_; }

  // array access
  size_t size() const { return t_ == Array ? arr_.size() : t_ == Object ? obj_.size() : 0; }
  const Json &at(size_t i) const;
  Json &push(Json v);

  std::string dump(int indent = 2) const;
  static bool parse(const std::string &text, Json &out);

 private:
  Type t_;
  bool b_ = false;
  double n_ = 0;
  std::string s_;
  std::vector<Json> arr_;
  std::vector<std::pair<std::string, Json>> obj_;
  void dumpTo(std::string &o, int indent, int depth) const;
};

}  // namespace sd
