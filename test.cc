#include "json.hh"

#include <fstream>

using namespace json;

#define TEST(name)                                              \
  {                                                             \
    std::cout   << "Run:  " << # name << std::endl;             \
    bool result = (name)();                                     \
    if (result) {                                               \
      std::cout << "Pass: " << # name << "\n" << std::endl;     \
    }                                                           \
  }                                                             \

bool TestParseObject() {
  std::string str = "{\"TreeParam\" : {\"num_feature\": \"10\"}}";
  std::istringstream iss(str);
  auto json = json::Json::Load(&iss);
  return true;
}

bool TestParseNumber() {
  std::string str = "31.8892";
  std::istringstream iss(str);
  json::Json::Load(&iss);
  return true;
}

bool TestParseArray() {
  std::string str = R"json(
{
    "nodes": [
        {
	    "depth": 3,
	    "gain": 10.4866,
	    "hess": 7,
	    "left": 3,
	    "missing": 3,
	    "nodeid": 1,
	    "right": 4,
	    "split_condition": 0.238748,
	    "split_index": 1
        },
        {
	    "hess": 6,
	    "leaf": 1.54286,
	    "nodeid": 4
        },
        {
	    "hess": 1,
	    "leaf": 0.225,
	    "nodeid": 3
        }
    ]
}
)json";
  std::istringstream iss(str);
  json::Json::Load(&iss);
  return true;
}

bool TestEmptyArray() {
  std::string str = R"json(
{
  "leaf_vector": []
}
)json";
  std::istringstream iss(str);
  json::Json::Load(&iss);
  return true;
}

bool TestIndexing() {
  std::ifstream fin ("/home/fis/Workspace/json/model.json");
  Json j {json::Json::Load(&fin)};

  auto value_1 = j["model_parameter"];
  auto value = value_1["base_score"];
  std::string result = Cast<JsonString>(&value.GetValue())->GetString();
  return result == "0.5";
}

int main(int argc, char * const argv[]) {
  TEST(TestParseObject);
  TEST(TestParseNumber);
  TEST(TestParseArray);
  TEST(TestEmptyArray);

  TEST(TestIndexing);
}
