#pragma once

/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * RdmaConfigurator.hpp
 * 
 * Project hartebeest is a RDMA(IB) connector,
 *  a refactored version from Mu.
 */

#include <map>
// #include <utility>

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
     *  Single RdmaConfigurator instance handles all of them in one instance, thus
     *  this is not designed to have many RdmaConfigurator objects in a program. 
     * 
     * Please refer to README.md for more information.
     * 
     */
    class RdmaConfigurator {
    private:
        
        bool                            dev_init;
        // It is reasonable to have a single device manager.

        //
        // Creation/Deletion/Linker
        std::unique_ptr<DeviceManager>  dev_manager;    // Device Management
        std::unique_ptr<MrManager>      mr_manager;     // Memory Management
        std::unique_ptr<QueueManager>   qs_manager;     // Queue Management
        std::unique_ptr<PdManager>      pd_manager;     // Protection Domain Management

        //
        // Metadata Management.
        // Records what resource(key) is associated to which Protection Domain.
        // <K, V> = <mr_name, pd_name>
        // <K, V> = <qp_name, pd_name>
        // <K, V> = <cq_name, hca_idx>
        std::map<std::string, std::string>  mr_pd_regbook; 
        std::map<std::string, std::string>  qp_pd_regbook;
        std::map<std::string, int>          cq_ctx_regbook;



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

        inline bool isMrAssociated(std::string arg_mr_name) {
            if (mr_pd_regbook.find(arg_mr_name) != mr_pd_regbook.end())
                return true;
            return false;
        }

        inline bool isQpAssociated(std::string arg_qp_name) {
            if (qp_pd_regbook.find(arg_qp_name) != qp_pd_regbook.end())
                return true;
            return false;
        }

        inline bool isCqAssociated(std::string arg_cq_name) {
            if (cq_ctx_regbook.find(arg_cq_name) != cq_ctx_regbook.end())
                return true;
            return false;
        }

        inline std::string getAssociatedPdFromMr(std::string arg_mr_name) {
            if (isMrAssociated(arg_mr_name))
                return mr_pd_regbook.find(arg_mr_name)->second;
            else return std::string("");
        }

        inline std::string getAssociatedPdFromQp(std::string arg_qp_name) {
            if (isQpAssociated(arg_qp_name))
                return qp_pd_regbook.find(arg_qp_name)->second;
            else return std::string("");
        }


        inline int getAssociatedHcaIdxFromCq(std::string arg_cq_name) {
            if (isCqAssociated(arg_cq_name))
                return cq_ctx_regbook.find(arg_cq_name)->second;
            else return int{-1};
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

            if (isMrAssociated(arg_mr_name)) return false;
            
            void* addr = mr_manager->getBufferAddress(arg_buf_name);
            size_t len = mr_manager->getBufferLen(arg_buf_name);

            struct ibv_mr* mr;
            mr = pd_manager->doCreateMr(
                arg_pd_name, 
                addr, 
                len, 
                0 | IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE
            );

            if (mr == nullptr) return false;
            
            mr_pd_regbook.insert(std::pair<std::string, std::string>(arg_mr_name, arg_pd_name));

            return mr_manager->doRegisterMr2(arg_mr_name, mr);
        }

        //
        // Call Sequence 4.
        bool doRegisterCq(std::string arg_cq_name) {
            
            if (isCqAssociated(arg_cq_name)) false;
            cq_ctx_regbook.insert(std::pair<std::string, int>(arg_cq_name, 0));

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

            if (isQpAssociated(arg_qp_name)) false;
            qp_pd_regbook.insert(std::pair<std::string, std::string>(arg_qp_name, arg_pd_name));

            return
                qs_manager->doCreateAndRegisterQp(
                    arg_qp_name,
                    pd_manager->getPd(arg_pd_name),
                    arg_send_cq_name,
                    arg_recv_cq_name
                );
        }

        bool doInitQp(std::string arg_qp_name, int arg_dev_idx = 0) {
            
            return
                qs_manager->doInitQp(
                    arg_qp_name, 
                    dev_manager->getHcaDevice().getPortId()
                    );
        }

        bool doConnectRcQp(
            std::string arg_qp_name,
            int arg_remote_port_id,
            uint32_t arg_remote_qpn,
            uint16_t arg_remote_lid
        ) {
            return 
                qs_manager->doConnectRemoteRcQp(
                    arg_qp_name, arg_remote_port_id, arg_remote_qpn, arg_remote_lid
                );
        }
    };

    

}