/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * configurator test
 */

#include <string>

#include <cstring>
#include <iostream>

#include "../extern/spdlog/spdlog.h"
#include "../src/includes/hartebeest.hh"

int main() {

    HARTEBEEST_CORE_HDL.init();
    int node_id = HARTEBEEST_CORE_HDL.get_nid();
    
    HB_CLOGGER->info("Node ID: {}", node_id);

    std::string global_pd_name = "newcons-pd-0";
    std::string global_mr_name = "newcons-mr-0";
    std::string global_qp_name = "newcons-qp-0";
    
    std::string other_mr_name = "newcons-mr-1";
    std::string other_qp_name = "newcons-qp-1";

    if (node_id == 1) {
        global_pd_name = "newcons-pd-1";
        global_mr_name = "newcons-mr-1";
        global_qp_name = "newcons-qp-1";

        other_mr_name = "newcons-mr-0";
        other_qp_name = "newcons-qp-0";
    }

    HARTEBEEST_CORE_HDL.create_local_pd(global_pd_name.c_str());;

    // Register the MR to PD name
    HARTEBEEST_CORE_HDL.create_local_mr(global_pd_name.c_str(), global_mr_name.c_str(), 8192, 
        0 | IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE
    );

    HARTEBEEST_CORE_HDL.memc_push_local_mr(
        global_mr_name.c_str(), global_pd_name.c_str(), global_mr_name.c_str()
    );
    
    HARTEBEEST_CORE_HDL.memc_fetch_remote_mr(other_mr_name.c_str());

    // HB_CLOGGER->info("Fetched flatten MR: {}", result);

    HARTEBEEST_CORE_HDL.create_basiccq("newcons-basiccq-1");
    HARTEBEEST_CORE_HDL.create_basiccq("newcons-basiccq-2");

    HARTEBEEST_CORE_HDL.create_local_qp(
        global_pd_name.c_str(), global_qp_name.c_str(), IBV_QPT_RC, "newcons-basiccq-1", "newcons-basiccq-2"
    );

    HARTEBEEST_CORE_HDL.init_local_qp(
        global_pd_name.c_str(),
        global_qp_name.c_str()
    );

    HARTEBEEST_CORE_HDL.memc_push_local_qp(
        global_qp_name.c_str(), global_pd_name.c_str(), global_qp_name.c_str());

    HARTEBEEST_CORE_HDL.memc_fetch_remote_qp(other_qp_name.c_str());

    HARTEBEEST_CORE_HDL.connect_local_qp(
        global_pd_name.c_str(),
        global_qp_name.c_str(),
        other_qp_name.c_str()
    );

    if (node_id == 0) {

        // auto local_mr = HARTEBEEST_CORE_HDL.get_local_mr(
        //         global_pd_name.c_str(),
        //         global_mr_name.c_str()
        //     )->get_mr();

        // auto remote_mr = HARTEBEEST_CORE_HDL
        // struct ibv_qp* local_qp, void* local_addr, void* remote_addr, size_t len, 
        // enum ibv_wr_opcode opcode, uint32_t lkey, uint32_t rkey
        auto local_qp = HARTEBEEST_CORE_HDL.get_local_qp(
                global_pd_name.c_str(),
                global_qp_name.c_str()
            )->get_qp();

        auto local_mr = HARTEBEEST_CORE_HDL.get_local_mr(
                global_pd_name.c_str(),
                global_mr_name.c_str()
            )->get_mr();

        auto remote_mr = HARTEBEEST_CORE_HDL.get_remote_mr(
                other_mr_name.c_str()
            )->get_mr();

        char* target = reinterpret_cast<char*>(local_mr->addr);
        char* source_str = "PAYLOAD STRING";
        std::memcpy(
            target,
            source_str,
            std::strlen(source_str)
        );
        // *target = 98;
        size_t buflen = std::strlen(source_str);

        HARTEBEEST_CORE_HDL.rdma_post_single_fast(
            local_qp, local_mr->addr, remote_mr->addr, buflen, 
            IBV_WR_RDMA_WRITE, local_mr->lkey, remote_mr->rkey, 0
        );

        HARTEBEEST_CORE_HDL.rdma_poll(
            "newcons-basiccq-1"
        );

        HARTEBEEST_CORE_HDL.memc_wait_general("general-memc-got");
        HARTEBEEST_CORE_HDL.memc_del_general("general-memc-got");
    }
    else {
        char* written = reinterpret_cast<char*>(HARTEBEEST_CORE_HDL.get_local_mr(global_pd_name.c_str(), global_mr_name.c_str())->get_buffer());

        HB_CLOGGER->info("To be written address: 0x{:x}", reinterpret_cast<uintptr_t>(written));
        while (std::strcmp(written, "PAYLOAD STRING") != 0) {
            
        }
        HB_CLOGGER->info("Catched: {}", written);
        HARTEBEEST_CORE_HDL.memc_push_general("general-memc-got");
    }

    

    HB_CLOGGER->info("Test end.");



    return 0;
}