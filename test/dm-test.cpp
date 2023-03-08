/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * dm_test.cpp
 */

// #include <spdlog/common.h>
#include "spdlog/spdlog.h"
// https://github.com/gabime/spdlog
// CentOS: sudo yum -y install spdlog-devel

#include "../src/device.hpp"

int main() {

    spdlog::set_pattern("[%H:%M:%S %z] [%L] [thread %t] %v");

#define __TEST_START__  spdlog::info("******** Start ********"); {
#define __TEST_END__    } spdlog::info("******** End ********\n");

    spdlog::info("Unit test 1: Node Instance Test");
    __TEST_START__

        spdlog::info("Node test.");

        hartebeest::Node node_0;
        hartebeest::Node node_1(12);

        spdlog::info("node_0 ID: {}", node_0.getId());
        spdlog::info("node_1 ID: {}", node_1.getId());

    __TEST_END__

    //
    // 
    spdlog::info("Unit test 2: HcaDevice Safety Test");
    __TEST_START__

        hartebeest::DeviceManager dev_man;

        int before_open_device = dev_man.getNumOfDevices();
        spdlog::info("Number of Devices (Before get): {}", before_open_device);

        if (dev_man.doOpenDevice() == false) {
            spdlog::error("Device open failed");

        }
        else {
            int after_open_device = dev_man.getNumOfDevices();
            spdlog::info("Number of Devices (After get): {}", after_open_device);
        }

    __TEST_END__

    //
    //
    spdlog::info("Unit test 3: HcaDevice Test");
    __TEST_START__

        hartebeest::DeviceManager dev_man;

        int before_open_device = dev_man.getNumOfDevices();
        spdlog::info("Number of Devices (Before get): {}", before_open_device);

        if (dev_man.doGetDevice() == false)
            spdlog::error("Device get failed");

        else {
            int after_open_device = dev_man.getNumOfDevices();
            spdlog::info("Number of Devices (After get): {}", after_open_device);

            if (dev_man.doOpenDevice() == false)
                spdlog::error("Device open failed");
        }
    __TEST_END__



    spdlog::info("Unit test 4: HcaDevice Port Binding Test");
    __TEST_START__

        hartebeest::DeviceManager dev_man;

        if (dev_man.doGetDevice() == false)
            spdlog::error("Device get failed");

        else {
            int opened_device = dev_man.getNumOfDevices();
            spdlog::info("Number of Devices: {}", opened_device);

            if (dev_man.doOpenDevice() == false)
                spdlog::error("Device open failed");

            const int device_id = 0;
            const int index = 0;

            if (dev_man.doPortBind(device_id, index) == false)
                spdlog::error("Device port binding failed");
            
            else
                spdlog::info("OK");
        }
    __TEST_END__

    spdlog::info("Unit test 5: common Test");
    __TEST_START__
        
        auto dev_man = hartebeest::getDeviceManager();

        int opened_device = dev_man.getNumOfDevices();
        spdlog::info("Number of Devices: {}", opened_device);

    __TEST_END__

    return 0;
}


