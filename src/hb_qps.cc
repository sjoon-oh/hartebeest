/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_qps.cc
 */

#include <string>
#include <sstream>
#include <cstdlib>

#include <memory>
#include <cassert>

#include <iostream>

#include <infiniband/verbs.h> // OFED IB verbs

#include "./includes/hb_retcode.hh"
#include "./includes/hb_logger.hh"

#include "./includes/hb_cfgldr.hh"
#include "./includes/hb_pds.hh"
#include "./includes/hb_qps.hh"

namespace hartebeest {
    std::string prefix_key(enum ibv_qp_type connect_type, const char* key) {
        if (connect_type == IBV_QPT_RC)
            return (std::string("rc:").append(key));
        
        else if (connect_type == IBV_QPT_UC)
            return (std::string("uc:").append(key));

        else if (connect_type == IBV_QPT_UD)
            return (std::string("ud:").append(key));

        else
            assert(false);

        return nullptr;
    }

    const int get_pref_attr(enum ibv_qp_type connect_type, const char* key) {
        return *HB_CFG_LOADER.get_attr(prefix_key(connect_type, key).c_str());
    }
}

hartebeest::Qp::Qp(const char* id, enum ibv_qp_type connect_type, hartebeest::Pd* inv_pd, struct ibv_cq* sq, struct ibv_cq* rq) {
    name = std::string(id);

    // Assumed to be local
    if (inv_pd != nullptr) {
        
        pos_type = hartebeest::QP_TYPE_LOCAL;
        conn_type = connect_type;

        assert(sq != nullptr);
        assert(rq != nullptr);

        pid = inv_pd->get_hca()->get_device_pid();
        plid = inv_pd->get_hca()->get_device_plid();

        struct ibv_qp_init_attr init_qp_attr;
        std::memset(&init_qp_attr, 0, sizeof(struct ibv_qp_init_attr));

        init_qp_attr.qp_type = connect_type;
        init_qp_attr.cap.max_send_wr = get_pref_attr(conn_type, "cap.max_send_wr");
        init_qp_attr.cap.max_recv_wr = get_pref_attr(conn_type, "cap.max_recv_wr");
        init_qp_attr.cap.max_send_sge = get_pref_attr(conn_type, "cap.max_send_sge");
        init_qp_attr.cap.max_recv_sge = get_pref_attr(conn_type, "cap.max_recv_sge");
        init_qp_attr.cap.max_inline_data = get_pref_attr(conn_type, "cap.max_inline_data");

        init_qp_attr.send_cq = sq;
        init_qp_attr.recv_cq = rq;

        qp = ibv_create_qp(inv_pd->get_pd(), &init_qp_attr);
        uintptr_t qp_addr = reinterpret_cast<uintptr_t>(qp);

        HB_CLOGGER->info("New QP({}, 0x{:x}), at {} state", name, qp_addr, query_state());

        assert(qp != nullptr);
    }
    else {
        // If involved pd is nullptr, then it is manually generating this container.
        pos_type = hartebeest::QP_TYPE_UNKNOWN;
    }
}

hartebeest::Qp::~Qp() {
    // HB_CLOGGER->info("Before ~QP({})", name);
    if (qp != nullptr)
        ibv_destroy_qp(qp);

    // HB_CLOGGER->info("Destroyed QP({})", name);
}

void hartebeest::Qp::set_type(enum hartebeest::QpType qp_type) {
    pos_type = qp_type;
}

bool hartebeest::Qp::is_qp_created() const {
    return (qp != nullptr);
}

int hartebeest::Qp::query_state() {
    struct ibv_qp_attr query_attr;
    struct ibv_qp_init_attr query_init_attr;

    assert(pos_type == QP_TYPE_LOCAL);

    // https://www.rdmamojo.com/2013/01/19/ibv_query_qp/
    if (ibv_query_qp(qp, &query_attr,
            IBV_QP_STATE, &query_init_attr)) { 
                HB_CLOGGER->warn("QP query somehow failed.");
    }

    return query_attr.qp_state;
}

hb_retcode hartebeest::Qp::transit_init() {

    assert(qp != nullptr);
    assert(pos_type == QP_TYPE_LOCAL);

    struct ibv_qp_attr qp_attr;
    std::memset(&qp_attr, 0, sizeof(struct ibv_qp_attr));

    qp_attr.qp_state = IBV_QPS_INIT;
    qp_attr.pkey_index = 0;

    qp_attr.port_num = pid;
    qp_attr.qp_access_flags = 
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;

    int ret = ibv_modify_qp(
        this->qp, &qp_attr, 
        IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS);

    if (ret != 0) {
        HB_CLOGGER->warn("Transit INIT QP({}), returned {}, should be 0", name, ret);
        return hb_retcode(hartebeest::QP_TRANSITION_2_INIT_ERR);
    }

    return hb_retcode(hartebeest::QP_TRANSITION_2_INIT_OK);
}

hb_retcode hartebeest::Qp::transit_rtr(hartebeest::Qp* remote_qp) {

    assert(qp != nullptr);
    assert(pos_type == QP_TYPE_LOCAL);

    struct ibv_qp_attr conn_attr;
    std::memset(&conn_attr, 0, sizeof(struct ibv_qp_attr));

    conn_attr.qp_state = IBV_QPS_RTR;
    conn_attr.path_mtu = static_cast<enum ibv_mtu>(get_pref_attr(conn_type, "path_mtu"));
    conn_attr.rq_psn = get_pref_attr(conn_type, "rq_psn");
    conn_attr.dest_qp_num = remote_qp->get_qp()->qp_num;

    conn_attr.ah_attr.is_global = get_pref_attr(conn_type, "ah_attr.is_global");
    conn_attr.ah_attr.sl = get_pref_attr(conn_type, "ah_attr.sl");
    conn_attr.ah_attr.src_path_bits = get_pref_attr(conn_type, "ah_attr.src_path_bits");
    
    conn_attr.ah_attr.port_num = remote_qp->get_pid();
    conn_attr.ah_attr.dlid = remote_qp->get_plid();

    conn_attr.max_dest_rd_atomic =  get_pref_attr(conn_type, "max_dest_rd_atomic");
    conn_attr.min_rnr_timer =  get_pref_attr(conn_type, "min_rnr_timer");

    HB_CLOGGER->info("Connecting to: qpn {:x}, pid {:x}, plid {:x}", conn_attr.dest_qp_num, conn_attr.ah_attr.port_num, conn_attr.ah_attr.dlid);

    int rtr_flags = 
        IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
        IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;

    int ret = ibv_modify_qp(
        this->qp, &conn_attr, rtr_flags);

    if (ret != 0) {
        HB_CLOGGER->warn("Transit RTR QP({}), returned {}, should be 0", name, ret);
        return hb_retcode(QP_TRANSITION_2_RTR_ERR);
    }

    return hb_retcode(QP_TRANSITION_2_RTR_OK);
}

hb_retcode hartebeest::Qp::transit_rts() {

    assert(qp != nullptr);
    assert(pos_type == QP_TYPE_LOCAL);

    struct ibv_qp_attr conn_attr;
    std::memset(&conn_attr, 0, sizeof(struct ibv_qp_attr));

    conn_attr.qp_state = IBV_QPS_RTS;
    conn_attr.sq_psn = get_pref_attr(conn_type, "sq_psn");

    conn_attr.timeout = get_pref_attr(conn_type, "timeout");
    conn_attr.retry_cnt = get_pref_attr(conn_type, "retry_cnt");
    conn_attr.rnr_retry = get_pref_attr(conn_type, "rnr_retry");
    conn_attr.max_rd_atomic = get_pref_attr(conn_type, "max_rd_atomic");
    conn_attr.max_dest_rd_atomic = get_pref_attr(conn_type, "max_dest_rd_atomic");

    int rts_flags = 
        IBV_QP_STATE | IBV_QP_SQ_PSN | IBV_QP_TIMEOUT |
        IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_MAX_QP_RD_ATOMIC;

    int ret = ibv_modify_qp(
        this->qp, &conn_attr, rts_flags);

    if (ret != 0) {
        HB_CLOGGER->warn("Transit RTR QP({}), returned {}, should be 0", name, ret);
        return hb_retcode(QP_TRANSITION_2_RTS_ERR);
    }

    return hb_retcode(QP_TRANSITION_2_RTS_OK);
}

struct ibv_qp* hartebeest::Qp::get_qp() {
    return qp;
}

uint8_t hartebeest::Qp::get_pid() {
    return pid;
}

uint16_t hartebeest::Qp::get_plid() {
    return plid;
}

std::string hartebeest::Qp::flatten_info() {
    std::ostringstream stream;

    int connect_type = conn_type;

    assert(qp != nullptr);
    stream
        << name << ":"
        << std::hex << qp->qp_num << ":"
        << static_cast<int>(pid) << ":"
        << plid << ":"
        << connect_type;
    
    return stream.str();
}

void hartebeest::Qp::unflatten_info(const char* fetch) {
    assert(qp == nullptr);

    std::string imported(fetch);

    std::replace(imported.begin(), imported.end(), ':', ' ');
    std::stringstream stream(imported);
    
    qp = reinterpret_cast<struct ibv_qp*>(std::malloc(sizeof(struct ibv_qp)));
    std::memset(qp, 0, sizeof(struct ibv_qp));

    int large_pid, large_conn_type;

    stream >> name;
    stream >> std::hex >> qp->qp_num;
    stream >> std::hex >> large_pid;
    stream >> std::hex >> plid;
    stream >> std::hex >> large_conn_type;

    // Assuming port number do not overflow.
    pid = large_pid;
    conn_type = static_cast<enum ibv_qp_type>(large_conn_type);
}

