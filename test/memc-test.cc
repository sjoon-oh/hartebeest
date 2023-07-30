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
    HARTEBEEST_MEMC_HDL.prefix_set(
        "node0", 
        hartebeest::HB_MEMC_KEY_PREF_INIT, 
        "AAAABBBBCCCCDDDD");

    std::string return_val;
    HARTEBEEST_MEMC_HDL.prefix_get(
        "node0",
        hartebeest::HB_MEMC_KEY_PREF_INIT, 
        return_val
    );

    HB_CLOGGER->info("Return from Memcached: {}", return_val);

    return 0;
}