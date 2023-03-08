#pragma once

/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * device.hpp
 */

// #include <atomic>

// #include <thread>
// #include <vector>

#include <cstdint>
#include <vector>

#include <mutex>
#include <memory>

#include <infiniband/verbs.h> /* OFED IB verbs */

#include <iostream>


namespace hartebeest {

#define DEFAULT_NODE_ID     (-0x00ff)

    // Node Settings
    struct Node {
    private:
        int32_t     id;

    public:
        Node() : id(DEFAULT_NODE_ID) {};
        Node(int32_t arg_id) : id(arg_id) {};
        ~Node() = default;

        void            setId(int32_t) = delete; // RAII
        const int32_t   getId() const { return id; };
    };

    class HcaDevice final {
    private:
        struct ibv_device*      device = nullptr;
        struct ibv_context*     context = nullptr;
        struct ibv_device_attr  device_attr;

        uint8_t                 port_id = 0;
        uint16_t                port_lid = 0;

    public:
        // Birth
        HcaDevice() {
            memset(&device_attr, 0, sizeof(device_attr));
        }

        HcaDevice(struct ibv_device* arg_device) : device(arg_device) { 
            memset(&device_attr, 0, sizeof(device_attr));
        }

        // Interface
        bool doReset() {
            if (context != nullptr)
                ibv_close_device(context);
            
            memset(&device_attr, 0, sizeof(device_attr));
        }

        // 
        bool doOpen() {
            context = ibv_open_device(device);
            return 
                (ibv_query_device(context, &device_attr) != 0) ? 0 : 1;
        }

        struct ibv_context*             getContext() const { return context; }
        struct ibv_device_attr const &  getDeviceAttributes() const { return device_attr; }

        uint8_t                         getPortId() const { return port_id; }
        uint16_t                        getPortLid() const {return port_lid; }

        void setPortId(uint8_t arg_port_id) { port_id = arg_port_id; }
        void setPortLid(uint16_t arg_port_lid) { port_lid = arg_port_lid; }

        // Clean Up
        ~HcaDevice() {
            doReset();
        }
        
    };

    //
    // Class Connection
    class DeviceManager final {
    private:

        enum NodeType : int8_t { UNKNOWN_NODE = -1, CA = 1, RNIC = 4 };
        enum TransportType : int8_t { UNKNOWN_TRANSPORT = -1, IB = 0, IWARP = 1 };

        struct ibv_device** raw_dev_list = nullptr;
        std::vector<HcaDevice> hca_device_list{};

    public:
        DeviceManager() {};
        ~DeviceManager() {
            if (raw_dev_list != nullptr)
                ibv_free_device_list(raw_dev_list);

            std::cout << "~DeviceManager()\n";
        };

        bool doGetDevice() {
            
            int num_devices = 0;
            raw_dev_list = ibv_get_device_list(&num_devices);

            if (raw_dev_list == nullptr) return false;

            for (int i = 0; i < num_devices; i++)
                hca_device_list.push_back(HcaDevice(raw_dev_list[i]));
        };

        bool doOpenDevice(int32_t arg_dev_id = 0) {

            if (hca_device_list.size() == 0) return false; // Must be initialized
            hca_device_list.at(arg_dev_id).doOpen();

        } // This can throw exceptions.

        bool doPortBind(int32_t arg_dev_id = 0, int32_t arg_index = 0) {

            HcaDevice& this_dev = hca_device_list.at(arg_dev_id);

            if (this_dev.getContext() == nullptr) // Device Not opened
                return false;
            
            for (int i = 1; 
                i <= this_dev.getDeviceAttributes().phys_port_cnt; // For each port numbers, 
                i++) {
                
                struct ibv_port_attr port_attr;
                memset(&port_attr, 0, sizeof(ibv_port_attr));

                if (ibv_query_port(this_dev.getContext(), i, &port_attr)) 
                    return false; // Port query fail

                if (port_attr.phys_state != IBV_PORT_ACTIVE &&
                    port_attr.phys_state != IBV_PORT_ACTIVE_DEFER) {
                    continue;
                }

                size_t skipped_active_ports = 0;
                
                if (skipped_active_ports == arg_index) {
                    
                    if (port_attr.link_layer != IBV_LINK_LAYER_INFINIBAND) return false;

                    this_dev.setPortId(i);
                    this_dev.setPortLid(port_attr.lid);

                    return true;
                }

                skipped_active_ports += 1;
            }
        }

        int getNumOfDevices() { return hca_device_list.size(); }
        
        HcaDevice& getHcaDevice(int32_t arg_dev_id = 0) {
            return hca_device_list.at(arg_dev_id);
        }
    };

    //
    // This code is for testing.
    std::unique_ptr<DeviceManager>  g_dm;   // Global Device Manager
    std::once_flag                  g_dm_init;

    DeviceManager& getDeviceManager() {
        std::call_once(g_dm_init, []() {
            g_dm.reset(new DeviceManager());
        });

        return *g_dm.get();
    }
}

