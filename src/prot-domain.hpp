#pragma once

/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * prot-domain.hpp
 * 
 * Project hartebeest is a RDMA(IB) connector,
 *  a refactored version from Mu.
 */

#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <functional>

#include <infiniband/verbs.h> // OFED IB verbs

#include <iostream>

#include "common.hpp"
#include "device.hpp"
#include "mem-region.hpp"

// #include <iostream>

// Note: PdManager must interact with single DeviceManager instance (context).
// A Protection Domain controls multiple PDs.
namespace hartebeest {

    // PDM Return codes.
    // This class do not use throws.
    enum {
        PDM_NOERROR = 0,
        PDM_ERROR_GENERAL = 0x20,
        PDM_ERROR_KEY_EXIST,
        PDM_ERROR_PD_REGISTER,
        PDM_ERRROR_NULL_CONTEXT
    };

    class PdManager {
    private:

        // Management containers:
        //
        // pdinfo_map inserts <K, V>. The key is a string (Protection Domain Name), and the
        //  value is an index value of pd_list.
        // pd_list holds struct ibv_pd type. 
        //
        // A user can distinguish multiple PDs with its name.

        std::map<uint32_t, struct ibv_pd*>  dbm;   // DB map.

    public:
        PdManager() {}
        ~PdManager() {
            for (auto& elem: dbm) ibv_dealloc_pd(elem.second);
        }

        //
        // The inteface naming convention is designed to have:
        //  - do** : These are management functions. 
        //      Does something important. Directly updates its member. 
        //  - is** : Check status.
        //  - get** : Returns reference/value of a member. 
        //
        //  Members follow the underscore, methods follow the CamelCase naming convention.
        bool isPdRegistered2(uint32_t arg_pd_id) {
            if (dbm.find(arg_pd_id) != dbm.end()) 
                return true;
            return false;
        }

        struct ibv_pd* getPd2(uint32_t arg_pd_id) {
            if (!isPdRegistered2(arg_pd_id)) 
                return nullptr;
            return dbm.find(arg_pd_id)->second;
        }

        std::map<uint32_t, struct ibv_pd*>& getPdMap() {
            return dbm;
        }

        //
        // All of the core interface starts with prefix 'do'.
        // doRegisterPd does the following:
        //  - Creates unique_ptr with a deleter that allocates ibv_pd (aligned).
        //  - Registers to management container, pd_list and pdinfo_map.

        int doRegisterPd2(uint32_t arg_pd_id, HcaDevice& arg_opened_dev) {
            if (isPdRegistered2(arg_pd_id))
                return PDM_ERROR_KEY_EXIST;

            struct ibv_pd* pd = ibv_alloc_pd(arg_opened_dev.getContext());
            if (pd == nullptr) 
                return PDM_ERRROR_NULL_CONTEXT;

            dbm.insert(std::pair<uint32_t, struct ibv_pd*>(arg_pd_id, pd));
            return PDM_NOERROR;
        }

        struct ibv_mr* doCreateMr2(uint32_t arg_pd_id, void* arg_addr, size_t arg_len, int arg_rights) {
            if (!isPdRegistered2(arg_pd_id))
                return nullptr;

            return ibv_reg_mr(getPd2(arg_pd_id), arg_addr, arg_len, arg_rights);
        }
    };


}
