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
        int nid;
        int hca_idx;

        // Manages remote resources.
        ResourceCache<Mr> remote_mr_cache;
        ResourceCache<Qp> remote_qp_cache;
        
    public:
        HartebeestCore(int = 0, uint8_t = 1);
        ~HartebeestCore();

        bool validation_test();
        
        bool memc_push_general(const char*);
        bool memc_wait_general(const char*);
        bool memc_del_general(const char*);


        void init();

        bool create_local_pd(const char*);

        // MR interfaces
        bool create_local_mr(const char*, const char*, size_t, int);
        bool memc_push_local_mr(const char*, const char*, const char*);
        bool memc_fetch_remote_mr(const char*);

        bool create_basiccq(const char*);

        bool create_local_qp(const char*, const char*, enum ibv_qp_type, const char*, const char*);
        bool init_local_qp(const char*, const char*);
        bool connect_local_qp(const char*, const char*, const char*);
        bool memc_push_local_qp(const char*, const char*, const char*);
        bool memc_fetch_remote_qp(const char*);

        bool rdma_post_single_fast(struct ibv_qp*, void*, void*, size_t, 
            enum ibv_wr_opcode, uint32_t, uint32_t, uint64_t);
        bool rdma_post_single_signaled_inline(struct ibv_qp*, void*, void*, size_t, 
            enum ibv_wr_opcode, uint32_t, uint32_t, uint64_t);

        bool rdma_poll(const char*);
        bool rdma_send_poll(struct ibv_qp*);
        bool rdma_recv_poll(struct ibv_qp*);
        
        // bool is_pd_exist(const char*);
        // bool is_mr_exist(const char*, const char*, const char*);
        // bool is_cq_exist(const char*);
        // bool is_qp_exist(const char*, const char*);

        // Access 
        Pd* get_local_pd(const char*);
        Mr* get_local_mr(const char*, const char*);
        // BasicCq* get_local_basiccq(const char*);
        Qp* get_local_qp(const char*, const char*);

        Mr* get_remote_mr(const char*);
        Qp* get_remote_qp(const char*);

        char* get_sysvar(const char*);
        int get_nid();

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