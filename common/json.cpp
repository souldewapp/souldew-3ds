#include "json.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace sd {

static const Json kNull;

const Json &Json::operator[](const char *key) const {
  if (t_ != Object) return kNull;
  for (auto &kv : obj_)
    if (kv.first == key) return kv.second;
  return kNull;
}

bool Json::has(const char *key) const {
  if (t_ != Object) return false;
  for (auto &kv : obj_)
    if (kv.first == key) return true;
  return false;
}

Json &Json::set(const char *key, Json v) {
  if (t_ != Object) {
    *this = object();
  }
  for (auto &kv : obj_)
    if (kv.first == key) {
      kv.second = std::move(v);
      return kv.second;
    }
  obj_.emplace_back(key, std::move(v));
  return obj_.back().second;
}

const Json &Json::at(size_t i) const {
  if (t_ == Array && i < arr_.size()) return arr_[i];
  if (t_ == Object && i < obj_.size()) return obj_[i].second;
  return kNull;
}

Json &Json::push(Json v) {
  if (t_ != Array) *this = array();
  arr_.push_back(std::move(v));
  return arr_.back();
}

static void escape(std::string &o, const std::string &s) {
  o += '"';
  for (unsigned char c : s) {
    switch (c) {
      case '"': o += "\\\""; break;
      case '\\': o += "\\\\"; break;
      case '\n': o += "\\n"; break;
      case '\r': o += "\\r"; break;
      case '\t': o += "\\t"; break;
      default:
        if (c < 0x20) {
          char b[8];
          std::snprintf(b, sizeof b, "\\u%04x", c);
          o += b;
        } else o += (char)c;
    }
  }
  o += '"';
}

void Json::dumpTo(std::string &o, int indent, int depth) const {
  auto nl = [&](int d) {
    if (!indent) return;
    o += '\n';
    o.append((size_t)(d * indent), ' ');
  };
  switch (t_) {
    case Null: o += "null"; break;
    case Bool: o += b_ ? "true" : "false"; break;
    case Number: {
      char b[32];
      if (std::fabs(n_ - std::round(n_)) < 1e-9 && std::fabs(n_) < 9e15) std::snprintf(b, sizeof b, "%lld", (long long)std::llround(n_));
      else std::snprintf(b, sizeof b, "%.6g", n_);
      o += b;
      break;
    }
    case String: escape(o, s_); break;
    case Array:
      if (arr_.empty()) { o += "[]"; break; }
      o += '[';
      for (size_t i = 0; i < arr_.size(); i++) {
        if (i) o += ',';
        nl(depth + 1);
        arr_[i].dumpTo(o, indent, depth + 1);
      }
      nl(depth);
      o += ']';
      break;
    case Object:
      if (obj_.empty()) { o += "{}"; break; }
      o += '{';
      for (size_t i = 0; i < obj_.size(); i++) {
        if (i) o += ',';
        nl(depth + 1);
        escape(o, obj_[i].first);
        o += indent ? ": " : ":";
        obj_[i].second.dumpTo(o, indent, depth + 1);
      }
      nl(depth);
      o += '}';
      break;
  }
}

std::string Json::dump(int indent) const {
  std::string o;
  dumpTo(o, indent, 0);
  return o;
}

namespace {
struct Parser {
  const char *p, *end;
  int depth = 0;
  void ws() {
    while (p < end && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')) p++;
  }
  static void utf8(std::string &o, unsigned cp) {
    if (cp < 0x80) o += (char)cp;
    else if (cp < 0x800) { o += (char)(0xC0 | (cp >> 6)); o += (char)(0x80 | (cp & 0x3f)); }
    else if (cp < 0x10000) { o += (char)(0xE0 | (cp >> 12)); o += (char)(0x80 | ((cp >> 6) & 0x3f)); o += (char)(0x80 | (cp & 0x3f)); }
    else { o += (char)(0xF0 | (cp >> 18)); o += (char)(0x80 | ((cp >> 12) & 0x3f)); o += (char)(0x80 | ((cp >> 6) & 0x3f)); o += (char)(0x80 | (cp & 0x3f)); }
  }
  bool str(std::string &o) {
    if (p >= end || *p != '"') return false;
    p++;
    while (p < end && *p != '"') {
      if (*p == '\\') {
        if (++p >= end) return false;
        char c = *p++;
        switch (c) {
          case 'n': o += '\n'; break;
          case 'r': o += '\r'; break;
          case 't': o += '\t'; break;
          case 'b': o += '\b'; break;
          case 'f': o += '\f'; break;
          case 'u': {
            if (end - p < 4) return false;
            char h[5] = {p[0], p[1], p[2], p[3], 0};
            unsigned cp = (unsigned)std::strtoul(h, nullptr, 16);
            p += 4;
            if (cp >= 0xD800 && cp < 0xDC00 && end - p >= 6 && p[0] == '\\' && p[1] == 'u') {
              char h2[5] = {p[2], p[3], p[4], p[5], 0};
              unsigned lo = (unsigned)std::strtoul(h2, nullptr, 16);
              if (lo >= 0xDC00 && lo < 0xE000) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                p += 6;
              }
            }
            utf8(o, cp);
            break;
          }
          default: o += c;
        }
      } else o += *p++;
    }
    if (p >= end) return false;
    p++;
    return true;
  }
  bool value(Json &out) {
    if (++depth > 64) return false;
    ws();
    if (p >= end) return false;
    bool ok = true;
    if (*p == '{') {
      p++;
      out = Json::object();
      ws();
      if (p < end && *p == '}') p++;
      else
        for (;;) {
          ws();
          std::string k;
          if (!str(k)) { ok = false; break; }
          ws();
          if (p >= end || *p != ':') { ok = false; break; }
          p++;
          Json v;
          if (!value(v)) { ok = false; break; }
          out.set(k.c_str(), std::move(v));
          ws();
          if (p < end && *p == ',') { p++; continue; }
          if (p < end && *p == '}') { p++; break; }
          ok = false;
          break;
        }
    } else if (*p == '[') {
      p++;
      out = Json::array();
      ws();
      if (p < end && *p == ']') p++;
      else
        for (;;) {
          Json v;
          if (!value(v)) { ok = false; break; }
          out.push(std::move(v));
          ws();
          if (p < end && *p == ',') { p++; continue; }
          if (p < end && *p == ']') { p++; break; }
          ok = false;
          break;
        }
    } else if (*p == '"') {
      std::string s;
      ok = str(s);
      out = Json(s);
    } else if (end - p >= 4 && !std::strncmp(p, "true", 4)) { p += 4; out = Json(true); }
    else if (end - p >= 5 && !std::strncmp(p, "false", 5)) { p += 5; out = Json(false); }
    else if (end - p >= 4 && !std::strncmp(p, "null", 4)) { p += 4; out = Json(); }
    else {
      char *e = nullptr;
      std::string num;
      const char *q = p;
      while (q < end && (std::strchr("+-.eE", *q) || (*q >= '0' && *q <= '9'))) num += *q++;
      double d = std::strtod(num.c_str(), &e);
      if (num.empty() || (size_t)(e - num.c_str()) != num.size()) ok = false;
      p = q;
      out = Json(d);
    }
    depth--;
    return ok;
  }
};
}  // namespace

bool Json::parse(const std::string &text, Json &out) {
  Parser ps{text.data(), text.data() + text.size()};
  if (!ps.value(out)) return false;
  ps.ws();
  return ps.p == ps.end;
}

}  // namespace sd
