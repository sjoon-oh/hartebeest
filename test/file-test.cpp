/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * file-test.cpp
 */

// #include <spdlog/common.h>
#include "spdlog/spdlog.h"
// https://github.com/gabime/spdlog
// CentOS: sudo yum -y install spdlog-devel

#include "../src/rdma-conf.hpp"

int main() {

    spdlog::set_pattern("[%H:%M:%S %z] [%L] [thread %t] %v");

    hartebeest::ConfigFileExchanger exchanger;

    //
    // 1. Read a file.
    spdlog::info("Reading pre- RDMA configuration file.");

    std::string file_path{"some_file.json"};
    auto ret = exchanger.doReadConfigFile(file_path);

    if (ret) spdlog::info("READ File OK");
    else {
        spdlog::info("Failed to read {}", file_path);
    }

    ret = exchanger.doReadConfigFile();

    if (ret) spdlog::info("READ File OK");
    else {
        spdlog::info("Failed to read {}", hartebeest::EXCH_CONF_DEFAULT_PATH);
        return 0;
    }











    return 0;
}