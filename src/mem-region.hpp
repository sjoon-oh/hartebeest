#pragma once

/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * mem-region.cpp
 * 
 * Project hartebeest is a RDMA(IB) connector,
 *  a refactored version from Mu.
 */

#include <map>
#include <string>
#include <vector>
#include <memory>

#include <infiniband/verbs.h> // OFED IB verbs

#include "prot-domain.hpp"

#include <iostream>

namespace hartebeest {


    enum {
        MRM_NOERROR = 0,
        MRM_ERROR_GENERAL = 0x40,
        MRM_ERROR_KEY_EXIST,

    };


    //
    // class MrManager does the following:
    //  - Manages memory region of a program uses. All MRs are handled by the manager.
    //  - Supports memory allocation. If a user want to manage Mr with the manager,
    //      call doBufferAllocate.
    class MrManager {
    private:

        //
        // Management containers:
        //
        // bufinfo_map inserts <K, V>. The key is a string (Buffer Name), and the
        //  value is an index value of buf_list.
        // buf_list holds unique_ptr type (with a deleter). 
        //
        // mrinfo_map inserts <K, V>. The key is a string (MR Name), and the
        //  value is an index value of mr_list.
        // mr_list holds unique_ptr type (with a deleter).
        //  ibv_mr can be referenced using mr_list.
        //
        // A user can distinguish multiple buffers and memory regions with their name.

        std::map<uint32_t, struct ibv_mr*> dbm;

    public:
        MrManager() {};
        ~MrManager() {
            
            void* buffer;
            for (auto& elem: dbm) {

                if ((buffer = elem.second->addr) != nullptr) {
                    free(buffer);
                    ibv_dereg_mr(elem.second);
                }
            }
        };

        //
        // The inteface naming convention is designed to have:
        //  - do** : These are management functions. 
        //      Does something important. Directly updates its member. 
        //  - is** : Check status.
        //  - get** : Returns reference/value of a member. 
        //
        //  Members follow the underscore, methods follow the CamelCase naming convention.

        bool isMrRegistered2(uint32_t arg_mr_id) {
            if (dbm.find(arg_mr_id) != dbm.end())
                return true;
            return false;
        }

        struct ibv_mr* getMr2(uint32_t arg_mr_id) {
            if (!isMrRegistered2(arg_mr_id)) {
                return nullptr;
            }

            return dbm.find(arg_mr_id)->second;        
        }

        std::map<uint32_t, struct ibv_mr*>& getMrMap() { return dbm; }

        std::vector<struct ibv_mr*> getAssociatedMrs(struct ibv_pd* arg_pd) {
            
            std::vector<struct ibv_mr*> ret;
            for (auto& elem: dbm) {
                if (elem.second->pd == arg_pd) ret.push_back(elem.second);
            }

            return ret;
        }

        //
        // All of the core interface starts with prefix 'do'.
        // doAllocateBuffer does the following:
        //  - Creates unique_ptr with a deleter that allocates memory (aligned).
        //  - Registers to management container, buf_list and bufinfo_map.
        //
        
        uint8_t* doAllocateBuffer2(size_t arg_len, int arg_align) {
            return reinterpret_cast<uint8_t*>(aligned_alloc(arg_align, arg_len));
        };

        //
        // doRegisterMr2 does the following:
        //  - Creates unique_ptr with a deleter that allocates ibv_mr (aligned).
        //      The resource is automatically freed when the manager instance is destroyed.
        //  - Registers memory region to the management container. 
        //
        int doRegisterMr22(uint32_t arg_mr_id, struct ibv_mr* arg_mr) {
            if (isMrRegistered2(arg_mr_id)) 
                return MRM_ERROR_KEY_EXIST;

            dbm.insert(std::pair<uint32_t, struct ibv_mr*>(arg_mr_id, arg_mr));

            return MRM_NOERROR;
        }
    };
}
