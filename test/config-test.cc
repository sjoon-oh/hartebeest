/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * configurator test
 */

#include <cstring>
#include <iostream>

#include "../extern/spdlog/spdlog.h"
#include "../src/includes/hartebeest.hh"

int main() {

    // Retcodes
    hartebeest::Retcode retcode;

    HB_CLOGGER->info("-- Configurator Test Start --");

    hartebeest::ConfigLoader cfg_ldr = hartebeest::ConfigLoader::get_instance();
    retcode = cfg_ldr.init_sysvars();

    HB_CLOGGER->info("{}", retcode.aux_str);

    retcode = cfg_ldr.init_params();
    HB_CLOGGER->info("{}", retcode.aux_str);

    int* test_val = cfg_ldr.get_attr("qp_type");
    HB_CLOGGER->info("Attribute qp_type value found: {}", *test_val);

    test_val = cfg_ldr.get_attr("cq_depth");
    HB_CLOGGER->info("Attribute cq_depth value found: {}", *test_val);

    test_val = cfg_ldr.get_attr("ah_attr.src_path_bits");
    HB_CLOGGER->info("Attribute ah_attr.src_path_bits value found: {}", *test_val);

    test_val = cfg_ldr.get_attr("cap.max_send_wr");
    HB_CLOGGER->info("Attribute cap.max_send_wr value found: {}", *test_val);

    test_val = cfg_ldr.get_attr("timeout");
    HB_CLOGGER->info("Attribute timeout value found: {}", *test_val);

    return 0;
}