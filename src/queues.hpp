#pragma once

/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * queues.hpp
 * 
 * Project hartebeest is a RDMA(IB) connector,
 *  a refactored version from Mu.
 */

#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <functional>

#include <infiniband/verbs.h> // OFED IB verbs

#include <iostream>

#include "common.hpp"


namespace hartebeest {

    /* The QueueManager managers all ibv-related queues.
     * These include: 
     *  - The list of Completion Queues (CQs)
     *  - The list of Queue Pairs (QPs)
     * 
     * The class does the following:
     *  - Creation of a Completion Queue
     *  - Creation of a Queue Pair
     */

    struct RcInfo {


    };

    struct UdInfo {

    };

    //
    // Class QueueManager
    class QueueManager {
    private:

        //
        // Management containers:
        //
        // cqinfo_map inserts <K, V>. The key is a string (Completion Queue Name), and the
        //  value is an index value of cq_list.
        // cq_list holds unique_ptr type (with a deleter). 
        //
        // qpinfo_map inserts <K, V>. The key is a string (Queue Pair Name), and the
        //  value is an index value of qp_list.
        // qp_list holds unique_ptr type (with a deleter).
        //
        // A user can distinguish multiple CQs and QPs with their name.

        std::map<std::string, size_t>               cqinfo_map;
        std::vector<del_unique_ptr<struct ibv_cq>>  cq_list{};

        std::map<std::string, size_t>               qpinfo_map;
        std::vector<del_unique_ptr<struct ibv_qp>>  qp_list{};

        // Fixed vars.
        const int                   cq_depth = 128;

        struct ibv_qp_init_attr     qp_init_attr;
        const int                   qp_wr_depth = 128;
        const int                   qp_sge_depth = 16;
        const int                   qp_max_inlining = 256;

    public:
        QueueManager() {

            // The intiial attribute of a queue pair is predefined here,
            //  but may be updated in a program execution. 
            //  Never trust these values.

            qp_init_attr.qp_type            = IBV_QPT_RC;
            qp_init_attr.cap.max_send_wr    = qp_wr_depth;
            qp_init_attr.cap.max_recv_wr    = qp_wr_depth;
            qp_init_attr.cap.max_send_sge   = qp_sge_depth;
            qp_init_attr.cap.max_recv_sge   = qp_sge_depth;
            qp_init_attr.cap.max_inline_data = qp_max_inlining;
        }

        ~QueueManager() {
            // std::cout << "~QueueManager\n";
        }

        //
        // The inteface naming convention is designed to have:
        //  - do** : These are management functions. 
        //      Does something important. Directly updates its member. 
        //  - is** : Check status.
        //  - get** : Returns reference/value of a member. 
        //
        //  Members follow the underscore, methods follow the CamelCase naming convention.
        bool isCqRegistered(std::string arg_cq_name) {
            if (cqinfo_map.find(arg_cq_name) != cqinfo_map.end())
                return true;
            return false;
        }

        bool isQpRegistered(std::string arg_qp_name) {
            if (qpinfo_map.find(arg_qp_name) != qpinfo_map.end())
                return true;
            return false;
        }

        size_t getCqIdx(std::string arg_cq_name) {
            return cqinfo_map.find(arg_cq_name)->second;
        }
        
        struct ibv_cq* getCq(std::string arg_cq_name) {
            if (!isCqRegistered(arg_cq_name))
                return nullptr;

            return cq_list.at(getCqIdx(arg_cq_name)).get();
        }     

        size_t getQpIdx(std::string arg_qp_name) {
            return qpinfo_map.find(arg_qp_name)->second;
        }

        struct ibv_qp* getQp(std::string arg_qp_name) {
            if (!isQpRegistered(arg_qp_name))
                return nullptr;

            return qp_list.at(getQpIdx(arg_qp_name)).get();
        }

        //
        // All of the core interface starts with prefix 'do'.
        // doRegisterCq does the following:
        //  - Creates CQ with name arg_cq_name.
        //  - Associates with IB context arg_ctx.
        //  - Registers to management container, cq_list and cqinfo_map.
        //
        bool doRegisterCq(std::string arg_cq_name, struct ibv_context* arg_ctx) {

            // Simple verification. It should not have the same key.
            if (isCqRegistered(arg_cq_name)) 
                return false;
            
            // Creates CQ.
            auto cq = ibv_create_cq(
                arg_ctx, 
                cq_depth, 
                nullptr,   
                nullptr, 
                0);

            if (cq == nullptr) return false;

            del_unique_ptr<struct ibv_cq> uniq_cq(cq, [](struct ibv_cq *arg_cq) {
                auto ret = ibv_destroy_cq(arg_cq);
                if (ret != 0) {
                    // Do nothing, for now.
                }
            });

            //
            // Preventing over/underflow.
            // Corner case exists in Mu.
            int idx = cq_list.size();
            
            cq_list.push_back(std::move(uniq_cq));
            cqinfo_map.insert(std::pair<std::string, size_t>(arg_cq_name, idx));

            return true;
        }


        bool doCreateAndRegisterQp(
                std::string arg_qp_name, 
                struct ibv_pd* arg_pd,
                std::string arg_send_cq_name,
                std::string arg_recv_cq_name
            ) {
            
            // Verifications.
            //  Should not be already registered.
            if (isQpRegistered(arg_qp_name)) return false;
            
            // Should have been registered.
            if (!isCqRegistered(arg_send_cq_name)) return false;
            if (!isCqRegistered(arg_recv_cq_name)) return false;

            qp_init_attr.send_cq = getCq(arg_send_cq_name);
            qp_init_attr.recv_cq = getCq(arg_recv_cq_name);

            //
            // Creates Queue Pair.
            auto qp = ibv_create_qp(arg_pd, &qp_init_attr);
            if (qp == nullptr) return false;

            auto uniq_qp = del_unique_ptr<struct ibv_qp>(qp, [](struct ibv_qp* arg_qp) {
                    auto ret = ibv_destroy_qp(arg_qp);
                    if (ret != 0) {
                        // Nothing, for now.
                    }
            });

            int idx = qp_list.size();

            qp_list.push_back(std::move(uniq_qp));
            qpinfo_map.insert(
                std::pair<std::string, size_t>(arg_qp_name, idx)
            );

            return true;
        }
    
        // 
        // Does QP transitions to RESET state.
        bool doQpReset(std::string arg_qp_name) {
            if (!isQpRegistered(arg_qp_name)) return false;

            struct ibv_qp_attr attr;
            memset(&attr, 0, sizeof(attr));

            // Sets attribute qp_state to RESET.
            attr.qp_state = IBV_QPS_RESET;

            // Turn!
            auto ret = ibv_modify_qp(
                qp_list.at(getQpIdx(arg_qp_name)).get(), 
                &attr, 
                IBV_QP_STATE
                );

            if (ret != 0)
                return false;

            return true;
        }

        //
        // Initializes QP. 
        // Note that right after the creation of a QP, it stays at RESET state.
        // doInitQp makes the transition to INIT state.
        bool doInitQp(std::string arg_qp_name, int arg_port_id) {

            struct ibv_qp_attr init_attr;
            memset(&init_attr, 0, sizeof(struct ibv_qp_attr));

            init_attr.qp_state          = IBV_QPS_INIT;
            init_attr.pkey_index        = 0;
            init_attr.port_num          = static_cast<uint8_t>(arg_port_id);
            init_attr.qp_access_flags   = 0 | IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE; // Local Read + ...

            auto ret = ibv_modify_qp(
                getQp(arg_qp_name), 
                &init_attr,
                IBV_QP_STATE | IBV_QP_PKEY_INDEX 
                    | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS);

            if (ret != 0) return false;
            return true;
        }

        bool doConnectRemoteRcQp(
            std::string arg_qp_name,
            int arg_remote_port_id,
            uint32_t arg_remote_qpn,
            uint16_t arg_remote_lid
        ) {
            struct ibv_qp_attr connect_attr;
            memset(&connect_attr, 0, sizeof(struct ibv_qp_attr));

            connect_attr.qp_state   = IBV_QPS_RTR;
            connect_attr.path_mtu   = IBV_MTU_4096;
            connect_attr.rq_psn     = 3185;

            connect_attr.ah_attr.is_global  = 0;
            connect_attr.ah_attr.sl         = 0;
            connect_attr.ah_attr.src_path_bits  = 0;
            connect_attr.ah_attr.port_num   = static_cast<uint8_t>(arg_remote_port_id);

            connect_attr.dest_qp_num    = arg_remote_qpn;
            connect_attr.ah_attr.dlid   = arg_remote_lid;

            connect_attr.max_dest_rd_atomic     = 16;
            connect_attr.min_rnr_timer          = 12;

            int rtr_flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
                IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC |
                IBV_QP_MIN_RNR_TIMER;

            auto ret = ibv_modify_qp(
                getQp(arg_qp_name),
                &connect_attr,
                rtr_flags
            );

            if (ret != 0) return false;

            memset(&connect_attr, 0, sizeof(struct ibv_qp_attr));
            connect_attr.qp_state   = IBV_QPS_RTS;
            connect_attr.sq_psn     = 3185;

            connect_attr.timeout    = 14;
            connect_attr.retry_cnt  = 7;
            connect_attr.rnr_retry  = 7;
            connect_attr.max_rd_atomic      = 16;
            connect_attr.max_dest_rd_atomic = 16;

            int rts_flags = IBV_QP_STATE | IBV_QP_SQ_PSN | IBV_QP_TIMEOUT |
                  IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_MAX_QP_RD_ATOMIC;

            ret = ibv_modify_qp(
                getQp(arg_qp_name),
                &connect_attr,
                rts_flags
            );

            if (ret != 0) return false;

            return true;
        }

        bool doQpTransition() {

            return true;
        }

        

        
    };
}
