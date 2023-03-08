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

    enum MemoryRights {
        LOCAL_READ = 0,
        LOCAL_WRITE = IBV_ACCESS_LOCAL_WRITE,
        REMOTE_READ = IBV_ACCESS_REMOTE_READ,
        REMOTE_WRITE = IBV_ACCESS_REMOTE_WRITE
    };

    struct MemoryRegion {
        uintptr_t addr;
        uint64_t size;
        uint32_t lkey;
        uint32_t rkey;
    };

    class MrManager {
    private:
        buffer_info_t bufinfo_map; // <index, length>
        buffer_list_t buf_list;

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

        size_t getIdx(std::string arg_mr_name) {
            return bufinfo_map.find(arg_mr_name)->second.first;
        }

        size_t getLen(std::string arg_mr_name) {
            return bufinfo_map.find(arg_mr_name)->second.second;
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

        void* getAddress(std::string arg_buf_name) {
            uint8_t* addr = buf_list.at(getIdx(arg_buf_name)).get();
            return addr;
        }
    };
}
