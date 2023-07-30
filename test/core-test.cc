/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * configurator test
 */

#include <cstring>
#include <iostream>

#include "../extern/spdlog/spdlog.h"
#include "../src/includes/hartebeest.hh"

int main() {
    
    HARTEBEEST_CORE_HDL.init();
    
    HARTEBEEST_CORE_HDL.create_local_pd("hartebeest-pd-1");

    // Register the MR to PD name
    HARTEBEEST_CORE_HDL.create_local_mr("hartebeest-pd-1", "mr-1", 8192, 0);
    HARTEBEEST_CORE_HDL.create_local_mr("hartebeest-pd-1", "mr-2", 8192, 0);

    // Create QP
    // HARTEBEEST_CORE_HDL.create_cq();
    HARTEBEEST_CORE_HDL.pd_status();
    HARTEBEEST_CORE_HDL.mr_status("hartebeest-pd-1");

    HARTEBEEST_CORE_HDL.cq_status();
    HARTEBEEST_CORE_HDL.qp_status("hartebeest-pd-1");

    hartebeest::Mr* local_mr_1 = nullptr;
    local_mr_1 = HARTEBEEST_CORE_HDL.get_local_mr("hartebeest-pd-1", "mr-1");

    HARTEBEEST_MEMC_HDL.prefix_set("mr-1-node0", hartebeest::HB_MEMC_KEY_PREF_MRINFO, local_mr_1->flatten_mr().c_str());
    




    return 0;
}