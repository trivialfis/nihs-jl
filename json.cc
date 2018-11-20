#include "json.hh"

namespace json {

class JsonWriter {
 public:
  JsonWriter() = default;
  ~JsonWriter() = default;

  void Save(Json json, std::istream *stream) {
    
  }
};

class JsonReader {
 private:
  struct SourceLocation {
    int cl_;  // current line
    int cc_;  // current column
    int pos_; // current position in raw_str_

   public:
    SourceLocation() : cl_(0), cc_(0), pos_(0) {}

    int Line() const { return cl_;  }
    int Col()  const { return cc_;  }
    int Pos()  const { return pos_; }

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

  void Expect(char c) {
    std::string msg = "\nAt ("
                      + std::to_string(cursor_.Line()) + ", "
                      + std::to_string(cursor_.Col()) + ")"
                      + ", Expecting: \"" + c
                      + "\", got: \"" + raw_str_[cursor_.Pos()-1] + "\"\n"; // FIXME
    Error(msg);
  }

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

  Json Parse() {
    while (true) {
      SkipSpaces();
      char c = PeekNextChar();
      if (c == -1) { break; }

      if (c == '{') {
        return std::move(ParseObject());
      } else if ( c == '[' ) {
        return std::move(ParseArray());
      } else if ( c == '-' || std::isdigit(c)) {
        return std::move(ParseNumber());
      } else if ( c == '\"' ) {
        return std::move(ParseString());
      } else {
        Error("Unknow construct.");
      }
    }
    return std::move(Json());
  }

 public:
  JsonReader() = default;
  ~JsonReader() = default;

  Json Load(std::istream* stream) {
    raw_str_ = {std::istreambuf_iterator<char>(*stream), {}};

    return Parse();
  }
};
// Json Object
JsonObject::JsonObject(std::map<std::string, Json> object)
    : Value(ValueKind::Object), object_{std::move(object)} {}

Json JsonObject::operator[](std::string const & key) {
  return object_.at(key);
}

Json& CreateNull() {
  static Json null;
  return null;
}

Json JsonObject::operator[](int ind) {
  throw std::runtime_error(
      "Object of type Json Object can not be indexed by Integer.");
  return CreateNull();
}

// Json String
Json JsonString::operator[](std::string const & key) {
  throw std::runtime_error(
      "Object of type Json String can not be indexed by string.");
  return CreateNull();
}

Json JsonString::operator[](int ind) {
  return Json(std::move(std::to_string(str_[ind])));
}

// Json Array
Json JsonArray::operator[](std::string const & key) {
  throw std::runtime_error(
      "Object of type Json Array can not be indexed by string.");
  return CreateNull();
}

Json JsonArray::operator[](int ind) {
  // Json res {std::move()};
  return Json(vec_.at(ind));
}

// Json Number
Json JsonNumber::operator[](std::string const & key) {
  throw std::runtime_error(
      "Object of type Json Number can not be indexed by string.");
  return CreateNull();
}

Json JsonNumber::operator[](int ind) {
  throw std::runtime_error(
      "Object of type Json Number can not be indexed by Integer.");
  return CreateNull();
}

// Json Null
Json JsonNull::operator[](std::string const & key) {
  throw std::runtime_error(
      "Object of type Json Null can not be indexed by string.");
  return CreateNull();
}

Json JsonNull::operator[](int ind) {
  throw std::runtime_error(
      "Object of type Json Null can not be indexed by Integer.");
  return CreateNull();
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
      char sch = static_cast<char>(GetNextChar());
      switch (sch) {
        case 'r': str += "\r"; break;
        case 'n': str += "\n"; break;
        case '\\': str += "\\"; break;
        case 't': str += "\t"; break;
        case '\"': str += "\"";; break;
        default:
          Error();
      }
    } else {
      if (ch == '\"') break;
      str += ch;
    }
    if (ch == EOF || ch == '\r' || ch == '\n') {
      Expect('\"');
    }
  }
  return std::move(Json(std::move(str)));
}

Json JsonReader::ParseArray() {
  std::vector<Json> data;

  char ch = GetChar('[');
  while (true) {
    if (PeekNextChar() == ']') {
      GetChar(']');
      return std::move(Json(std::move(data)));
    }
    auto obj = Parse();
    // L("obj.Type: " << obj.GetValue().TypeStr());
    data.push_back(obj);
    ch = GetNextNonSpaceChar();
    if (ch == ']') break;
    if (ch != ',') {
      Expect(',');
    }
  }

  return std::move(Json(std::move(data)));
}

Json JsonReader::ParseObject() {
  char ch = GetChar('{');

  std::map<std::string, Json> data;
  if (ch == '}') return std::move(Json(std::move(data)));

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

    Json value {std::move(Parse())};
    data[Cast<JsonString>(&(key.GetValue()))->GetString()] = std::move(value);

    ch = GetNextNonSpaceChar();

    if (ch == '}') break;
    if (ch != ',') {
      Expect(',');
    }
  }

  return std::move(Json(std::move(data)));
}

Json JsonReader::ParseNumber() {
  std::string substr = raw_str_.substr(cursor_.Pos(), 17);
  size_t pos = 0;
  double number = std::stod(substr, &pos);
  for (size_t i = 0; i < pos; ++i) {
    GetNextChar();
  }
  return std::move(Json(number));
}

Json Json::Load(std::istream* stream) {
  JsonReader reader;
  try {
    Json json{std::move(reader.Load(stream))};
    return std::move(json);
  } catch (std::runtime_error const& e) {
    std::cerr << e.what();
    return std::move(Json());
  }
}

void Json::Save(std::istream *stream) {
  throw std::runtime_error("Not implemented");
}

}  // namespace json
