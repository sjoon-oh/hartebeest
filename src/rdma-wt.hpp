#pragma once

/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * RdmaConfigurator.hpp
 * 
 * Project hartebeest is a RDMA(IB) connector,
 *  a refactored version from Mu.
 */

#include <map>

#include "device.hpp"
#include "mem-region.hpp"
#include "queues.hpp"
#include "prot-domain.hpp"

namespace hartebeest {
    
    /*
     * Class RdmaConfigurator (Temporary Class Name)
     * 
     * This instance holds all RDMA-related managers. 
     * Each manager handles single-type ibv* api instances.
     *  
     * - DeviceManager : Manages HCA
     * - MrManager : Manages memory-related stuff, i.e. Memory Region, Buffers
     * - QueueManager : Provides basic IBV queues APIs.
     * - PdManager : Manages Protection Domains.
     * 
     * RdmaConfigurator works as a coordinator of these instances. 
     *  It provides few member functions, so that all managers may interact
     *  without big errors.
     * 
     * A program may have multiple IBV-resources, 
     *  such as many Queue Pairs, Protection Domains, and much more.
     *  Single RdmaWalkieTalike instance handles all of them in one instance, thus
     *  this is not designed to have many RdmaConfigurator objects in a program. 
     * 
     * Please refer to README.md for more information.
     * 
     */
    class RdmaConfigurator {
    private:
        
        bool                            dev_init;
        // It is reasonable to have a single device manager.

        std::unique_ptr<DeviceManager>  dev_manager;    // Device Management
        std::unique_ptr<MrManager>      mr_manager;     // Memory Management
        std::unique_ptr<QueueManager>   qs_manager;     // Queue Management
        std::unique_ptr<PdManager>      pd_manager;     // Protection Domain Management

        

    public:
        RdmaConfigurator() : dev_init(false) {
            
            dev_manager.reset(new DeviceManager());
            mr_manager.reset(new MrManager());
            qs_manager.reset(new QueueManager());
            pd_manager.reset(new PdManager());
        }
        ~RdmaConfigurator() {

            mr_manager.release();
            qs_manager.release();
            pd_manager.release();
            dev_manager.release();

            // Resource release order matters. 
            // May change, but it is not recommended.
            // Do it at your own risk.
        }

        DeviceManager&  getDeviceManager() { return *dev_manager.get(); }
        MrManager&      getMrManager() { return *mr_manager.get(); }
        QueueManager&   getQManager() { return *qs_manager.get(); }
        PdManager&      getPdManager() { return *pd_manager.get(); }
        
        // Queries
        inline bool isPdRegistered(std::string arg_pd_name) {
            return pd_manager->isPdRegistered(arg_pd_name);
        }

        inline bool isBufferAllocated(std::string arg_buf_name) {
            return mr_manager->isBufferAllocated(arg_buf_name);
        }
        
        inline bool isMrRegistered(std::string arg_mr_name) {
            return mr_manager->isMrRegistered(arg_mr_name);
        }

        //
        // This is temporary, set device/port 0 as default.
        // Call Sequence 0.
        bool doInitDevice() {
            
            bool ret = true;
            // auto dev_manager = getDeviceManager();

            const int dev_id = 0;
            const int dev_idx = 0;
            
            if (!dev_init) {
                ret = dev_manager->doGetDevice();
                ret = dev_manager->doOpenDevice(dev_id);
                ret = dev_manager->doPortBind(dev_id, dev_idx);
            }

            dev_init = true;

            return ret;
        }
        
        //
        // Call Sequence 1.
        bool doRegisterPd(std::string arg_pd_name) {
            return pd_manager->doRegisterPd(arg_pd_name, dev_manager->getHcaDevice());
        }
        
        //
        // Call Sequence 2.
        bool doAllocateBuffer(std::string arg_buf_name, size_t arg_len, int arg_align) {
            return mr_manager->doAllocateBuffer(arg_buf_name, arg_len, arg_align);
        }

        //
        // Call Sequence 3.
        bool doRegisterMr(std::string arg_pd_name, 
            std::string arg_buf_name, std::string arg_mr_name) {
            
            void* addr = mr_manager->getAddress(arg_buf_name);
            size_t len = mr_manager->getLen(arg_buf_name);

            struct ibv_mr* mr;
            mr = pd_manager->doCreateMr(
                arg_pd_name, 
                addr, 
                len, 
                LOCAL_READ | LOCAL_WRITE | REMOTE_READ | REMOTE_WRITE
            );

            if (mr == nullptr) return false;

            return mr_manager->doRegisterMr2(arg_mr_name, mr);
        }

        //
        // Call Sequence 4.
        bool doRegisterCq(std::string arg_cq_name) {
            
            return
                qs_manager->doRegisterCq(
                    arg_cq_name, 
                    dev_manager->getHcaDevice().getContext());
        }

        //
        // Call Sequence 5.
        bool doCreateAndRegisterQp(
            std::string arg_pd_name,
            std::string arg_qp_name,
            std::string arg_send_cq_name,
            std::string arg_recv_cq_name
        ) {
            return
                qs_manager->doCreateAndRegisterQp(
                    arg_qp_name,
                    pd_manager->getPd(arg_pd_name),
                    arg_send_cq_name,
                    arg_recv_cq_name
                );
        }



    };

}