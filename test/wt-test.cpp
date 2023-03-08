/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * wt-test.cpp
 * 
 * Project hartebeest is a RDMA(IB) connector,
 *  a refactored version from Mu.
 */

#include "spdlog/spdlog.h"
// https://github.com/gabime/spdlog
// CentOS: sudo yum -y install spdlog-dev

#include "../src/common.hpp"
#include "../src/device.hpp"
#include "../src/mem-region.hpp"
#include "../src/prot-domain.hpp"

#include "../src/rdma-wt.hpp"

int main() {

    spdlog::set_pattern("[%H:%M:%S %z] [%L] [thread %t] %v");

    spdlog::info("RdmaWalkieTalike Init.");

    hartebeest::RdmaConfigurator wt;

    bool ret = wt.doInitDevice();
    
    if (ret) spdlog::info("OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    // Device Opened Test
    auto dev_man_inner = wt.getDeviceManager();
    int num_opened_dev = dev_man_inner.getNumOfDevices();

    spdlog::info("Opened device: {}", num_opened_dev);
    if (num_opened_dev)
        spdlog::info("OK", num_opened_dev);
    

    // Buffer alloc,
    spdlog::info("RdmaWalkieTalike buffer allocation test.");

    ret = wt.doAllocateBuffer("some-buffer", 1000, 64);

    if (ret) spdlog::info("some-buffer OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    ret = wt.doAllocateBuffer("some-buffer-2", 1000, 64);

    if (ret) spdlog::info("some-buffer-2 OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    ret = wt.doAllocateBuffer("some-buffer-3", 1000, 64);

    if (ret) spdlog::info("some-buffer-3 OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    spdlog::info("some-buffer Info: addr {}, idx {}, len {}", 
        wt.getMrManager().getAddress("some-buffer"),
        wt.getMrManager().getIdx("some-buffer"),
        wt.getMrManager().getLen("some-buffer")
        );

    spdlog::info("some-buffer-2 Info: addr {}, idx {}, len {}", 
        wt.getMrManager().getAddress("some-buffer-2"),
        wt.getMrManager().getIdx("some-buffer-2"),
        wt.getMrManager().getLen("some-buffer-2")
        );

    spdlog::info("some-buffer-3 Info: addr {}, idx {}, len {}", 
        wt.getMrManager().getAddress("some-buffer-3"),
        wt.getMrManager().getIdx("some-buffer-3"),
        wt.getMrManager().getLen("some-buffer-3")
        );

    // Pd Registration Test
    spdlog::info("RdmaWalkieTalike protection domain registration test.");
    ret = wt.doRegisterPd("some-pd-1");

    if (ret) spdlog::info("some-pd-1 OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    ret = wt.doRegisterPd("some-pd-2");

    if (ret) spdlog::info("some-pd-2 OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    ret = wt.doRegisterPd("some-pd-3");

    if (ret) spdlog::info("some-pd-3 OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    spdlog::info("some-pd-1 Info: idx {}", 
        wt.getPdManager().getIdx("some-pd-1")
        );
    spdlog::info("some-pd-2 Info: idx {}", 
        wt.getPdManager().getIdx("some-pd-2")
        );
    spdlog::info("some-pd-3 Info: idx {}", 
        wt.getPdManager().getIdx("some-pd-3")
        );


    spdlog::info("RdmaWalkieTalike MR registration test.");

    ret = wt.doRegisterMr(
        "some-pd-1",
        "some-buffer",
        "some-mr-1"
    );

    if (ret) spdlog::info("some-mr-1 OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    ret = wt.doRegisterMr(
        "some-pd-2",
        "some-buffer-2",
        "some-mr-2"
    );

    if (ret) spdlog::info("some-mr-2 OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    ret = wt.doRegisterMr(
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
    ret = wt.doRegisterCq("some-send-cq");
    if (ret) spdlog::info("some-send-cq OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    ret = wt.doRegisterCq("some-recv-cq");
    if (ret) spdlog::info("some-recv-cq OK");
    else {
        spdlog::info("Failed.");
        return 0;
    }

    spdlog::info("Queue Pair registration test.");
    ret = wt.doCreateAndRegisterQp(
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


    return 0;
}
