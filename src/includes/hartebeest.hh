#pragma once
/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hartebeest.hh
 */

#include <cstdint>
#include <vector>

#include "./hb_retcode.hh"
#include "./hb_logger.hh"

#include "./hb_alloc.hh"
#include "./hb_cache.hh"
#include "./hb_cfgldr.hh"
#include "./hb_hca.hh"
#include "./hb_mrs.hh"
#include "./hb_cqs.hh"
#include "./hb_pds.hh"

namespace hartebeest {

    // Funcs
    class HartebeestCore {
    public:
        std::vector<PdCache> pd_cache_vec;
        std::vector<BasicCqCache> cq_cache_vec;
    
    public:
        HartebeestCore();
        ~HartebeestCore();

        bool validation_test();

        void create_pd();
        void destroy_pd();

        void create_cq();
        void destroy_cq();

        static HartebeestCore& get_instance() {
            static HartebeestCore core;
            return core;
        }
    };
}

