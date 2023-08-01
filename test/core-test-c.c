

#include <stdio.h>
#include <stdlib.h>

#include "../src/includes/hartebeest-c.h"

// export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/build/lib
// gcc -o build/bin/core-test-c test/core-test-c.c -libverbs -lmemcached -L ./build/lib/ -lhartebeest

int main() {

    const char* node0_pd_name = "newcons-pd-0";
    const char* node1_pd_name = "newcons-pd-1";

    const char* node0_mr_name = "newcons-mr-0";
    const char* node1_mr_name = "newcons-mr-1";

    const char* node0_qp_name = "newcons-qp-0";
    const char* node1_qp_name = "newcons-qp-1";

    char* this_node_pd_name;
    char* other_node_pd_name;

    char* this_node_mr_name;
    char* other_node_mr_name;
    
    char* this_node_qp_name;
    char* other_node_qp_name;

    hartebeest_init();
    int node_id = atoi(hartebeest_get_sysvar("HARTEBEEST_NID"));

    if (node_id == 0) {
        this_node_pd_name = node0_pd_name;
        other_node_pd_name = node1_pd_name;

        this_node_mr_name = node0_mr_name;
        other_node_mr_name = node1_mr_name;
        
        this_node_qp_name = node0_qp_name;
        other_node_qp_name = node1_qp_name;
    }
    else if (node_id == 1) {
        this_node_pd_name = node1_pd_name;
        other_node_pd_name = node0_pd_name;

        this_node_mr_name = node1_mr_name;
        other_node_mr_name = node0_mr_name;
        
        this_node_qp_name = node1_qp_name;
        other_node_qp_name = node0_qp_name;
    }

    hartebeest_create_local_pd(this_node_pd_name);
    hartebeest_create_local_mr(
        this_node_pd_name, this_node_mr_name, 
        8192, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);

    hartebeest_memc_push_local_mr(
        this_node_mr_name, this_node_pd_name, this_node_mr_name);

    hartebeest_memc_fetch_remote_mr(other_node_mr_name);

    hartebeest_create_basiccq("send-cq");
    hartebeest_create_basiccq("recv-cq");

    hartebeest_create_local_qp(
        this_node_pd_name, this_node_qp_name, "send-cq", "recv-cq"
    );

    hartebeest_init_local_qp(this_node_pd_name, this_node_qp_name);
    hartebeest_memc_push_local_qp(
        this_node_qp_name, this_node_pd_name, this_node_qp_name
    );

    hartebeest_memc_fetch_remote_qp(other_node_qp_name);
    hartebeest_connect_local_qp(
        this_node_pd_name, 
        this_node_qp_name,
        other_node_qp_name
    );

    if (node_id == 0) {

        struct ibv_qp* local_qp = hartebeest_get_local_qp(
            this_node_pd_name, this_node_qp_name);

        struct ibv_mr* local_mr = hartebeest_get_local_mr(
            this_node_pd_name, this_node_mr_name
        );

        struct ibv_mr* remote_mr = hartebeest_get_remote_mr(
            other_node_mr_name
        );

        printf("Remote addr: %p\n", remote_mr->addr);

        char* target = local_mr->addr;
        char* source_str = "PAYLOAD STRING";

        memcpy(target, source_str, strlen(source_str));
        size_t buflen = strlen(source_str);

        hartebeest_rdma_post_single_fast(
            local_qp, local_mr->addr, remote_mr->addr, buflen, IBV_WR_RDMA_WRITE,
                local_mr->lkey, remote_mr->rkey, 0
        );

        hartebeest_rdma_poll("send-cq");

        hartebeest_memc_wait_general("general-memc-got");
        hartebeest_memc_del_general("general-memc-got");
    }
    else {
        char* written = hartebeest_get_local_mr(
            this_node_pd_name, this_node_mr_name)->addr;

        while (strcmp(written, "PAYLOAD STRING") != 0) {
            
        }
        printf("Detected\n");

        hartebeest_memc_push_general("general-memc-got");
    }

    return 0;
}