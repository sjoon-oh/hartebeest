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
    
    HARTEBEEST_CORE_HDL.create_pd("hartebeest-pd-1");

    // Register the MR to PD name
    HARTEBEEST_CORE_HDL.create_mr("hartebeest-pd-1", "mr-1", 8192, 0);
    HARTEBEEST_CORE_HDL.create_mr("hartebeest-pd-1", "mr-2", 8192, 0);



    return 0;
}