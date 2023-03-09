/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * pm_test.cpp
 */

#include <cstdlib>

#include "spdlog/spdlog.h"
// https://github.com/gabime/spdlog
// CentOS: sudo yum -y install spdlog-dev

#include "../src/common.hpp"
#include "../src/device.hpp"
#include "../src/mem-region.hpp"
#include "../src/prot-domain.hpp"

int main() {

    spdlog::set_pattern("[%H:%M:%S %z] [%L] [thread %t] %v");

#define __TEST_START__  spdlog::info("******** Start ********"); {
#define __TEST_END__    } spdlog::info("******** End ********\n");


    spdlog::info("Unit test 1: PdManager & MrManager Init Test");
    __TEST_START__
        
        auto dev_man = hartebeest::getDeviceManager();

        if (dev_man.doGetDevice() == false)
            spdlog::error("Device get failed");

        int num_opened_device = dev_man.getNumOfDevices();
        spdlog::info("Number of Devices: {}", num_opened_device);

        if (dev_man.doOpenDevice() == false) {
            spdlog::error("Device open failed");
            return -1;
        }

        const int device_id = 0;
        const int index = 0;

        if (dev_man.doPortBind(device_id, index) == false)
            spdlog::error("Device port binding failed");
        
        else
            spdlog::info("OK");

        spdlog::info("Device opened.");
        spdlog::info("Initializing PdManager.");

        hartebeest::PdManager pd_man;

        if (pd_man.doRegisterPd("first-pd", dev_man.getHcaDevice())) {
            spdlog::info("Protection Domain successfully registered.");
        }
        else {
            spdlog::error("PD registration failed");
            return -1;
        }

        hartebeest::MrManager mr_man;
        const size_t allocated_size = 1024 * 1024;
        const int alignment = 64;

        if (mr_man.doAllocateBuffer("first-mr", 1024, alignment)) {
            spdlog::info("Memory allocation success: first-mr");
            spdlog::info("first-mr index: {}", mr_man.getBufferIdx("first-mr"));
            spdlog::info("first-mr len: {}", mr_man.getBufferLen("first-mr"));
        }
        else {
            spdlog::error("Allocation of buffer failed");
            return -1;
        }

        // Multiple Allocation
        if (mr_man.doAllocateBuffer("second-mr", 1024, alignment)) {
            spdlog::info("Memory allocation success: second-mr");
            spdlog::info("second-mr index: {}", mr_man.getBufferIdx("second-mr"));
            spdlog::info("second-mr len: {}", mr_man.getBufferLen("second-mr"));
        }
        else {
            spdlog::error("Allocation of buffer failed");
            return -1;
        }

        auto mr = pd_man.doCreateMr(
            "first-pd",
            mr_man.getBufferAddress("first-mr"), 
            mr_man.getBufferLen("first-mr"), 
            0 // Local Read
        );
        
        if (mr == nullptr) {
            spdlog::error("MR registration on PD (1st) failed.");
            return -1;
        }

        if (mr_man.doRegisterMr2("first-mr", mr)) {
            spdlog::info("MR registration process sucess.");
        }
        else {
            spdlog::error("MR registration process fail.");
            return -1;
        }



    __TEST_END__


    return 0;
}




