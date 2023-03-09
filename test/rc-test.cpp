/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * configurator-test.cpp
 * 
 * Project hartebeest is a RDMA(IB) connector,
 *  a refactored version from Mu.
 */

// #include <cstdlib>
#include <stdio.h>

#include "spdlog/spdlog.h"
// https://github.com/gabime/spdlog
// CentOS: sudo yum -y install spdlog-dev

#include "../src/common.hpp"
#include "../src/device.hpp"
#include "../src/mem-region.hpp"
#include "../src/prot-domain.hpp"

#include "../src/rdma-conf.hpp"

int main() {

    spdlog::set_pattern("[%H:%M:%S %z] [%L] [thread %t] %v");

    spdlog::info("RdmaConfigurator Init.");

    hartebeest::RdmaConfigurator configurator;

    bool ret = configurator.doInitDevice();
    
    if (ret) spdlog::info("OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    // Device Opened Test
    auto dev_man_inner = configurator.getDeviceManager();
    int num_opened_dev = dev_man_inner.getNumOfDevices();

    spdlog::info("Opened device: {}", num_opened_dev);
    if (num_opened_dev)
        spdlog::info("OK", num_opened_dev);
    

    // Buffer alloc,
    spdlog::info("RdmaConfigurator buffer allocation test.");

    ret = configurator.doAllocateBuffer("some-buffer", 1000, 64);

    if (ret) spdlog::info("some-buffer OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    ret = configurator.doAllocateBuffer("some-buffer-2", 1000, 64);

    if (ret) spdlog::info("some-buffer-2 OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    ret = configurator.doAllocateBuffer("some-buffer-3", 1000, 64);

    if (ret) spdlog::info("some-buffer-3 OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    spdlog::info("some-buffer Info: addr {}, idx {}, len {}", 
        configurator.getMrManager().getBufferAddress("some-buffer"),
        configurator.getMrManager().getBufferIdx("some-buffer"),
        configurator.getMrManager().getBufferLen("some-buffer")
        );

    spdlog::info("some-buffer-2 Info: addr {}, idx {}, len {}", 
        configurator.getMrManager().getBufferAddress("some-buffer-2"),
        configurator.getMrManager().getBufferIdx("some-buffer-2"),
        configurator.getMrManager().getBufferLen("some-buffer-2")
        );

    spdlog::info("some-buffer-3 Info: addr {}, idx {}, len {}", 
        configurator.getMrManager().getBufferAddress("some-buffer-3"),
        configurator.getMrManager().getBufferIdx("some-buffer-3"),
        configurator.getMrManager().getBufferLen("some-buffer-3")
        );

    // Pd Registration Test
    spdlog::info("RdmaConfigurator protection domain registration test.");
    ret = configurator.doRegisterPd("some-pd-1");

    if (ret) spdlog::info("some-pd-1 OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    ret = configurator.doRegisterPd("some-pd-2");

    if (ret) spdlog::info("some-pd-2 OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    ret = configurator.doRegisterPd("some-pd-3");

    if (ret) spdlog::info("some-pd-3 OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    spdlog::info("some-pd-1 Info: idx {}", 
        configurator.getPdManager().getIdx("some-pd-1")
        );
    spdlog::info("some-pd-2 Info: idx {}", 
        configurator.getPdManager().getIdx("some-pd-2")
        );
    spdlog::info("some-pd-3 Info: idx {}", 
        configurator.getPdManager().getIdx("some-pd-3")
        );


    spdlog::info("RdmaConfigurator MR registration test.");

    ret = configurator.doRegisterMr(
        "some-pd-1",
        "some-buffer",
        "some-mr-1"
    );

    if (ret) spdlog::info("some-mr-1 OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    ret = configurator.doRegisterMr(
        "some-pd-2",
        "some-buffer-2",
        "some-mr-2"
    );

    if (ret) spdlog::info("some-mr-2 OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    ret = configurator.doRegisterMr(
        "some-pd-3",
        "some-buffer-3",
        "some-mr-3"
    );

    if (ret) spdlog::info("some-mr-3 OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    spdlog::info("Completion Queue registration test.");
    ret = configurator.doRegisterCq("some-send-cq");
    if (ret) spdlog::info("some-send-cq OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    ret = configurator.doRegisterCq("some-recv-cq");
    if (ret) spdlog::info("some-recv-cq OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    spdlog::info("Queue Pair registration test.");
    ret = configurator.doCreateAndRegisterQp(
        "some-pd-1",
        "some-qp-1",
        "some-send-cq",
        "some-recv-cq"
    );

    if (ret) spdlog::info("some-qp-1 OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }


    // Show info.
    spdlog::info("Registration Info test.");
    auto portlid = configurator.getDeviceManager().getHcaDevice().getPortLid();
    auto portid = configurator.getDeviceManager().getHcaDevice().getPortId();

    spdlog::info("Port lid : {}", portlid);
    spdlog::info("Port id : {}", portid);

    spdlog::info("Association Test: PD {}, MR {}", 
        configurator.getAssociatedPdFromMr("some-mr-1"),
        "some-mr-1");

    spdlog::info("Association Test: PD {}, MR {}", 
        configurator.getAssociatedPdFromMr("some-mr-2"),
        "some-mr-2");

    spdlog::info("Association Test: PD {}, QP {}", 
        configurator.getAssociatedPdFromQp("some-mr-2"),
        "some-mr-2");

    spdlog::info("Association Test: PD {}, QP {}", 
        configurator.getAssociatedPdFromQp("some-qp-2"),
        "some-qp-2");
    
    spdlog::info("Association Test: PD {}, QP {}", 
        configurator.getAssociatedPdFromQp("some-qp-1"),
        "some-qp-1");

    spdlog::info(
        "My {} QPN = {}, PortLID = {}", 
        "some-qp-1",
        configurator.getQManager().getQp("some-qp-1")->qp_num,
        configurator.getDeviceManager().getHcaDevice().getPortLid()    
    );

    // Initializing QP

    ret = configurator.getQManager().doInitQp(
        "some-qp-1",
        configurator.getDeviceManager().getHcaDevice().getPortId()   
    );

    if (ret) spdlog::info("QP Init OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    ret = configurator.getQManager().doConnectRemoteRcQp(
        "some-qp-1",
        1,
        1546,
        5
    );

    if (ret) spdlog::info("QP Modify OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }



    getchar();

    return 0;
}
