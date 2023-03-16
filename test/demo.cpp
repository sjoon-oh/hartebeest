/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * conn-test.cpp
 * 
 * Project hartebeest is a RDMA(IB) connector,
 *  a refactored version from Mu.
 */

#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"

#include "../src/rdma-conf.hpp"

#include <cstring>
#include <cstdlib>

int main() {

    spdlog::set_pattern("[%H:%M:%S] [%L] [thread %t] %v");

    hartebeest::RdmaConfigurator        confr;
    hartebeest::ConfigFileExchanger     exchr;

    std::string funcn = "";
    std::string buf_dummy{"I am hartebeest dummy."};
    bool ret;

    int other_node_id = 0;

#define __TEST__(X)     if ((X)) ; else goto FAIL;

    //
    // Initialize device
    funcn = "RdmaConfigurator::doInitDevice()";
    __TEST__((ret = confr.doInitDevice()) == true)

    // Load configuration file
    funcn = "ConfigFileExchanger::doReadConfigFile()";
    __TEST__((ret = exchr.doReadConfigFile()) == true) 

    //
    // Allocate Buffer Test
    funcn = "RdmaConfigurator::doAllocateBuffer()";
    __TEST__((ret = confr.doAllocateBuffer("buffer-1", 512, 64)) == true)
    __TEST__((ret = confr.doAllocateBuffer("buffer-2", 512, 64)) == true) 
    __TEST__((ret = confr.doAllocateBuffer("buffer-3", 512, 64)) == true) 
    __TEST__((ret = confr.doAllocateBuffer("buffer-4", 512, 64)) == true) 
    __TEST__((ret = confr.doAllocateBuffer("buffer-5", 512, 64)) == true) 
    __TEST__((ret = confr.doAllocateBuffer("buffer-6", 512, 64)) == true)
    __TEST__((ret = confr.doAllocateBuffer("buffer-7", 512, 64)) == true) 
    __TEST__((ret = confr.doAllocateBuffer("buffer-8", 512, 64)) == true) 
    __TEST__((ret = confr.doAllocateBuffer("buffer-9", 512, 64)) == true) 
    __TEST__((ret = confr.doAllocateBuffer("buffer-10", 512, 64)) == true) 

    //
    // Protection Domain Generate Test
    funcn = "RdmaConfigurator::doRegisterPd()";
    __TEST__((ret = confr.doRegisterPd("pd-1")) == true) 
    __TEST__((ret = confr.doRegisterPd("pd-2")) == true) 

    //
    // MR, PD OK
    funcn = "RdmaConfigurator::doCreateAndRegisterMr()";
    __TEST__((ret = confr.doCreateAndRegisterMr("pd-1", "buffer-1", "mr-1")) == true) 
    __TEST__((ret = confr.doCreateAndRegisterMr("pd-1", "buffer-2", "mr-2")) == true) 
    __TEST__((ret = confr.doCreateAndRegisterMr("pd-1", "buffer-3", "mr-3")) == true) 
    __TEST__((ret = confr.doCreateAndRegisterMr("pd-2", "buffer-4", "mr-4")) == true) 
    __TEST__((ret = confr.doCreateAndRegisterMr("pd-2", "buffer-5", "mr-5")) == true)
    __TEST__((ret = confr.doCreateAndRegisterMr("pd-1", "buffer-6", "mr-6")) == true) 
    __TEST__((ret = confr.doCreateAndRegisterMr("pd-1", "buffer-7", "mr-7")) == true) 
    __TEST__((ret = confr.doCreateAndRegisterMr("pd-1", "buffer-8", "mr-8")) == true) 
    __TEST__((ret = confr.doCreateAndRegisterMr("pd-2", "buffer-9", "mr-9")) == true) 
    __TEST__((ret = confr.doCreateAndRegisterMr("pd-2", "buffer-10", "mr-10")) == true) 


    funcn = "RdmaConfigurator::doCreateAndRegisterCq()";
    __TEST__((ret = confr.doCreateAndRegisterCq("send-cq-1")) == true) 
    __TEST__((ret = confr.doCreateAndRegisterCq("send-cq-2")) == true) 
    __TEST__((ret = confr.doCreateAndRegisterCq("recv-cq-1")) == true) 
    __TEST__((ret = confr.doCreateAndRegisterCq("recv-cq-2")) == true) 

    funcn = "RdmaConfigurator::doCreateAndRegisterQp()";
    __TEST__((ret = confr.doCreateAndRegisterQp("pd-1", "qp-1", "send-cq-1", "recv-cq-1")) == true) 
    __TEST__((ret = confr.doCreateAndRegisterQp("pd-1", "qp-2", "send-cq-2", "recv-cq-2")) == true) 

    // 
    // Initialize QP
    funcn = "RdmaConfigurator::doInitQp()";
    __TEST__((ret = confr.doInitQp("qp-1")) == true) 
    __TEST__((ret = confr.doInitQp("qp-2")) == true) 

    //
    // Export
    funcn = "RdmaConfigurator::doExportAll()";
    __TEST__((ret = confr.doExportAll(
            exchr.getThisNodeId()
        )) == true)

    funcn = "ConfigFileExchanger::setThisNodeConf()";
    __TEST__((ret = exchr.setThisNodeConf()) == true)

    if (exchr.getThisNodeRole() == hartebeest::ROLE_SERVER) {
        spdlog::info("Playing server role.");
        other_node_id = 1;
    }
    
    else {
        spdlog::info("Playing client role.");
        other_node_id = 0;
    }

    // spdlog::info("Paused. Press any key to continue.");
    // getchar();

    funcn = "ConfigFileExchanger::doExchange()";
    __TEST__((ret = exchr.doExchange()) == true)

    // In this test, QPs with identical name will be connected.
    // QP connection test.
    funcn = "RdmaConfigurator::doConnectRcQp()";
    __TEST__((ret = confr.doConnectRcQp(
            "qp-1",
            exchr.getOtherNodePortId(other_node_id, "qp-1"),
            exchr.getOtherNodeQpn(other_node_id, "qp-1"),
            exchr.getOtherNodePortLid(other_node_id, "qp-1")
        )) == true)

    sleep(1);

    // Pause for a second.
    if (exchr.getThisNodeRole() == hartebeest::ROLE_CLIENT) {
        while ((std::string((char* )confr.getMrManager().getMrAddr("mr-1")) != buf_dummy)) {

            sleep(1);
            spdlog::info("Memory Region Read: {}", 
                (char *)confr.getMrManager().getMrAddr("mr-1"));
        }

        spdlog::info("Check finished.");
    }
    else {

        // Server sends
        memcpy(
            confr.getMrManager().getMrAddr("mr-1"),     // Destination
            buf_dummy.c_str(),                          // Source
            buf_dummy.size()                            // Size
        );

        spdlog::info("Dummy string copied: at [{}]", confr.getMrManager().getMrAddr("mr-1"));
        spdlog::info("                   : {}", (char *)confr.getMrManager().getMrAddr("mr-1"));

        // Connected to each 'qp-1's
        funcn = "RdmaConfigurator::doBuildSendWr()";
        __TEST__( 
            confr.doPosrSendWr(
                "qp-1",             // My QP name
                confr.getMrManager().getMrAddr("mr-1"),
                buf_dummy.size(),
                confr.getMrManager().getMrLocalKey("mr-1"),
                exchr.getOtherNodeMrAddr(other_node_id, "mr-1"),
                exchr.getOtherNodeMrRk(other_node_id, "mr-1")
                )
            == true)

        spdlog::info("Server paused: Press any key to continue;");
        getchar();

    }


    spdlog::info("End reached.");
    return 0;

FAIL:
    spdlog::error("{} FAIL", funcn);

    return -1;
}