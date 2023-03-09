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

#include "common.hpp"
#include "prot-domain.hpp"

#include <iostream>

namespace hartebeest {

    struct Mr {
        uintptr_t   addr;
        uint64_t    size;
        uint32_t    lkey;
        uint32_t    rkey;
    };

    class MrManager {
    private:
        std::map<std::string, std::pair<size_t, size_t>> bufinfo_map; // <index, length>
        std::vector<std::unique_ptr<uint8_t[], DeleteAligned<uint8_t>>> buf_list;

        std::map<std::string, size_t> mrinfo_map;
        std::vector<del_unique_ptr<struct ibv_mr>> mr_list{};

    public:
        MrManager() {};
        ~MrManager() {
            std::cout << "~MrManager()\n";
        };

        bool isBufferAllocated(std::string arg_buf_name) {
            if (bufinfo_map.find(arg_buf_name) != bufinfo_map.end())
                return true;
            return false;
        }

        bool isMrRegistered(std::string arg_mr_name) {
            if (mrinfo_map.find(arg_mr_name) != mrinfo_map.end())
                return true;
            return false;
        }

        bool isAllMrRegistered() {
            return buf_list.size() == mr_list.size();
        }

        size_t getBufferIdx(std::string arg_mr_name) {
            return bufinfo_map.find(arg_mr_name)->second.first;
        }

        size_t getBufferLen(std::string arg_mr_name) {
            return bufinfo_map.find(arg_mr_name)->second.second;
        }

        void* getBufferAddress(std::string arg_buf_name) {
            uint8_t* addr = buf_list.at(getBufferIdx(arg_buf_name)).get();
            return addr;
        }

        size_t getMrIdx(std::string arg_mr_name) {
            return mrinfo_map.find(arg_mr_name)->second;
        }

        struct ibv_mr* getMr(std::string arg_mr_name) {
            if (!isMrRegistered(arg_mr_name)) {
                return nullptr;
            }

            return mr_list.at(getMrIdx(arg_mr_name)).get();            
        }

        uint32_t getMrLocalKey(std::string arg_mr_name) {
            struct ibv_mr* mr = getMr(arg_mr_name);
            return mr == nullptr ? 0 : mr->lkey;
        }

        uint32_t getMrRemoteKey(std::string arg_mr_name) {
            struct ibv_mr* mr = getMr(arg_mr_name);
            return mr == nullptr ? 0 : mr->rkey;
        }

        bool doAllocateBuffer(std::string arg_buf_name, size_t arg_len, int arg_align) {
            if (isBufferAllocated(arg_buf_name))
                return false;

            std::unique_ptr<uint8_t[], DeleteAligned<uint8_t>> 
                data(allocate_aligned<uint8_t>(arg_align, arg_len));
            
            memset(data.get(), 0, arg_len);

            int idx = buf_list.size();
            buf_list.push_back(std::move(data));

            std::pair<size_t, size_t> index_length(idx, arg_len);
            bufinfo_map.insert(
                std::pair<std::string, std::pair<size_t, size_t>>(arg_buf_name, index_length));

            return true;
        }

        bool doRegisterMr2(std::string arg_mr_name, struct ibv_mr* arg_mr) {

            if (isMrRegistered(arg_mr_name)) {
                return false;
            }

            del_unique_ptr<struct ibv_mr> uniq_mr(arg_mr, [](struct ibv_mr *arg_mr) {
                auto ret = ibv_dereg_mr(arg_mr);
                if (ret != 0) 
                    ;
            });

            int idx = mr_list.size();

            mr_list.push_back(std::move(uniq_mr));
            mrinfo_map.insert(std::pair<std::string, size_t>(arg_mr_name, idx));

            return true;
        }




    };
}
