#pragma once
/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hartebeest.hh
 */

#include <cstdint>

#include <infiniband/verbs.h> // OFED IB verbs

#include "./hb_retcode.hh"
#include "./hb_logger.hh"

#include "./hb_hca.hh"
#include "./hb_alloc.hh"
#include "./hb_cache.hh"
#include "./hb_cfgldr.hh"
#include "./hb_mrs.hh"
#include "./hb_cqs.hh"
#include "./hb_pds.hh"

#include "./hb_memc.hh"

namespace hartebeest {

    // Funcs
    class HartebeestCore {
    private:
        int hca_idx;
    
    public:
        HartebeestCore(int = 0);
        ~HartebeestCore();

        bool validation_test();

        void init();

        bool create_local_pd(const char*);
        void destroy_pd();

        bool create_local_mr(const char*, const char*, size_t, int);
        void destroy_mr();

        bool create_basiccq(const char*);
        void destroy_basiccq();

        bool is_pd_exist(const char*);
        bool is_mr_exist(const char*, const char*);
        bool is_cq_exist(const char*);
        bool is_qp_exist(const char*, const char*);

        // Access 
        Pd* get_local_pd(const char*);
        Mr* get_local_mr(const char*, const char*);
        BasicCq get_local_basiccq(const char*);
        Qp* get_local_qp(const char*, const char*);

        // Debug
        void pd_status();
        void mr_status(const char*);
        void cq_status();
        void qp_status(const char*);

        static HartebeestCore& get_instance() {
            static HartebeestCore core;
            return core;
        }
    };
}

#define HARTEBEEST_CORE_HDL     hartebeest::HartebeestCore::get_instance()
#define HARTEBEEST_MEMC_HDL     HB_EXCHANGER