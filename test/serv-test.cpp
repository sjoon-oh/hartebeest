/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * serv-test.cpp
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


#include "../src/rdma-conf.hpp"

int main() {

    // sudo firewall-cmd --permanent --zone=public --add-port=2023/tcp

    spdlog::set_pattern("[%H:%M:%S %z] [%L] [thread %t] %v");

    hartebeest::ConfigFileExchanger exchanger;

    //
    // 1. Read a file.
    spdlog::info("Reading pre- RDMA configuration file.");

    auto ret = exchanger.doReadConfigFile();
    // ret = exchanger.doReadConfigFile();

    if (ret) spdlog::info("READ File OK");
    else {
        spdlog::error("Failed to read {}", hartebeest::EXCH_CONF_DEFAULT_PATH);
        return 0;
    }

    //
    // 2. File check
    std::string str_dump = exchanger.getObjDump();

    if (str_dump != "") spdlog::info("Dump: {}", str_dump);
    else {
        spdlog::error("Failed to parse {}", hartebeest::EXCH_CONF_DEFAULT_PATH);
        return 0;
    }

    spdlog::info("Number of nodes: {}", exchanger.getNumOfPlayers());
    
    for (int i = 0; i < exchanger.getNumOfPlayers(); i++) {
        spdlog::info(
            "Player[{}]",
            exchanger.getPlayer(i).node_id
        );
    }

    //
    // 3. Check roles
    spdlog::info("What role am I?");

    if (exchanger.getThisNodeRole() 
        == hartebeest::ROLE_SERVER) {
        spdlog::info("I am a distributer.");
    
        if ((ret = exchanger.doTestServer()) == false)
            spdlog::error("doTestServer() Error.");


    }
    else if (exchanger.getThisNodeRole() 
        == hartebeest::ROLE_CLIENT) {
        spdlog::info("I am a passive player.");


        if ((ret = exchanger.doTestClient()) == false)
            spdlog::error("doTestClient() Error.");

    }   


    return 0;
}