/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_qps.cc
 */

#include <string>
#include <memory>
#include <cassert>

#include <iostream>

#include <infiniband/verbs.h> // OFED IB verbs

#include "./includes/hb_retcode.hh"
#include "./includes/hb_logger.hh"

#include "./includes/hb_cfgldr.hh"
#include "./includes/hb_pds.hh"
#include "./includes/hb_qps.hh"

hartebeest::Qp::Qp(const char* id, hartebeest::Pd* inv_pd, struct ibv_cq* sq, struct ibv_cq* rq) {
    name = std::string(id);

    assert(inv_pd != nullptr);
    assert(sq != nullptr);
    assert(rq != nullptr);

    involv = inv_pd;

    struct ibv_qp_init_attr init_qp_attr;
    std::memset(&init_qp_attr, 0, sizeof(struct ibv_qp_init_attr));

    init_qp_attr.qp_type = static_cast<enum ibv_qp_type>(*HB_CFG_LOADER.get_attr("qp_type"));
    init_qp_attr.cap.max_send_wr = *HB_CFG_LOADER.get_attr("cap.max_send_wr");
    init_qp_attr.cap.max_recv_wr = *HB_CFG_LOADER.get_attr("cap.max_recv_wr");
    init_qp_attr.cap.max_send_sge = *HB_CFG_LOADER.get_attr("cap.max_send_sge");
    init_qp_attr.cap.max_recv_sge = *HB_CFG_LOADER.get_attr("cap.max_recv_sge");
    init_qp_attr.cap.max_inline_data = *HB_CFG_LOADER.get_attr("cap.max_inline_data");

    // std::cout << *HB_CFG_LOADER.get_attr("qp_type") << "\n";
    // std::cout << *HB_CFG_LOADER.get_attr("cap.max_send_wr") << "\n";
    // std::cout << *HB_CFG_LOADER.get_attr("cap.max_recv_wr") << "\n";
    // std::cout << *HB_CFG_LOADER.get_attr("cap.max_recv_sge") << "\n";
    // std::cout << *HB_CFG_LOADER.get_attr("cap.max_send_sge") << "\n";

    init_qp_attr.send_cq = sq;
    init_qp_attr.recv_cq = rq;

    qp = ibv_create_qp(involv->get_pd(), &init_qp_attr);
    uintptr_t qp_addr = reinterpret_cast<uintptr_t>(qp);

    HB_CLOGGER->info("New QP({}, 0x{:x}), at {} state", name, qp_addr, query_state());

    assert(qp != nullptr);
}

hartebeest::Qp::~Qp() {
    // HB_CLOGGER->info("Before ~QP({})", name);
    if (qp != nullptr)
        ibv_destroy_qp(qp);

    // HB_CLOGGER->info("Destroyed QP({})", name);
}

bool hartebeest::Qp::is_qp_created() const {
    return (qp != nullptr);
}

bool hartebeest::Qp::is_involved() const {
    return (involv != nullptr);
}

int hartebeest::Qp::query_state() {
    struct ibv_qp_attr query_attr;
    struct ibv_qp_init_attr query_init_attr;

    // https://www.rdmamojo.com/2013/01/19/ibv_query_qp/
    if (ibv_query_qp(qp, &query_attr,
            IBV_QP_STATE, &query_init_attr)) { 
                HB_CLOGGER->warn("QP query somehow failed.");
    }

    return query_attr.qp_state;
}

hb_retcode hartebeest::Qp::transit_init() {

    struct ibv_qp_attr qp_attr;
    std::memset(&qp_attr, 0, sizeof(struct ibv_qp_attr));

    qp_attr.qp_state = IBV_QPS_INIT;
    qp_attr.pkey_index = 0;

    qp_attr.port_num = involv->get_hca()->get_device_pid();
    qp_attr.qp_access_flags = 
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;

    int ret = ibv_modify_qp(
        this->qp, &qp_attr, 
        IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS);

    if (ret != 0) {
        HB_CLOGGER->info("Transit INIT QP({}), returned {}, should be 0", name, ret);
        return hb_retcode(hartebeest::QP_TRANSITION_2_INIT_ERR);
    }

    return hb_retcode(hartebeest::QP_TRANSITION_2_INIT_OK);
}

struct ibv_qp* hartebeest::Qp::get_qp() {
    return qp;
}

