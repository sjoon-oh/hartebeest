/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * conn-test.cpp
 * 
 * Project hartebeest is a RDMA(IB) connector,
 *  a refactored version from Mu.
 */

#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"

#define LOG_ENABLED
#include "../src/rdma-conf.hpp"

#include <cstring>
#include <cstdlib>

int main() {

    spdlog::set_pattern("[%H:%M:%S] [%L] [thread %t] %v");

    hartebeest::RdmaConfigurator            confr2;
    hartebeest::ConfigFileExchanger         exchr;

    struct hartebeest::RdmaNetworkContext   actx;
    struct hartebeest::NodeContext          *this_ctx, *other_ctx;
    struct hartebeest::Pd                   *this_pd, *other_pd;
    struct hartebeest::Mr                   *this_mr, *other_mr;
    struct hartebeest::Qp                   *this_qp, *other_qp;

    const int sample_buf_size = 1024 * 1024 * 1024;   // 1GB

    std::string funcn = "";    
    
    char* my_mr_addr = nullptr;
    std::string buf_dummy{"I am hartebeest dummy."};

    int ret;
    uint8_t* buf = nullptr;

    int this_node_id = 0;
    int other_node_id = 0;

    char mr_id_template = 'a';

#define __TEST__(X)     if ((X)) ; else goto FAIL;
#define __OK__(X)       spdlog::info("[OK]{}", (X));

    //
    // Initialize device
    funcn = "RdmaConfigurator::doInitDevice()";
    __TEST__((ret = confr2.doInitDevice2()) == 0)

    // Load configuration file
    funcn = "ConfigFileExchanger::doReadConfigFile()";
    __TEST__((ret = exchr.doReadConfigFile()) == true)
    __OK__(funcn)

    //
    // Allocate PD Test
    funcn = "RdmaConfigurator::doRegisterPd()";
    __TEST__((ret = confr2.doRegisterPd2('a')) == 0)
    __TEST__((ret = confr2.doRegisterPd2('b')) == 0)
    __OK__(funcn)

    //
    // Allocate Buffer & Register MR test
    for (int i = 0; i < 24; i++) {

        __TEST__((buf = confr2.doAllocateBuffer2(sample_buf_size, 64)) != nullptr)
        __TEST__((ret = confr2.doCreateAndRegisterMr2('a', mr_id_template, buf, sample_buf_size)) == 0)

        std::memset(buf, 0, sample_buf_size);

        spdlog::info("Created MR: ID[{}], sized: {}", mr_id_template, sample_buf_size);
        mr_id_template++;
    }

    funcn = "RdmaConfigurator::doAllocateBuffer()";
    __OK__(funcn)

    funcn = "RdmaConfigurator::doCreateAndRegisterMr()";
    __OK__(funcn)

    mr_id_template = 'a';

#define SEND_CQ_1_ID   1
#define RECV_CQ_1_ID   2
#define SEND_CQ_2_ID   3
#define RECV_CQ_2_ID   4

    funcn = "RdmaConfigurator::doCreateAndRegisterCq()";
    __TEST__((ret = confr2.doCreateAndRegisterCq2(SEND_CQ_1_ID)) == 0) 
    __TEST__((ret = confr2.doCreateAndRegisterCq2(SEND_CQ_2_ID)) == 0) 
    __TEST__((ret = confr2.doCreateAndRegisterCq2(RECV_CQ_1_ID)) == 0) 
    __TEST__((ret = confr2.doCreateAndRegisterCq2(RECV_CQ_2_ID)) == 0)
    __OK__(funcn) 

    funcn = "RdmaConfigurator::doCreateAndRegisterRcQp()";
    __TEST__((ret = confr2.doCreateAndRegisterRcQp2('a', 'a', SEND_CQ_1_ID, RECV_CQ_1_ID)) == 0)
    __TEST__((ret = confr2.doCreateAndRegisterRcQp2('b', 'b', SEND_CQ_2_ID, RECV_CQ_2_ID)) == 0)
    __OK__(funcn)

    // 
    // Initialize QP
    funcn = "RdmaConfigurator::doInitQp()";
    __TEST__((ret = confr2.doInitQp2('a')) == 0) 
    __TEST__((ret = confr2.doInitQp2('b')) == 0) 
    __OK__(funcn)
    //
    // Export
    funcn = "RdmaConfigurator::doExportAll()";
    __TEST__((ret = confr2.doExportAll2(
            exchr.getThisNodeId()
        )) == 0)
    __OK__(funcn)

    funcn = "ConfigFileExchanger::setThisNodeConf()";
    __TEST__((ret = exchr.setThisNodeConf()) == true)
    __OK__(funcn)

    if (exchr.getThisNodeRole() == hartebeest::ROLE_SERVER) {
        spdlog::info("Playing server role.");
        this_node_id = 0;
        other_node_id = 1;
    }
    
    else {
        spdlog::info("Playing client role.");
        this_node_id = 1;
        other_node_id = 0;
    }

    funcn = "ConfigFileExchanger::doExchange()";
    __TEST__((ret = exchr.doExchange()) == true)
    __OK__(funcn)
    
    funcn = "ConfigFileExchanger::doExchange()";
    actx = exchr.doExportNetworkContext();
    __OK__(funcn)

    // 
    // Sample Linear Search: Find node info.
    for (uint32_t i = 0; i < actx.num_nodes; i++) {
        if (actx.nodes[i].nid == other_node_id)
            other_ctx = &(actx.nodes[i]);
        if (actx.nodes[i].nid == this_node_id)
            this_ctx = &(actx.nodes[i]);
    }

    // Find Pd.
    for (uint32_t pi = 0; pi < other_ctx->num_pds; pi++) {
        if (other_ctx->pds[pi].pd_id == 'a') { other_pd = &(other_ctx->pds[pi]); break; }
    }

    for (uint32_t pi = 0; pi < this_ctx->num_pds; pi++) {
        if (this_ctx->pds[pi].pd_id == 'a') {this_pd = &(this_ctx->pds[pi]); break; }
    }

    // Find Mr
    for (uint32_t mi = 0; mi < other_pd->num_mrs; mi++) {
        if (other_pd->mrs[mi].mr_id == 'a') { other_mr = &(other_pd->mrs[mi]); break; }
    }

    for (uint32_t mi = 0; mi < this_pd->num_mrs; mi++) {
        if (this_pd->mrs[mi].mr_id == 'a') { this_mr = &(this_pd->mrs[mi]); break; }
    }

    // 
    for (uint32_t qi = 0; qi < other_pd->num_qps; qi++) {
        if (other_pd->qps[qi].qp_id == 'a') {
            other_qp = &(other_pd->qps[qi]);
            break;
        }
    }

    for (uint32_t qi = 0; qi < this_pd->num_qps; qi++) {
        if (this_pd->qps[qi].qp_id == 'a') {
            this_qp = &(this_pd->qps[qi]);
            break;
        }
    }
    
    //
    // Connect to the other node.

    // In this test, QPs with identical name will be connected.
    // QP connection test.
    funcn = "RdmaConfigurator::doConnectRcQp()";
    __TEST__((ret = confr2.doConnectRcQp2('a', other_qp->pid, other_qp->qpn, other_qp->plid)) == 0)
    __OK__(funcn)
    
    if (exchr.getThisNodeRole() == hartebeest::ROLE_CLIENT) {

        // my_mr_addr = (char*)confr2.getMr('a')->addr;
        my_mr_addr = (char*)this_mr->addr;
        do {
            spdlog::info("'a' Read at [{}]: {}", 
                (uintptr_t)my_mr_addr, 
                (char*)my_mr_addr);
            
            sleep(1);
        } while ((std::string(my_mr_addr) != buf_dummy));

        spdlog::info("RDMA Write success. String detected: {}", (char*)my_mr_addr);
        std::memset((void*)my_mr_addr, 0, confr2.getMr('a')->length); // Buffer reset.
    }
    else {
        
        // Server waits for a second.
        sleep(1);

        //
        // Server sends.
        std::memcpy(
            // (void*)confr2.getMr('a')->addr,                 // Destination
            (void*)this_mr->addr,
            buf_dummy.c_str(),                              // Source
            buf_dummy.size()                                // Size
        );

        spdlog::info("Dummy string copied: {}", (char*)this_mr->addr);

        // Connected to each 'qp-1's
        funcn = "RdmaConfigurator::doRdmaWrite()";
        spdlog::info(
            "RDMA WRITE to [{}] at [{}]", 
            other_node_id, 
            other_mr->addr);

        __TEST__(confr2.doRdmaWrite2('a', this_mr->addr, this_mr->length, confr2.getMr('a')->lkey, other_mr->addr, other_mr->rkey) == 0)
        __OK__(funcn)

        std::memset((void*)this_mr->addr, 0, this_mr->length);
    }

    sleep(1);

    // RDMA Read test.
    if (exchr.getThisNodeRole() == hartebeest::ROLE_CLIENT) {

        buf_dummy.clear();
        buf_dummy = "Read me if you can";

        sleep(1);

        spdlog::info("RDMA Read at[{}] of the server.", other_mr->addr);

        funcn = "RdmaConfigurator::doRdmaRead()";
        do {
            confr2.doRdmaRead2('a', this_mr->addr, 100, confr2.getMr('a')->lkey, other_mr->addr, other_mr->rkey);
            sleep(0.2);

        } while ((std::string((char*)this_mr->addr) != buf_dummy));

        spdlog::info("RDMA Read success. String detected: ({})", (char*)this_mr->addr);       

    }
    else {

        buf_dummy.clear();
        buf_dummy = "Read me if you can";

        std::memcpy(
            (void*)this_mr->addr,  // Destination
            buf_dummy.c_str(),                              // Source
            buf_dummy.size()                                // Size
        );

        sleep(5); // Give some time to fetch.
        // Server copies dummy string.
    }


    spdlog::info("End reached.");

    exchr.doCleanNetworkContext(actx);

    return 0;

FAIL:
    spdlog::error("[FAIL]{}", funcn);

    return -1;
}