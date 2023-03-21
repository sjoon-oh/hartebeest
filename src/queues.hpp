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

#include <cstring>
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

    // PDM Return codes.
    // This class do not use throws.
    enum {
        QM_NOERROR = 0,
        QM_ERROR_GENERAL = 0x30,
        QM_ERROR_KEY_EXIST,
        QM_ERROR_KEY_NOEXIST,
        QM_ERROR_QP_CREATE,
        QM_ERROR_CQ_CREATE,
        QM_ERROR_INITQP,
        QM_ERROR_GENERAL_MODQP,
        QM_ERRROR_NULL_CONTEXT
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
        std::map<uint32_t, struct ibv_cq*>          cq_dbm; // Completion Queue DB map
        std::map<uint32_t, struct ibv_qp*>          qp_dbm; // Queue Pair DB map


        // Fixed vars.
        const int       default_cq_depth = 128;
        const int       default_qp_wr_depth = 128;
        const int       default_qp_sge_depth = 16;
        const int       default_qp_max_inlining = 256;

    public:
        QueueManager() { }
        ~QueueManager() {

            for (auto& elem: cq_dbm) ibv_destroy_cq(elem.second);
            for (auto& elem: qp_dbm) ibv_destroy_qp(elem.second);
        }

        //
        // The inteface naming convention is designed to have:
        //  - do** : These are management functions. 
        //      Does something important. Directly updates its member. 
        //  - is** : Check status.
        //  - get** : Returns reference/value of a member. 
        //
        //  Members follow the underscore, methods follow the CamelCase naming convention.
        bool isCqRegistered2(uint32_t arg_cq_id) {
            if (cq_dbm.find(arg_cq_id) != cq_dbm.end())
                return true;
            return false;
        }

        bool isQpRegistered2(uint32_t arg_qp_id) {
            if (qp_dbm.find(arg_qp_id) != qp_dbm.end())
                return true;
            return false;
        }

        struct ibv_cq* getCq2(uint32_t arg_cq_id) {
            if (!isCqRegistered2(arg_cq_id))
                return nullptr;

            return cq_dbm.find(arg_cq_id)->second;
        }

        struct ibv_qp* getQp2(uint32_t arg_qp_id) {
            if (!isQpRegistered2(arg_qp_id))
                return nullptr;

            return qp_dbm.find(arg_qp_id)->second;
        }

        std::map<uint32_t, struct ibv_qp*>& getQpMap() { return qp_dbm; }

        std::vector<struct ibv_qp*> getAssociatedQps(struct ibv_pd* arg_pd) {
            
            std::vector<struct ibv_qp*> ret;
            for (auto& elem: qp_dbm) {
                if (elem.second->pd == arg_pd) ret.push_back(elem.second);
            }

            return ret;
        }

        //
        // All of the core interface starts with prefix 'do'.
        // doRegisterCq does the following:
        //  - Creates CQ with name arg_cq_name.
        //  - Associates with IB context arg_ctx.
        //  - Registers to management container, cq_list and cqinfo_map.
        //
        int doRegisterCq2(uint32_t arg_cq_id, struct ibv_context* arg_ctx) {

            // Simple verification. It should not have the same key.
            if (isCqRegistered2(arg_cq_id)) 
                return QM_ERROR_KEY_EXIST;
            
            // Creates CQ.
            auto cq = ibv_create_cq(arg_ctx, default_cq_depth, nullptr, nullptr, 0);

            if (cq == nullptr) 
                return QM_ERROR_CQ_CREATE;

            cq_dbm.insert(std::pair<uint32_t, struct ibv_cq*>(arg_cq_id, cq));

            return QM_NOERROR;
        }

        int doCreateAndRegisterRcQp2(
                uint32_t        arg_qp_id, 
                struct ibv_pd*  arg_pd,
                uint32_t        arg_send_cq_id,
                uint32_t        arg_recv_cq_id
            ) {
            
            struct ibv_qp_init_attr iq;

            //
            // Verifications.
            //  Should not be already registered.
            if (isQpRegistered2(arg_qp_id)) return QM_ERROR_KEY_EXIST;
            
            //
            // Should have been registered.
            if (!isCqRegistered2(arg_send_cq_id)) return QM_ERROR_KEY_NOEXIST;
            if (!isCqRegistered2(arg_recv_cq_id)) return QM_ERROR_KEY_NOEXIST;

            std::memset(&iq, 0, sizeof(iq));

            iq.qp_type                  = IBV_QPT_RC;
            iq.cap.max_send_wr          = default_qp_wr_depth;
            iq.cap.max_recv_wr          = default_qp_wr_depth;
            iq.cap.max_send_sge         = default_qp_sge_depth;
            iq.cap.max_recv_sge         = default_qp_sge_depth;
            iq.cap.max_inline_data      = default_qp_max_inlining;

            iq.send_cq                  = getCq2(arg_send_cq_id);
            iq.recv_cq                  = getCq2(arg_recv_cq_id);

            //
            // Creates Queue Pair.
            auto qp = ibv_create_qp(arg_pd, &iq);
            if (qp == nullptr)  
                return QM_ERROR_QP_CREATE;

            qp_dbm.insert(std::pair<uint32_t, struct ibv_qp*>(arg_qp_id, qp));

            return QM_NOERROR;
        }

    
        // 
        // Does QP transitions to RESET state.
        // bool doQpReset(std::string arg_qp_name) {
        //     if (!isQpRegistered(arg_qp_name)) return false;

        //     struct ibv_qp_attr attr;
        //     memset(&attr, 0, sizeof(attr));

        //     // Sets attribute qp_state to RESET.
        //     attr.qp_state = IBV_QPS_RESET;

        //     // Turn!
        //     auto ret = ibv_modify_qp(qp_list.at(getQpIdx(arg_qp_name)).get(), &attr, IBV_QP_STATE);

        //     if (ret != 0)
        //         return false;

        //     return true;
        // }

        //
        // Initializes QP. 
        // Note that right after the creation of a QP, it stays at RESET state.
        // doInitQp makes the transition to INIT state.
        int doInitQp2(uint32_t arg_qp_id, int arg_port_id) {

            struct ibv_qp_attr init_attr;
            memset(&init_attr, 0, sizeof(struct ibv_qp_attr));

            init_attr.qp_state          = IBV_QPS_INIT;
            init_attr.pkey_index        = 0;
            init_attr.port_num          = static_cast<uint8_t>(arg_port_id);
            init_attr.qp_access_flags   = 
                IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE; // Local Read + ...

            auto ret = ibv_modify_qp(
                getQp2(arg_qp_id), 
                &init_attr,
                IBV_QP_STATE | IBV_QP_PKEY_INDEX 
                    | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS);

            if (ret != 0) return QM_ERROR_INITQP;
            return QM_NOERROR;

        }

        int doConnectRemoteRcQp2(
            uint32_t        arg_qp_id,
            int             arg_remote_port_id,
            uint32_t        arg_remote_qpn,
            uint16_t        arg_remote_lid
        ) {
            struct ibv_qp_attr connect_attr;
            memset(&connect_attr, 0, sizeof(struct ibv_qp_attr));

            connect_attr.qp_state           = IBV_QPS_RTR;
            connect_attr.path_mtu           = IBV_MTU_4096;
            connect_attr.rq_psn             = 3185;

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

            auto ret = ibv_modify_qp(getQp2(arg_qp_id), &connect_attr, rtr_flags);
            if (ret != 0) return QM_ERROR_GENERAL_MODQP;

            memset(&connect_attr, 0, sizeof(struct ibv_qp_attr));
            connect_attr.qp_state   = IBV_QPS_RTS;
            connect_attr.sq_psn     = 3185;

            connect_attr.timeout        = 14;
            connect_attr.retry_cnt      = 7;
            connect_attr.rnr_retry      = 7;
            connect_attr.max_rd_atomic      = 16;
            connect_attr.max_dest_rd_atomic = 16;

            int rts_flags = IBV_QP_STATE | IBV_QP_SQ_PSN | IBV_QP_TIMEOUT |
                  IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_MAX_QP_RD_ATOMIC;

            ret = ibv_modify_qp(getQp2(arg_qp_id), &connect_attr, rts_flags);
            if (ret != 0) return QM_ERROR_GENERAL_MODQP;

            return QM_NOERROR;
        }


        // bool doQpTransition() {

        //     return true;
        // }
        
    };
}
