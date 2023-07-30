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

    retcode = HB_CFG_LOADER.init_sysvars();

    HB_CLOGGER->info("{}", retcode.aux_str);

    retcode = HB_CFG_LOADER.init_params();
    HB_CLOGGER->info("{}", retcode.aux_str);

    HB_CLOGGER->info("-- RDMA Resource Alloc Test Start --");

    // hartebeest::HcaInitializer::get_instance();
    int n_hca = hartebeest::HcaInitializer::get_instance().get_n_hca_devs();

    HB_CLOGGER->info("HCA #: {}", n_hca);

    const int default_hca_idx = 0;
    if (n_hca > 0)
        retcode = hartebeest::HcaInitializer::get_instance().open_device(default_hca_idx);
    HB_CLOGGER->info("{}", retcode.aux_str);
    
    retcode = hartebeest::HcaInitializer::get_instance().bind_port(0);
    HB_CLOGGER->info("{}", retcode.aux_str);
    // hartebeest::PdCache::get_instance();

    hartebeest::Pd* single_pd = new hartebeest::Pd(
            "SINGLE-PD",
            hartebeest::HcaInitializer::get_instance().get_hca(default_hca_idx)
        );
    // hartebeest::PdCache::get_instance().register_resrc("SINGLE-PD", single_pd);
    HB_PD_CACHE.register_resrc("SINGLE-PD", single_pd);

    retcode = single_pd->create_mr("TEST-MR", 8192 * 4, 
        0 | IBV_ACCESS_LOCAL_WRITE | 
        IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
    // single_pd->create_mr();

    HB_CLOGGER->info("{}", retcode.aux_str);

    hartebeest::BasicCq* basic_cq_1 = new hartebeest::BasicCq(
            "BCQ1", 
            hartebeest::HcaInitializer::get_instance().get_hca(default_hca_idx));

    hartebeest::BasicCq* basic_cq_2 = new hartebeest::BasicCq(
        "BCQ2", 
        hartebeest::HcaInitializer::get_instance().get_hca(default_hca_idx));
    
    HB_BASICCQ_CACHE.register_resrc("BCQ1", basic_cq_1);
    HB_BASICCQ_CACHE.register_resrc("BCQ2", basic_cq_2);

    HB_BASICCQ_CACHE.out_cache_status();    
    HB_PD_CACHE.out_cache_status();

    HB_PD_CACHE.get_resrc("SINGLE-PD")->get_mr_cache().out_cache_status();
    HB_PD_CACHE.get_resrc("SINGLE-PD")->create_qp("QP1", basic_cq_1->get_cq(), basic_cq_2->get_cq());

    // auto qp_inst = hartebeest::PdCache::get_instance().get_resrc("SINGLE-PD")->get_qp_cache().get_resrc("QP1");
    auto qp_inst = HB_PD_CACHE.get_resrc("SINGLE-PD")->get_qp_cache().get_resrc("QP1");

    if (qp_inst == nullptr)
        HB_CLOGGER->info("get Not found");
    else {
        retcode = qp_inst->transit_init();
        HB_CLOGGER->info("{}", retcode.aux_str);
    }
    
    return 0;
}