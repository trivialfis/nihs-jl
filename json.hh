#ifndef JSON_HH_
#define JSON_HH_

#define LOG std::cout << __FILE__ << ", " << __LINE__ << ": "
#define L(CONTENT)                                      \
  std::cout << __FILE__ << ", " << __LINE__ << ": "     \
  << CONTENT << '|' << std::endl;                       \

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
class JsonWriter;

class Value {
 protected:
  /*!\brief Simplified implementation of LLVM RTTI. */
  enum class ValueKind {
    String,
    Number,
    Object,  // std::map
    Array,   // std::vector, std::list, std::array
    Boolean,
    Null
  };

 private:
  ValueKind kind_;

 public:
  Value(ValueKind _kind) : kind_{_kind} {}

  ValueKind Type() const { return kind_; }
  virtual ~Value() = default;

  virtual void Save(JsonWriter* stream) = 0;

  virtual Json& operator[](std::string const & key) = 0;
  virtual Json& operator[](int ind) = 0;

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
        "Invalid cast, from " + value->TypeStr() + " to " + T().TypeStr());
  }
}

class JsonString : public Value {
  std::string str_;
 public:
  JsonString() : Value(ValueKind::String) {}
  JsonString(std::string const& str) :
      Value(ValueKind::String), str_{str} {}
  JsonString(std::string&& str) :
      Value(ValueKind::String), str_{std::move(str)} {}

  virtual void Save(JsonWriter* stream);

  virtual Json& operator[](std::string const & key);
  virtual Json& operator[](int ind);

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
  JsonArray(std::vector<Json> const& arr) :
      Value(ValueKind::Array), vec_{arr} {}

  virtual void Save(JsonWriter* stream);

  virtual Json& operator[](std::string const & key);
  virtual Json& operator[](int ind);

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

  virtual Json& operator[](std::string const & key);
  virtual Json& operator[](int ind);

  std::map<std::string, Json> const& GetObject() const { return object_; }
  std::map<std::string, Json> &      GetObject() { return object_; }

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

  virtual Json& operator[](std::string const & key);
  virtual Json& operator[](int ind);

  double GetNumber() const { return number_; }

  static bool IsClassOf(Value* value) {
    return value->Type() == ValueKind::Number;
  }
};

class JsonNull : public Value {
 public:
  JsonNull() : Value(ValueKind::Null) {}
  JsonNull(std::nullptr_t) : Value(ValueKind::Null) {}

  virtual void Save(JsonWriter* stream);

  virtual Json& operator[](std::string const & key);
  virtual Json& operator[](int ind);

  static bool IsClassOf(Value* value) {
    return value->Type() == ValueKind::Null;
  }
};

/*! \brief Describes both true and false. */
class JsonBoolean : public Value {
  bool boolean_;
 public:
  JsonBoolean() : Value(ValueKind::Boolean) {}
  // Ambigious with JsonNumber.
  template <typename Bool,
            typename std::enable_if<
              std::is_same<Bool, bool>::value ||
              std::is_same<Bool, bool const>::value>::type* = nullptr>
  JsonBoolean(Bool value) :
      Value(ValueKind::Boolean), boolean_{value} {}

  virtual void Save(JsonWriter* writer);

  virtual Json& operator[](std::string const & key);
  virtual Json& operator[](int ind);

  bool GetBoolean() { return boolean_; }

  static bool IsClassOf(Value* value) {
    return value->Type() == ValueKind::Boolean;
  }
};

/*!
 * \brief Data structure representing JSON format.
 */
class Json {
  friend JsonWriter;
  void Save(JsonWriter* writer) {
    this->ptr_->Save(writer);
  }

 public:
  /*! \brief Load a Json file from stream. */
  static Json Load(std::istream* stream);
  /*! \brief Dump json into stream. */
  static void Dump(Json json, std::ostream* stream);

  Json() : ptr_{new JsonNull} {}

  // number
  explicit Json(JsonNumber number) : ptr_{new JsonNumber(number)} {}
  Json& operator=(JsonNumber number) {
    ptr_.reset(new JsonNumber(std::move(number)));
    return *this;
  }
  // array
  explicit Json(JsonArray list) :
      ptr_{new JsonArray(std::move(list))}{}
  Json& operator=(JsonArray array) {
    ptr_.reset(new JsonArray(std::move(array)));
    return *this;
  }
  // object
  explicit Json(JsonObject object) :
      ptr_{new JsonObject(std::move(object))} {}
  Json& operator=(JsonObject object) {
    ptr_.reset(new JsonObject(std::move(object)));
    return *this;
  }
  // string
  explicit Json(JsonString str) :
      ptr_{new JsonString(std::move(str))} {}
  Json& operator=(JsonString str) {
    ptr_.reset(new JsonString(std::move(str)));
    return *this;
  }
  // bool
  explicit Json(JsonBoolean boolean) :
      ptr_{new JsonBoolean(std::move(boolean))} {}
  Json& operator=(JsonBoolean boolean) {
    ptr_.reset(new JsonBoolean(std::move(boolean)));
    return *this;
  }
  // null
  explicit Json(JsonNull null) :
      ptr_{new JsonNull(std::move(null))} {}
  Json& operator=(JsonNull null) {
    ptr_.reset(new JsonNull(std::move(null)));
    return *this;
  }

  // copy
  Json(Json const& other) : ptr_{other.ptr_} {}
  Json& operator=(Json const& other) {
    ptr_ = other.ptr_;
    return *this;
  }
  // move
  Json(Json&& other) : ptr_{std::move(other.ptr_)} {}
  Json& operator=(Json&& other) {
    ptr_ = std::move(other.ptr_);
    return *this;
  }

  /*! \brief Index Json object with a std::string, used for Json Object. */
  Json& operator[](std::string const & key) const { return (*ptr_)[key]; }
  /*! \brief Index Json object with int, used for Json Array. */
  Json& operator[](int ind)                 const { return (*ptr_)[ind]; }

  /*! \Brief Return the reference to stored Json value. */
  Value& GetValue() { return *ptr_; }

 private:
  std::shared_ptr<Value> ptr_;
};

/*!
 * \brief Get Json value.
 *
 * \tparam T One of the Json value type.
 *
 * \param json
 * \return Json value with type T.
 */
template <typename T>
T Get(Json json) {
  auto value = *Cast<T>(&json.GetValue());
  return value;
}
}      // namespace json
#endif  // JSON_HH_