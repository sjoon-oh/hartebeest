/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hartebeest.cc 
 */

#include <unistd.h>
#include <iostream>

#include "./includes/hartebeest.hh"

hartebeest::HartebeestCore::HartebeestCore(int idx, uint8_t port_num) : hca_idx(idx) {

    hb_retcode hb_rc = HB_CFG_LOADER.init_sysvars();
    hb_rc = HB_CFG_LOADER.init_params();

    nid = std::stoi(HB_CFG_LOADER.get_sysvar("HARTEBEEST_NID"));

    int n_hca = HB_HCA_INITR.get_n_hca_devs();

    assert(n_hca > 0);
    hb_rc = HB_HCA_INITR.open_device(idx);
    HB_CLOGGER->info("{}", hb_rc.aux_str);

    hb_rc = HB_HCA_INITR.bind_port(idx, port_num);
    HB_CLOGGER->info("{}", hb_rc.aux_str);
}

hartebeest::HartebeestCore::~HartebeestCore() {    
    HB_CLOGGER->info("Hartebeest core end");
}

bool hartebeest::HartebeestCore::memc_push_general(const char* msg_key) {

    const char* empty_str = "  ";
    hb_retcode hb_rc;
    hb_rc = HARTEBEEST_MEMC_HDL.set(msg_key, empty_str);

    if (hb_rc.ret_code == MEMCH_SET_OK)
        return true;
    
    return false;
}

bool hartebeest::HartebeestCore::memc_wait_general(const char* msg_key) {

    int retry_cnt = 0;

    std::string fetched;
    HB_CLOGGER->info("Message wait: {}", msg_key);

    while (HARTEBEEST_MEMC_HDL.get(msg_key, fetched).ret_code != MEMCH_GET_OK) {
        
        retry_cnt += 1;
        if (retry_cnt == 10000)
            break;

        sleep(0.5);
    }

    if (retry_cnt == 10000) {
        HB_CLOGGER->warn("Message fetch timeout: {}", msg_key);
        return false;
    }
    
    HB_CLOGGER->info("Message detected: {}", msg_key);
    return true;
}

bool hartebeest::HartebeestCore::memc_del_general(const char* msg_key) {
    hb_retcode hb_rc;
    hb_rc = HARTEBEEST_MEMC_HDL.del(msg_key);

    if (hb_rc.ret_code == MEMCH_DEL_OK)
        return true;
    
    return false;
}

void hartebeest::HartebeestCore::init() {
    // Does nothing, but for the call the get_instance().
    if (1)
        ;
}

bool hartebeest::HartebeestCore::create_local_pd(const char* pd_key) {

    HB_CLOGGER->info("Creating protection domain: {}", pd_key);
    hartebeest::Pd* new_pd = new hartebeest::Pd(
        pd_key, HB_HCA_INITR.get_hca(hca_idx)
    );
    hb_retcode hb_rc = HB_PD_CACHE.register_resrc(pd_key, new_pd);
    HB_CLOGGER->info("{}", hb_rc.aux_str);

    if (hb_rc.ret_code == CACHE_RETCODE_REGISTER_OK)
        return true;

    return false;
}

bool hartebeest::HartebeestCore::create_local_mr(const char* pd_key, const char* mr_key, size_t buflen, int rights) {

    HB_CLOGGER->info("Creating memory region: {}, to {}", mr_key, pd_key);
    hartebeest::Pd* registered_pd = HB_PD_CACHE.get_resrc(pd_key);
    assert(registered_pd != nullptr);

    hb_retcode hb_rc = registered_pd->create_mr(mr_key, buflen, rights);
    HB_CLOGGER->info("{}", hb_rc.aux_str);

    if (hb_rc.ret_code == PD_RETCODE_CREATE_MR_OK)
        return true;
    
    return false;
}

bool hartebeest::HartebeestCore::memc_push_local_mr(const char* memc_key, const char* pd_key, const char* mr_key) {

    hartebeest::Pd* local_pd = HB_PD_CACHE.get_resrc(pd_key);
    hartebeest::Mr* local_mr = local_pd->get_mr_cache().get_resrc(mr_key);

    hb_retcode hb_rc;
    hb_rc = HARTEBEEST_MEMC_HDL.set(memc_key, local_mr->flatten_info().c_str());

    if (hb_rc.ret_code == MEMCH_SET_OK)
        return true;
    
    return false;
}

bool hartebeest::HartebeestCore::memc_fetch_remote_mr(const char* remote_mr_key) {
    std::string fetched{""};

    while (HARTEBEEST_MEMC_HDL.get(remote_mr_key, fetched).ret_code != MEMCH_GET_OK)
        sleep(0.5);

    hartebeest::Mr* remote_mr = new hartebeest::Mr(remote_mr_key, 0);
    remote_mr->unflatten_info(fetched.c_str());
    remote_mr_cache.register_resrc(remote_mr_key, remote_mr);

    HB_CLOGGER->info("Fetch MEMC: {}, 0x{:x}", fetched, uintptr_t(remote_mr));

    return true;
}

bool hartebeest::HartebeestCore::create_basiccq(const char* cq_key) {

    HB_CLOGGER->info("Creating basic completion queue: {}", cq_key);
    hartebeest::BasicCq* new_basiccq = new hartebeest::BasicCq(
        cq_key, 
        HB_HCA_INITR.get_hca(hca_idx));

    assert(new_basiccq != nullptr);

    hb_retcode hb_rc;
    hb_rc = HB_BASICCQ_CACHE.register_resrc(cq_key, new_basiccq);

    if (hb_rc.ret_code == CACHE_RETCODE_REGISTER_OK)
        return true;
    
    return false;
}


bool hartebeest::HartebeestCore::create_local_qp(const char* pd_key, const char* qp_key, const char* sendcq_key, const char* recvcq_key) {

    HB_CLOGGER->info("Creating Queue Pair {}, to {}", qp_key, pd_key);
    hartebeest::Pd* registered_pd = HB_PD_CACHE.get_resrc(pd_key);
    assert(registered_pd != nullptr);

    struct ibv_cq* send_cq = HB_BASICCQ_CACHE.get_resrc(sendcq_key)->get_cq();
    struct ibv_cq* recv_cq = HB_BASICCQ_CACHE.get_resrc(recvcq_key)->get_cq();

    assert((send_cq != nullptr) && (recv_cq != nullptr));

    hb_retcode hb_rc = registered_pd->create_qp(qp_key, send_cq, recv_cq);
    HB_CLOGGER->info("{}", hb_rc.aux_str);

    if (hb_rc.ret_code == hartebeest::PD_RETCODE_CREATE_QP_OK)
        return true;

    return false;
}

bool hartebeest::HartebeestCore::init_local_qp(const char* pd_key, const char* qp_key) {

    hb_retcode hb_rc = HARTEBEEST_CORE_HDL.get_local_qp(pd_key, qp_key)->transit_init();
    if (hb_rc.ret_code == QP_TRANSITION_2_INIT_OK)
        return true;
    
    return false;
}

bool hartebeest::HartebeestCore::connect_local_qp(const char* pd_key, const char* local_qp_key, const char* remote_qp_key) {

    hartebeest::Qp* remote_qp = get_remote_qp(remote_qp_key);
    hartebeest::Qp* local_qp = get_local_qp(pd_key, local_qp_key);

    assert(remote_qp != nullptr);
    assert(local_qp != nullptr);

    hb_retcode hb_rc = local_qp->transit_rtr(remote_qp);
    if (hb_rc.ret_code == QP_TRANSITION_2_RTR_ERR) {
        HB_CLOGGER->warn("{}", hb_rc.aux_str);
        return false;
    }

    hb_rc = local_qp->transit_rts();
    if (hb_rc.ret_code == QP_TRANSITION_2_RTS_ERR) {
        HB_CLOGGER->warn("{}", hb_rc.aux_str);
        return false;
    }

    HB_CLOGGER->info("Local QP(State: RTS) {} connected to: {}", local_qp_key, remote_qp_key);
    return true;
}

bool hartebeest::HartebeestCore::memc_push_local_qp(const char* memc_key, const char* pd_key, const char* qp_key) {

    hartebeest::Pd* local_pd = HB_PD_CACHE.get_resrc(pd_key);
    hartebeest::Qp* local_qp = local_pd->get_qp_cache().get_resrc(qp_key);

    hb_retcode hb_rc;
    hb_rc = HARTEBEEST_MEMC_HDL.set(
        memc_key, local_qp->flatten_info().c_str()
    );

    if (hb_rc.ret_code == MEMCH_SET_OK)
        return true;
    
    return false;
}

bool hartebeest::HartebeestCore::memc_fetch_remote_qp(const char* remote_qp_key) {
    std::string fetched{""};

    while (HARTEBEEST_MEMC_HDL.get(remote_qp_key, fetched).ret_code != MEMCH_GET_OK)
        sleep(0.5);

    hartebeest::Qp* remote_qp = new hartebeest::Qp(remote_qp_key, 0, 0, 0);
    remote_qp->unflatten_info(fetched.c_str());
    remote_qp_cache.register_resrc(remote_qp_key, remote_qp);

    HB_CLOGGER->info("Fetch MEMC: {}, 0x{:x}", fetched, uintptr_t(remote_qp));

    return true;
}

bool hartebeest::HartebeestCore::rdma_post_single_fast(
        struct ibv_qp* local_qp, void* local_addr, void* remote_addr, size_t len, 
        enum ibv_wr_opcode opcode, uint32_t lkey, uint32_t rkey, uint64_t work_id
    ) {
        struct ibv_send_wr work_req;
        struct ibv_sge sg_elem;
        struct ibv_send_wr* bad_work_req = nullptr;

        std::memset(&sg_elem, 0, sizeof(sg_elem));
        std::memset(&work_req, 0, sizeof(work_req)); 

        sg_elem.addr = reinterpret_cast<uintptr_t>(local_addr);
        sg_elem.length = len;
        sg_elem.lkey = lkey;

        work_req.wr_id = work_id;
        work_req.num_sge = 1;
        work_req.opcode = opcode;
        work_req.send_flags = IBV_SEND_SIGNALED;
        work_req.wr_id = 0;
        work_req.sg_list = &sg_elem;
        work_req.next = nullptr;

        work_req.wr.rdma.remote_addr = reinterpret_cast<uintptr_t>(remote_addr);
        work_req.wr.rdma.rkey = rkey;

        int ret = ibv_post_send(local_qp, &work_req, &bad_work_req);

        if (bad_work_req != nullptr) {
            HB_CLOGGER->warn("Bad work request");
            return false;
        }
        if (ret != 0) {
            HB_CLOGGER->warn("RDMA Post unusual return: {}", ret);
            return false;
        }

        return true;
}

bool hartebeest::HartebeestCore::rdma_poll(const char* cq_key) {
    struct ibv_wc wc;
    struct ibv_cq* cq = HB_BASICCQ_CACHE.get_resrc(cq_key)->get_cq();

    int nwc;
    do {
        nwc = ibv_poll_cq(cq, 1, &wc);
    } while (nwc == 0);

    if (wc.status != IBV_WC_SUCCESS) {
        HB_CLOGGER->warn("Expected IBV_WC_SUCCESS, but no");
        return false;
    }

    return true;
}

hartebeest::Pd* hartebeest::HartebeestCore::get_local_pd(const char* pd_key) {
    return HB_PD_CACHE.get_resrc(pd_key);
}

hartebeest::Mr* hartebeest::HartebeestCore::get_local_mr(const char* pd_key, const char* mr_key) {
    return HB_PD_CACHE.get_resrc(pd_key)->get_mr_cache().get_resrc(mr_key);
}

hartebeest::Qp* hartebeest::HartebeestCore::get_local_qp(const char* pd_key, const char* qp_key) {
    return HB_PD_CACHE.get_resrc(pd_key)->get_qp_cache().get_resrc(qp_key);
}

hartebeest::Mr* hartebeest::HartebeestCore::get_remote_mr(const char* remote_mr_key) {
    return remote_mr_cache.get_resrc(remote_mr_key);
}

hartebeest::Qp* hartebeest::HartebeestCore::get_remote_qp(const char* remote_qp_key) {
    return remote_qp_cache.get_resrc(remote_qp_key);
}


char* hartebeest::HartebeestCore::get_sysvar(const char* envvar) {
    return HB_CFG_LOADER.get_sysvar(envvar);
}

int hartebeest::HartebeestCore::get_nid() {
    return nid;
}

// Debugs
void hartebeest::HartebeestCore::pd_status() {
    HB_PD_CACHE.out_cache_status();
}

void hartebeest::HartebeestCore::mr_status(const char* pd_key) {
    HB_PD_CACHE.get_resrc(pd_key)->get_mr_cache().out_cache_status();
}

void hartebeest::HartebeestCore::cq_status() {
    HB_BASICCQ_CACHE.out_cache_status();
}

void hartebeest::HartebeestCore::qp_status(const char* pd_key) {
    HB_PD_CACHE.get_resrc(pd_key)->get_qp_cache().out_cache_status();
}
