#pragma once
/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_cqs.hh
 */

#include <string>

#include <infiniband/verbs.h> // OFED IB verbs

#include "./hb_retcode.hh"
#include "./hb_logger.hh"

#include "./hb_hca.hh"
#include "./hb_cache.hh"

namespace hartebeest {

    class BasicCq {
    private:
        std::string name;

        Hca* hca_ln = nullptr;
        struct ibv_cq* cq = nullptr;

    public:
        BasicCq(const char*, Hca&);
        ~BasicCq();

        struct ibv_cq* get_cq();
    };

    class BasicCqCache : public ResourceCache<BasicCq> {
    public:
        BasicCqCache(const char*);
        static BasicCqCache& get_instance() {
            static BasicCqCache global_basiccq_cache("SINGLETON_BASICCQ_CACHE");
            return global_basiccq_cache;
        }
    };
}

#define HB_BASICCQ_CACHE  hartebeest::BasicCqCache::get_instance()