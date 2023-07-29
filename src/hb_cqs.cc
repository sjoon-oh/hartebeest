/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_cqs.cc
 */

#include <string>
#include <iostream>

#include <infiniband/verbs.h>

// #include "./hb_retcode.hh"
// #include "./hb_logger.hh"

#include "./includes/hb_cfgldr.hh"
#include "./includes/hb_cqs.hh"

hartebeest::BasicCq::BasicCq(const char* name, hartebeest::Hca& hca_dev) {
    hca_ln = &hca_dev;

    HB_CLOGGER->info("Ctor, CQ: {}", name);

    struct ibv_context* hca_ctx = hca_dev.get_device_ctx();
    // int* cq_depth = ConfigLoader::get_instance().get_attr("cq_depth");
    
    int* cq_depth = HB_CFG_LOADER.get_attr("cq_depth");
    assert(cq_depth != nullptr);
    
    cq = ibv_create_cq(hca_ctx, *cq_depth, nullptr, nullptr, 0);
}

hartebeest::BasicCq::~BasicCq() {
    if (cq != nullptr)
        ibv_destroy_cq(cq);
}

struct ibv_cq* hartebeest::BasicCq::get_cq() {
    return cq;
}

hartebeest::BasicCqCache::BasicCqCache(const char* name) : ResourceCache<BasicCq>(name) {
    
}



