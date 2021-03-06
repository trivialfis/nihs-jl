#ifndef JSON_HH_
#define JSON_HH_

#define LOG std::cout << __FILE__ << ", " << __LINE__ << ": "
#define L(CONTENT)                                      \
  std::cout << __FILE__ << ", " << __LINE__ << ": "     \
  << CONTENT << '|' << std::endl;                       \

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
 public:
  /*!\brief Simplified implementation of LLVM RTTI. */
  enum class ValueKind {
    String,
    Number,
    Object,  // std::map
    Array,   // std::vector, std::list, std::array
    Boolean,
    Null
  };

  Value(ValueKind _kind) : kind_{_kind} {}

  ValueKind Type() const { return kind_; }
  virtual ~Value() = default;

  virtual void Save(JsonWriter* stream) = 0;

  virtual Json& operator[](std::string const & key) = 0;
  virtual Json& operator[](int ind) = 0;

  virtual bool operator==(Value const& rhs) const = 0;
  virtual Value& operator=(Value const& rhs) = 0;

  std::string TypeStr() const;

 private:
  ValueKind kind_;
};

template <typename T>
bool IsA(Value const* value) {
  return T::IsClassOf(value);
}

template <typename T, typename U>
T* Cast(U* value) {
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

  virtual bool operator==(Value const& rhs) const;
  virtual Value& operator=(Value const& rhs);

  static bool IsClassOf(Value const* value) {
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

  virtual bool operator==(Value const& rhs) const;
  virtual Value& operator=(Value const& rhs);

  static bool IsClassOf(Value const* value) {
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

  virtual bool operator==(Value const& rhs) const;
  virtual Value& operator=(Value const& rhs);

  static bool IsClassOf(Value const* value) {
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

  virtual void Save(JsonWriter* stream);

  virtual Json& operator[](std::string const & key);
  virtual Json& operator[](int ind);

  double GetNumber() const { return number_; }

  virtual bool operator==(Value const& rhs) const;
  virtual Value& operator=(Value const& rhs);

  static bool IsClassOf(Value const* value) {
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

  virtual bool operator==(Value const& rhs) const;
  virtual Value& operator=(Value const& rhs);

  static bool IsClassOf(Value const* value) {
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

  bool GetBoolean() const { return boolean_; }

  virtual bool operator==(Value const& rhs) const;
  virtual Value& operator=(Value const& rhs);

  static bool IsClassOf(Value const* value) {
    return value->Type() == ValueKind::Boolean;
  }
};

/*!
 * \brief Data structure representing JSON format.
 *
 * Limitation:  UTF-8 is not properly supported.  Code points above ASCII are
 *              invalid.
 *
 * Examples:
 *
 * \code
 *   // Create a JSON object.
 *   json::Json object = json::Object();
 *   // Assign key "key" with a JSON string "Value";
 *   object["key"] = Json::String("Value");
 *   // Assign key "arr" with a empty JSON Array;
 *   object["arr"] = Json::Array();
 * \endcode
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
  Json& operator=(Json const& other);
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
  Value const& GetValue() const {return *ptr_;}

  bool operator==(Json const& rhs) const {
    return *ptr_ == *(rhs.ptr_);
  }

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
template <typename T, typename U>
T Get(U json) {
  auto value = *Cast<T>(&json.GetValue());
  return value;
}

using Object = JsonObject;
using Array = JsonArray;
using Number = JsonNumber;
using Boolean = JsonBoolean;
using String = JsonString;
using Null = JsonNull;

}      // namespace json
#endif  // JSON_HH_