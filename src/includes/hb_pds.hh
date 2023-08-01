#pragma once
/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_pds.hh
 */

#include <cstdint>
#include <string>
#include <functional>

#include <map>
#include <vector>

#include <infiniband/verbs.h> // OFED IB verbs

#include "./hb_retcode.hh"
#include "./hb_logger.hh"

#include "./hb_hca.hh"
#include "./hb_cache.hh"
#include "./hb_mrs.hh"
#include "./hb_qps.hh"


namespace hartebeest {

    const uint32_t NAME_STR_MAX = 128;
    
    class Qp; // Somewhere.
    class Pd {
    private:
        std::string name;     // Human readable

        Hca* hca_ln = nullptr;
        struct ibv_pd* pd = nullptr;

        ResourceCache<Mr> mr_cache;
        ResourceCache<Qp> qp_cache;

    public:
        Pd(const char*, Hca&);
        ~Pd();

        struct ibv_pd* get_pd();
        Hca* get_hca();

        hb_retcode create_mr(const char*, size_t, int);
        hb_retcode create_qp(const char*, enum ibv_qp_type, struct ibv_cq*, struct ibv_cq*);
        
        ResourceCache<Mr>& get_mr_cache();
        ResourceCache<Qp>& get_qp_cache();
    };

    class PdCache : public ResourceCache<Pd> {
    public:
        PdCache(const char*);
        static PdCache& get_instance() {
            static PdCache global_pdcache("SINGLETON_PD_CACHE");
            return global_pdcache;
        }
    };
}

#define HB_PD_CACHE  hartebeest::PdCache::get_instance()