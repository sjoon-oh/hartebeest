/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hartebeest.cc 
 */

#include <unistd.h>
#include <stdio.h>

#include <infiniband/verbs.h> // OFED IB verbs

#include "./includes/hartebeest-c.h"
#include "./includes/hartebeest.hh"

void hartebeest_init() {
    HARTEBEEST_CORE_HDL;
}

char* hartebeest_get_sysvar(const char* envvar) {
    return HARTEBEEST_CORE_HDL.get_sysvar(envvar);
}

bool hartebeest_memc_push_general(const char* msg_key) {
    return HARTEBEEST_CORE_HDL.memc_push_general(msg_key);
}

bool hartebeest_memc_wait_general(const char* msg_key) {
    return HARTEBEEST_CORE_HDL.memc_wait_general(msg_key);
}

bool hartebeest_memc_del_general(const char* msg_key) {
    return HARTEBEEST_CORE_HDL.memc_del_general(msg_key);
}

bool hartebeest_create_local_pd(const char* pd_key) {
    return HARTEBEEST_CORE_HDL.create_local_pd(pd_key);
}

bool hartebeest_create_local_mr(const char* pd_key, const char* mr_key, size_t buflen, int rights) {
    return HARTEBEEST_CORE_HDL.create_local_mr(pd_key, mr_key, buflen, rights);
}

bool hartebeest_memc_push_local_mr(const char* memc_key, const char* pd_key, const char* mr_key) {
    return HARTEBEEST_CORE_HDL.memc_push_local_mr(memc_key, pd_key, mr_key);
}

bool hartebeest_memc_fetch_remote_mr(const char* remote_mr_key) {
    return HARTEBEEST_CORE_HDL.memc_fetch_remote_mr(remote_mr_key);
}

bool hartebeest_create_basiccq(const char* cq_key) {
    return HARTEBEEST_CORE_HDL.create_basiccq(cq_key);
}

bool hartebeest_create_local_qp(const char* pd_key,const char* qp_key, enum ibv_qp_type conn, const char* sendcq_key, const char* recvcq_key) {
    return HARTEBEEST_CORE_HDL.create_local_qp(pd_key, qp_key, conn, sendcq_key, recvcq_key);
}

bool hartebeest_init_local_qp(const char* pd_key, const char* qp_key) {
    return HARTEBEEST_CORE_HDL.init_local_qp(pd_key, qp_key);
}

bool hartebeest_connect_local_qp(const char* pd_key, const char* local_qp_key, const char* remote_qp_key) {
    return HARTEBEEST_CORE_HDL.connect_local_qp(pd_key, local_qp_key, remote_qp_key);
}

bool hartebeest_memc_push_local_qp(const char* memc_key, const char* pd_key, const char* qp_key) {
    return HARTEBEEST_CORE_HDL.memc_push_local_qp(memc_key, pd_key, qp_key);
}

bool hartebeest_memc_fetch_remote_qp(const char* remote_qp_key) {
    return HARTEBEEST_CORE_HDL.memc_fetch_remote_qp(remote_qp_key);
}

bool hartebeest_rdma_post_single_fast(
        struct ibv_qp* local_qp, void* local_addr, void* remote_addr, size_t len, 
        enum ibv_wr_opcode opcode, uint32_t lkey, uint32_t rkey, uint64_t work_id) {
    return HARTEBEEST_CORE_HDL.rdma_post_single_fast(local_qp, local_addr, remote_addr, len, opcode, lkey, rkey, work_id);
}

bool hartebeest_rdma_post_single_signaled_inline(
        struct ibv_qp* local_qp, void* local_addr, void* remote_addr, size_t len, 
        enum ibv_wr_opcode opcode, uint32_t lkey, uint32_t rkey, uint64_t work_id) {
    return HARTEBEEST_CORE_HDL.rdma_post_single_signaled_inline(local_qp, local_addr, remote_addr, len, opcode, lkey, rkey, work_id);
}

bool hartebeest_rdma_poll(const char* cq_key) {
    return HARTEBEEST_CORE_HDL.rdma_poll(cq_key);
}

bool hartebeest_rdma_send_poll(struct ibv_qp* qp) {
    return HARTEBEEST_CORE_HDL.rdma_send_poll(qp);
}

bool hartebeest_rdma_recv_poll(struct ibv_qp* qp) {
    return HARTEBEEST_CORE_HDL.rdma_recv_poll(qp);
}

struct ibv_pd* hartebeest_get_local_pd(const char* pd_key) {
    return HARTEBEEST_CORE_HDL.get_local_pd(pd_key)->get_pd();
}

struct ibv_mr* hartebeest_get_local_mr(const char* pd_key, const char* mr_key) {
    return HARTEBEEST_CORE_HDL.get_local_mr(pd_key, mr_key)->get_mr();
}

struct ibv_qp* hartebeest_get_local_qp(const char* pd_key, const char* qp_key) {
    return HARTEBEEST_CORE_HDL.get_local_qp(pd_key, qp_key)->get_qp();
}

struct ibv_mr* hartebeest_get_remote_mr(const char* remote_mr_key) {
    return HARTEBEEST_CORE_HDL.get_remote_mr(remote_mr_key)->get_mr();
}

struct ibv_qp* hartebeest_get_remote_qp(const char* remote_qp_key) {
    return HARTEBEEST_CORE_HDL.get_remote_qp(remote_qp_key)->get_qp();
}







