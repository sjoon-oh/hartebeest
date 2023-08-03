#pragma once

#include <infiniband/verbs.h> // OFED IB verbs
#ifdef __cplusplus

#include <cstdint>
#include <cstddef>

extern "C" {
#else

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#endif

void hartebeest_init();

char* hartebeest_get_sysvar(const char*);

bool hartebeest_memc_push_general(const char*);
bool hartebeest_memc_wait_general(const char*);
bool hartebeest_memc_del_general(const char*);

bool hartebeest_create_local_pd(const char*);

bool hartebeest_create_local_mr(const char*, const char*, size_t, int);
bool hartebeest_memc_push_local_mr(const char*, const char*, const char*);
bool hartebeest_memc_fetch_remote_mr(const char*);

bool hartebeest_create_basiccq(const char*);

bool hartebeest_create_local_qp(const char*, const char*, enum ibv_qp_type, const char*, const char*);
bool hartebeest_init_local_qp(const char*, const char*);
bool hartebeest_connect_local_qp(const char*, const char*, const char*);
bool hartebeest_memc_push_local_qp(const char*, const char*, const char*);
bool hartebeest_memc_fetch_remote_qp(const char*);

bool hartebeest_rdma_post_single_fast(struct ibv_qp*, void*, void*, size_t, 
    enum ibv_wr_opcode, uint32_t, uint32_t, uint64_t);

bool hartebeest_rdma_poll(const char*);
bool hartebeest_rdma_send_poll(struct ibv_qp*);
bool hartebeest_rdma_recv_poll(struct ibv_qp*);

struct ibv_pd* hartebeest_get_local_pd(const char*);
struct ibv_mr* hartebeest_get_local_mr(const char*, const char*);
// BasicCq* get_local_basiccq(const char*);
struct ibv_qp* hartebeest_get_local_qp(const char*, const char*);

struct ibv_mr* hartebeest_get_remote_mr(const char*);
struct ibv_qp* hartebeest_get_remote_qp(const char*);

#ifdef __cplusplus
}
#endif