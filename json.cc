#include "json.hh"

namespace json {

class JsonWriter {
  static constexpr size_t kIndentSize = 2;

  size_t n_spaces_;
  std::ostream* stream_;

 public:
  JsonWriter(std::ostream* stream) : n_spaces_{0}, stream_{stream} {}
  ~JsonWriter() = default;

  void NewLine() {
    *stream_ << "\n" << std::string(n_spaces_, ' ');
  }

  void BeginIndent() {
    n_spaces_ += kIndentSize;
  }
  void EndIndent() {
    n_spaces_ -= kIndentSize;
    CHECK_GE(n_spaces_, 0);
  }

  void Write(std::string str) {
    *stream_ << str;
  }

  void Save(Json json) {
    json.ptr_->Save(this);
  }
};

class JsonReader {
 private:
  struct SourceLocation {
    int cl_;      // current line
    int cc_;      // current column
    size_t pos_;  // current position in raw_str_

   public:
    SourceLocation() : cl_(0), cc_(0), pos_(0) {}

    int Line() const { return cl_;  }
    int Col()  const { return cc_;  }
    size_t Pos()  const { return pos_; }

    SourceLocation& Forward(char c=0) {
      if (c == '\n') {
        cc_ = 0;
        cl_++;
      } else {
        cc_++;
      }
      pos_++;
      return *this;
    }
  } cursor_;

  std::string raw_str_;

  void SkipSpaces();

  char GetNextChar() {
    if (cursor_.Pos() == raw_str_.size()) {
      return -1;
    }
    char ch = raw_str_[cursor_.Pos()];
    cursor_.Forward();
    return ch;
  }

  char PeekNextChar() {
    if (cursor_.Pos() == raw_str_.size()) {
      return -1;
    }
    char ch = raw_str_[cursor_.Pos()];
    return ch;
  }

  char GetNextNonSpaceChar() {
    SkipSpaces();
    return GetNextChar();
  }

  char GetChar(char c) {
    char result = GetNextNonSpaceChar();
    if (result != c) { Expect(c); }
    return result;
  }

  Json Error() const {
    std::istringstream str_s(raw_str_);

    std::string line;
    int line_count = 0;
    while (std::getline(str_s, line) && line_count < cursor_.Line()) {
      line_count++;
    }
    std::cerr << line << std::endl;
    std::string spaces (cursor_.Col(), ' ');
    std::cerr << spaces << '^' << std::endl;
    return Json();
  }

  void Error(std::string msg) const {
    std::istringstream str_s(raw_str_);

    msg += "\nAt ("
           + std::to_string(cursor_.Line()) + ", "
           + std::to_string(cursor_.Col()) + ")\n";
    std::string line;
    int line_count = 0;
    while (std::getline(str_s, line) && line_count < cursor_.Line()) {
      line_count++;
    }
    msg+= line += '\n';
    std::string spaces (cursor_.Col(), ' ');
    msg+= spaces + "^\n";

    throw std::runtime_error(msg);
  }

  // Report expected character
  void Expect(char c) {
    std::string msg = "Expecting: \"";
    msg += std::to_string(c)
           + "\", got: \"" + raw_str_[cursor_.Pos()-1] + "\"\n"; // FIXME
    Error(msg);
  }

  // FIXME: Maybe remove this.
  // Report expected set of possibile characters
  void Expect(std::vector<char> expectations) {
    std::stringstream strstream;
    strstream << "\nAt ("
              << cursor_.Line() << ", "
              << cursor_.Col() << ")";
    strstream << ", Expecting: ";
    for (auto c : expectations) {
      strstream << '\"' << c << "\" or ";
    }
    strstream << "\", got: \"" << raw_str_[cursor_.Pos()] << "\""
              << std::endl;
    Error(strstream.str());
  }

  Json ParseString();
  Json ParseObject();
  Json ParseArray();
  Json ParseNumber();
  Json ParseBoolean();

  Json Parse() {
    while (true) {
      SkipSpaces();
      char c = PeekNextChar();
      if (c == -1) { break; }

      if (c == '{') {
        return ParseObject();
      } else if ( c == '[' ) {
        return ParseArray();
      } else if ( c == '-' || std::isdigit(c)) {
        return ParseNumber();
      } else if ( c == '\"' ) {
        return ParseString();
      } else if ( c == 't' || c == 'f') {
        return ParseBoolean();
      } else {
        Error("Unknow construct.");
      }
    }
    return Json();
  }

 public:
  JsonReader() = default;
  ~JsonReader() = default;

  Json Load(std::istream* stream) {
    raw_str_ = {std::istreambuf_iterator<char>(*stream), {}};

    return Parse();
  }
};

// Value
std::string Value::TypeStr() const {
  switch (kind_) {
    case ValueKind::String: return "String";  break;
    case ValueKind::Number: return "Number";  break;
    case ValueKind::Object: return "Object";  break;
    case ValueKind::Array:  return "Array";   break;
    case ValueKind::Boolean:return "Boolean"; break;
    case ValueKind::Null:   return "Null";    break;
  }
  return "";
}

// Json Object
JsonObject::JsonObject(std::map<std::string, Json> object)
    : Value(ValueKind::Object), object_{std::move(object)} {}

Json& JsonObject::operator[](std::string const & key) {
  return object_[key];
}

// Only used for keeping old compilers happy about non-reaching return
// statement.
Json& DummyJsonObject () {
  static Json obj;
  return obj;
}

Json& JsonObject::operator[](int ind) {
  throw std::runtime_error(
      "Object of type " +
      Value::TypeStr() + " can not be indexed by Integer.");
  return DummyJsonObject();
}

void JsonObject::Save(JsonWriter* writer) {
  writer->Write("{");
  writer->BeginIndent();
  writer->NewLine();

  size_t i = 0;
  size_t size = object_.size();

  for (auto& value : object_) {
    writer->Write("\"" + value.first + "\": ");
    writer->Save(value.second);

    if (i != size-1) {
      writer->Write(",");
      writer->NewLine();
    }
    i++;
  }
  writer->EndIndent();
  writer->NewLine();
  writer->Write("}");
}

// Json String
Json& JsonString::operator[](std::string const & key) {
  throw std::runtime_error(
      "Object of type " +
      Value::TypeStr() + " can not be indexed by string.");
  return DummyJsonObject();
}

Json& JsonString::operator[](int ind) {
  throw std::runtime_error(
      "Object of type " +
      Value::TypeStr() + " can not be indexed by Integer, " +
      "please try obtaining std::string first.");
  return DummyJsonObject();
}

void JsonString::Save(JsonWriter* writer) {
  std::string buffer;
  buffer += '"';
  for (size_t i = 0; i < str_.length(); i++) {
    const char ch = str_[i];
    if (ch == '\\') {
      buffer += "\\\\";
    } else if (ch == '"') {
      buffer += "\\\"";
    } else if (ch == '\b') {
      buffer += "\\b";
    } else if (ch == '\f') {
      buffer += "\\f";
    } else if (ch == '\n') {
      buffer += "\\n";
    } else if (ch == '\r') {
      buffer += "\\r";
    } else if (ch == '\t') {
      buffer += "\\t";
    } else if (static_cast<uint8_t>(ch) <= 0x1f) {
      char buf[8];
      snprintf(buf, sizeof buf, "\\u%04x", ch);
      buffer += buf;
    } else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(str_[i+1]) == 0x80
               && static_cast<uint8_t>(str_[i+2]) == 0xa8) {
      buffer += "\\u2028";
      i += 2;
    } else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(str_[i+1]) == 0x80
               && static_cast<uint8_t>(str_[i+2]) == 0xa9) {
      buffer += "\\u2029";
      i += 2;
    } else {
      buffer += ch;
    }
  }
  buffer += '"';
  writer->Write(buffer);
}

// Json Array
Json& JsonArray::operator[](std::string const & key) {
  throw std::runtime_error(
      "Object of type " +
      Value::TypeStr() + " can not be indexed by string.");
  return DummyJsonObject();
}

Json& JsonArray::operator[](int ind) {
  return vec_.at(ind);
}

void JsonArray::Save(JsonWriter* writer) {
  writer->Write("[");
  size_t size = vec_.size();
  for (size_t i = 0; i < size; ++i) {
    auto& value = vec_[i];
    writer->Save(value);
    if (i != size-1) { writer->Write(", "); }
  }
  writer->Write("]");
}

// Json Number
Json& JsonNumber::operator[](std::string const & key) {
  throw std::runtime_error(
      "Object of type " +
      Value::TypeStr() + " can not be indexed by string.");
  return DummyJsonObject();
}

Json& JsonNumber::operator[](int ind) {
  throw std::runtime_error(
      "Object of type " +
      Value::TypeStr() + " can not be indexed by Integer.");
  return DummyJsonObject();
}

void JsonNumber::Save(JsonWriter* writer) {
  writer->Write(std::to_string(this->GetDouble()));
}

// Json Null
Json& JsonNull::operator[](std::string const & key) {
  throw std::runtime_error(
      "Object of type " +
      Value::TypeStr() + " can not be indexed by string.");
  return DummyJsonObject();
}

Json& JsonNull::operator[](int ind) {
  throw std::runtime_error(
      "Object of type " +
      Value::TypeStr() + " can not be indexed by Integer.");
  return DummyJsonObject();
}

void JsonNull::Save(JsonWriter* writer) {
  writer->Write("null");
}

// Json Boolean
Json& JsonBoolean::operator[](std::string const & key) {
  throw std::runtime_error(
      "Object of type " +
      Value::TypeStr() + " can not be indexed by string.");
  return DummyJsonObject();
}
Json& JsonBoolean::operator[](int ind) {
  throw std::runtime_error(
      "Object of type " +
      Value::TypeStr() + " can not be indexed by Integer.");
  return DummyJsonObject();
}

void JsonBoolean::Save(JsonWriter *writer) {
  if (boolean_) {
    writer->Write("true");
  } else {
    writer->Write("false");
  }
}

// Json class
void JsonReader::SkipSpaces() {
  while (cursor_.Pos() < raw_str_.size()) {
    char c = raw_str_[cursor_.Pos()];
    if (std::isspace(c)) {
      cursor_.Forward(c);
    } else {
      break;
    }
  }
}

Json JsonReader::ParseString() {
  char ch = GetChar('\"');
  std::ostringstream output;
  std::string str;
  while (true) {
    ch = GetNextChar();
    if (ch == '\\') {
      char next = static_cast<char>(GetNextChar());
      switch (next) {
        case 'r': str += "\r"; break;
        case 'n': str += "\n"; break;
        case '\\': str += "\\"; break;
        case 't': str += "\t"; break;
        case '\"': str += "\"";; break;
        default: Error();
      }
    } else {
      if (ch == '\"') break;
      str += ch;
    }
    if (ch == EOF || ch == '\r' || ch == '\n') {
      Expect('\"');
    }
  }
  return Json(std::move(str));
}

Json JsonReader::ParseArray() {
  std::vector<Json> data;

  char ch = GetChar('[');
  while (true) {
    if (PeekNextChar() == ']') {
      GetChar(']');
      return Json(std::move(data));
    }
    auto obj = Parse();
    data.push_back(obj);
    ch = GetNextNonSpaceChar();
    if (ch == ']') break;
    if (ch != ',') {
      Expect(',');
    }
  }

  return Json(std::move(data));
}

Json JsonReader::ParseObject() {
  char ch = GetChar('{');

  std::map<std::string, Json> data;
  if (ch == '}') return Json(std::move(data));

  while(true) {
    SkipSpaces();
    ch = PeekNextChar();
    if (ch != '"') {
      Expect('"');
    }
    Json key = ParseString();

    ch = GetNextNonSpaceChar();

    if (ch != ':') {
      Expect(':');
    }

    Json value {Parse()};
    data[Cast<JsonString>(&(key.GetValue()))->GetString()] = std::move(value);

    ch = GetNextNonSpaceChar();

    if (ch == '}') break;
    if (ch != ',') {
      Expect(',');
    }
  }

  return Json(std::move(data));
}

Json JsonReader::ParseNumber() {
  std::string substr = raw_str_.substr(cursor_.Pos(), 17);
  size_t pos = 0;
  double number = std::stod(substr, &pos);
  for (size_t i = 0; i < pos; ++i) {
    GetNextChar();
  }
  return Json(number);
}

Json JsonReader::ParseBoolean() {
  bool result = false;
  char ch = GetNextNonSpaceChar();
  std::string const t_value = u8"true";
  std::string const f_value = u8"false";
  std::string buffer;

  if (ch == 't') {
    for (size_t i = 0; i < 3; ++i) {
      buffer.push_back(GetNextNonSpaceChar());
    }
    if (buffer != u8"rue") {
      Error("Expecting boolean value \"true\".");
    }
    result = true;
  } else {
    for (size_t i = 0; i < 4; ++i) {
      buffer.push_back(GetNextNonSpaceChar());
    }
    if (buffer != u8"alse") {
      Error("Expecting boolean value \"false\".");
    }
    result = false;
  }
  return Json{JsonBoolean{result}};
}

Json Json::Load(std::istream* stream) {
  JsonReader reader;
  try {
    Json json{reader.Load(stream)};
    return json;
  } catch (std::runtime_error const& e) {
    std::cerr << e.what();
    return Json();
  }
}

void Json::Dump(Json json, std::ostream *stream) {
  JsonWriter writer(stream);
  try {
    writer.Save(json);
  } catch (std::runtime_error const& e) {
    std::cerr << e.what();
  }
}

}  // namespace json