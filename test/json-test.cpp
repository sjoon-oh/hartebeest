/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * json-test.cpp
 * 
 * Project hartebeest is a RDMA(IB) connector,
 *  a refactored version from Mu.
 */

#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"

#include <string>

// Exports
#include <fstream>
#include <ostream>

int main() {

    spdlog::set_pattern("[%H:%M:%S %z] [%L] [thread %t] %v");

    using json = nlohmann::json;

    json some_json_obj;

    some_json_obj["serv_id"] = 0;
    some_json_obj["qpns"] = json::array();

    some_json_obj["qpns"].push_back(1);
    some_json_obj["qpns"].push_back(2);
    some_json_obj["qpns"].push_back(3);
    some_json_obj["qpns"].push_back(4);

    json qp_info({{"name", "some-qp-1"}, {"qpn", 0}, {"lkey", 1234}, {"rkey", 4321}});

    some_json_obj["qp_infos"].push_back(qp_info);

    std::string dump_str = some_json_obj.dump(2);
    spdlog::info("JSON dump: {}", dump_str);

    // File export
    std::string file_path{"build/example.json"};

    std::ofstream json_output(file_path);
    json_output << std::setw(4) << some_json_obj << std::endl;

    std::ifstream json_input(file_path);

    json loaded_json;
    json_input >> loaded_json;

    dump_str = loaded_json.dump(2);

    spdlog::info("After Load: {}", dump_str);

    return 0;
}