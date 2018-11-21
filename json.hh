#ifndef JSON_HH_
#define JSON_HH_

#define LOG std::cout << __FILE__ << ", " << __LINE__ << ": "
#define L(CONTENT)                                      \
  std::cout << __FILE__ << ", " << __LINE__ << ": "     \
  << CONTENT << std::endl;

/*!
 * \brief Unicode
 *
 * [ U+005B left square bracket
 * { U+007B left curly bracket
 * ] U+005D right square bracket
 * } U+007D right curly bracket
 * : U+003A colon
 * , U+002C comma
 *
 * true  U+0074 U+0072 U+0075 U+0065
 * false U+0066 U+0061 U+006C U+0073 U+0065
 * null  U+006E U+0075 U+006C U+006C
 */
#include <iostream>
#include <istream>
#include <string>
#include <sstream>

#include <map>
#include <memory>
#include <vector>

namespace json {

class DebugFunction {
  std::string f;
 public:
  DebugFunction(std::string func) : f(func){
    std::cout << "start " << func << std::endl;;
  }
  ~DebugFunction() {
    std::cout << "end   " << f << std::endl;;
  }
};

#define DEBUG_F auto __debug_f__ = DebugFunction(__PRETTY_FUNCTION__);
#define CHECK_GE(a, b)                                  \
  if (!((a) >= (b))) {                                  \
    throw std::runtime_error("CHECK_GE failed");        \
  }                                                     \

class Json;
namespace {
class JsonWriter;
};  // namespace

class Value {
 protected:
  enum class ValueKind {
    String,
    Number,
    Object,  // std::map
    Array,   // std::vector, std::list, std::array
    True,
    False,
    Null
  };

 private:
  ValueKind kind_;

 public:
  Value(ValueKind _kind) : kind_{_kind} {}
  ValueKind Type() const { return kind_; }
  virtual ~Value() = default;

  virtual void Save(JsonWriter* stream) = 0;

  virtual Json operator[](std::string const & key) = 0;
  virtual Json operator[](int ind) = 0;

  std::string TypeStr() const;
};

template <typename T>
bool IsA(Value* value) {
  return T::IsClassOf(value);
}

template <typename T>
T* Cast(Value* value) {
  if (IsA<T>(value)) {
    return dynamic_cast<T*>(value);
  } else {
    throw std::runtime_error(
        "Invalid cast, value is a " + value->TypeStr());
  }
}

class JsonString : public Value {
  std::string str_;
 public:
  JsonString() : Value(ValueKind::String) {}
  JsonString(std::string&& str) :
      Value(ValueKind::String), str_{std::move(str)} {}

  virtual void Save(JsonWriter* stream);

  virtual Json operator[](std::string const & key);
  virtual Json operator[](int ind);

  std::string const& GetString() const { return str_; }
  std::string & GetString() { return str_;}

  static bool IsClassOf(Value* value) {
    return value->Type() == ValueKind::String;
  }
};

class JsonArray : public Value {
  std::vector<Json> vec_;
 public:
  JsonArray() : Value(ValueKind::Array) {}
  JsonArray(std::vector<Json>&& arr) :
      Value(ValueKind::Array), vec_{std::move(arr)} {}

  virtual void Save(JsonWriter* stream);

  virtual Json operator[](std::string const & key);
  virtual Json operator[](int ind);

  std::vector<Json> const& GetArray() const { return vec_; }
  std::vector<Json> & GetArray() { return vec_; }

  static bool IsClassOf(Value* value) {
    return value->Type() == ValueKind::Array;
  }
};

class JsonObject : public Value {
  std::map<std::string, Json> object_;

 public:
  JsonObject() : Value(ValueKind::Object) {}
  JsonObject(std::map<std::string, Json> object);

  virtual void Save(JsonWriter* writer);

  virtual Json operator[](std::string const & key);
  virtual Json operator[](int ind);

  static bool IsClassOf(Value* value) {
    return value->Type() == ValueKind::Object;
  }
};

class JsonNumber : public Value {
  double number_;
 public:
  JsonNumber() : Value(ValueKind::Number) {}
  JsonNumber(double value) : Value(ValueKind::Number) {
    number_ = value;
  }

  double GetDouble() const { return number_; }

  virtual void Save(JsonWriter* stream);

  virtual Json operator[](std::string const & key);
  virtual Json operator[](int ind);

  static bool IsClassOf(Value* value) {
    return value->Type() == ValueKind::Number;
  }
};

class JsonNull : public Value {
 public:
  JsonNull() : Value(ValueKind::Null) {}
  JsonNull(std::nullptr_t) : Value(ValueKind::Null) {}

  virtual void Save(JsonWriter* stream);

  virtual Json operator[](std::string const & key);
  virtual Json operator[](int ind);

  static bool IsClassOf(Value* value) {
    return value->Type() == ValueKind::Null;
  }
};

class Json {
  friend JsonWriter;
  void Save(JsonWriter* writer) {
    this->ptr_->Save(writer);
  }

 public:
  static Json Load(std::istream* stream);
  static void Dump(Json json, std::ostream* stream);

  Json() : ptr_{new JsonNull} {}
  explicit Json(std::map<std::string, Json>&& object) :
      ptr_{new JsonObject(std::move(object))} {}
  explicit Json(std::vector<Json>&& list) :
      ptr_{new JsonArray(std::move(list))}{}
  explicit Json(std::string&& str) :
      ptr_(new JsonString(std::move(str))) {}
  explicit Json(double d) : ptr_{new JsonNumber(d)} {}
  explicit Json(float f)  : ptr_{new JsonNumber(f)} {}
  explicit Json(int i)    : ptr_{new JsonNumber(i)} {}

  Json(Json const& other) : ptr_{other.ptr_} {}
  Json(Json&& other) : ptr_{std::move(other.ptr_)} {}

  Json& operator=(Json const& other) {
    ptr_ = other.ptr_;
    return *this;
  }

  Json& operator=(Json&& other) {
    ptr_ = std::move(other.ptr_);
    return *this;
  }

  Json operator[](std::string const & key) {
    return (*ptr_)[key];
  }

  Value& GetValue() {
    return *ptr_;
  }

 private:
  std::shared_ptr<Value> ptr_;
};
}      // namespace json
#endif  // JSON_HH_