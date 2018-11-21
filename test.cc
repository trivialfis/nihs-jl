#include "json.hh"

#include <fstream>

using namespace json;

#define TEST(name)                                              \
  {                                                             \
    std::cout   << "Run:  " << # name << std::endl;             \
    bool result = (name)();                                     \
    if (result) {                                               \
      std::cout << "Pass: " << # name << "\n" << std::endl;     \
    } else {                                                    \
      std::cout << "Failed: " << # name << "\n" << std::endl;   \
    }                                                           \
  }

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

  fin.close();
  return result == "0.5";
}

bool TestLoadDump() {
  std::string model_json = R"json(
{
  "model_parameter": {
    "base_score": "0.5",
    "num_class": "0",
    "num_feature": "10"
  },
  "train_parameter": {
    "debug_verbose": "0",
    "disable_default_eval_metric": "0",
    "dsplit": "auto",
    "nthread": "0",
    "seed": "0",
    "seed_per_iteration": "0",
    "test_flag": "",
    "tree_method": "gpu_hist"
  },
  "configuration": {
    "booster": "gbtree",
    "n_gpus": "1",
    "num_class": "0",
    "num_feature": "10",
    "objective": "reg:linear",
    "predictor": "gpu_predictor",
    "tree_method": "gpu_hist",
    "updater": "grow_gpu_hist"
  },
  "objective": "reg:linear",
  "booster": "gbtree",
  "gbm": {
    "GBTreeModelParam": {
      "num_feature": "10",
      "num_output_group": "1",
      "num_roots": "1",
      "size_leaf_vector": "0"
    },
    "trees": [{
        "TreeParam": {
          "num_feature": "10",
          "num_roots": "1",
          "size_leaf_vector": "0"
        },
        "num_nodes": "9",
        "nodes": [
          {
            "depth": 0,
            "gain": 31.8892,
            "hess": 10,
            "left": 1,
            "missing": 1,
            "nodeid": 0,
            "right": 2,
            "split_condition": 0.580717,
            "split_index": 2
          },
          {
            "depth": 1,
            "gain": 1.5625,
            "hess": 3,
            "left": 5,
            "missing": 5,
            "nodeid": 2,
            "right": 6,
            "split_condition": 0.160345,
            "split_index": 0
          },
          {
            "depth": 2,
            "gain": 0.25,
            "hess": 2,
            "left": 7,
            "missing": 7,
            "nodeid": 6,
            "right": 8,
            "split_condition": 0.62788,
            "split_index": 0
          },
          {
            "hess": 1,
            "leaf": 0.375,
            "nodeid": 8
          },
          {
            "hess": 1,
            "leaf": 0.075,
            "nodeid": 7
          },
          {
            "hess": 1,
            "leaf": -0.075,
            "nodeid": 5
          },
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
        ],
        "leaf_vector": []
      }],
    "tree_info": [0]
  }
}
)json";
  std::stringstream ss(model_json);
  Json j {json::Json::Load(&ss)};

  std::string dump_path = "/tmp/dump-model.json";
  std::ofstream fout (dump_path, std::ios_base::out);
  Json::Dump(j, &fout);
  fout.close();

  std::ifstream fin (dump_path);
  std::string dump_str = {std::istreambuf_iterator<char>(fin), {}};
  fin.close();
  return model_json == dump_str;
}

bool TestAssigningObjects() {
  Json json;
  json = JsonObject();
  json["ok"] = "Not ok";
}

int main(int argc, char * const argv[]) {
  TEST(TestParseObject);
  TEST(TestParseNumber);
  TEST(TestParseArray);
  TEST(TestEmptyArray);

  TEST(TestIndexing);

  TEST(TestLoadDump);
}