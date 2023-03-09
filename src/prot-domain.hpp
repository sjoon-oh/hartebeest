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

    class PdManager {
    private:
        std::map<std::string, size_t>       pdinfo_map; // <index>
        std::vector<del_unique_ptr<struct ibv_pd>>       pd_list{};

    public:
        PdManager() {}
        ~PdManager() {
            std::cout << "~PdManager()\n";
        }

        bool isPdRegistered(std::string arg_pd_name) {
            if (pdinfo_map.find(arg_pd_name) != pdinfo_map.end())
                return true;
            return false;
        }

        size_t getIdx(std::string arg_pd_name) {
            return pdinfo_map.find(arg_pd_name)->second;
        }

        struct ibv_pd* getPd(std::string arg_pd_name) {
            int idx = getIdx(arg_pd_name);
            return pd_list.at(idx).get();
        }      

        bool doRegisterPd(std::string arg_pd_name, HcaDevice& arg_opened_dev) {
            if (isPdRegistered(arg_pd_name))
                return false;

            struct ibv_pd* pd = ibv_alloc_pd(arg_opened_dev.getContext());
            if (pd == nullptr) return false;

            // std::cout << "PD: " << arg_pd_name << " - " << pd << std::endl;

            del_unique_ptr<struct ibv_pd> uniq_pd(pd, [](struct ibv_pd* arg_pd) {

                auto ret = ibv_dealloc_pd(arg_pd);
                if (ret != 0) 
                    ;
                    // Undefined, but is an error.
            });

            //
            // Preventing over/underflow.
            // Corner case exists in Mu.
            int idx = pd_list.size();

            pd_list.push_back(std::move(uniq_pd));
            pdinfo_map.insert(std::pair<std::string, size_t>(arg_pd_name, idx));
            // Map: <K, V> = <"Name", index_val>

            return true;
        }

        struct ibv_mr* doCreateMr(std::string arg_pd_name, void* arg_addr, size_t arg_len, int arg_rights) {
            if (!isPdRegistered(arg_pd_name))
                return nullptr;

            return ibv_reg_mr(
                pd_list.at(getIdx(arg_pd_name)).get(),
                arg_addr, 
                arg_len,
                static_cast<int>(arg_rights));
            ;
        }
    };


}
