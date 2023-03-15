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

int main() {

    spdlog::set_pattern("[%H:%M:%S] [%L] [thread %t] %v");

    hartebeest::RdmaConfigurator        confr;
    hartebeest::ConfigFileExchanger     exchr;

    std::string funcn = "";
    bool ret;

    
    

#define __TEST__(X)     if ((X)) ; else goto FAIL;


    //
    // Initialize device
    funcn = "RdmaConfigurator::doInitDevice()";
    __TEST__((ret = confr.doInitDevice()) == true)

    // Load configuration file
    funcn = "ConfigFileExchanger::doInitDevice()";
    __TEST__((ret = exchr.doReadConfigFile()) == true) 

    //
    // Allocate Buffer Test
    funcn = "RdmaConfigurator::doAllocateBuffer()";
    __TEST__((ret = confr.doAllocateBuffer("buffer-1", 512, 64)) == true)
    __TEST__((ret = confr.doAllocateBuffer("buffer-2", 512, 64)) == true) 
    __TEST__((ret = confr.doAllocateBuffer("buffer-3", 512, 64)) == true) 
    __TEST__((ret = confr.doAllocateBuffer("buffer-4", 512, 64)) == true) 
    __TEST__((ret = confr.doAllocateBuffer("buffer-5", 512, 64)) == true) 

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

    //
    // 

    spdlog::info("Paused. Press any key to continue.");
    getchar();

    funcn = "ConfigFileExchanger::doExchange()";
    __TEST__((ret = exchr.doExchange()) == true)





    spdlog::info("End reached.");

    return 0;

FAIL:
    spdlog::error("{} FAIL", funcn);

    return 0;
}