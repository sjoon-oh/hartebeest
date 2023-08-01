/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * pds.cc
 */

#include <cassert>

#include <iostream>
#include <cstdint>
#include <infiniband/verbs.h>

#include "./includes/hb_retcode.hh"
#include "./includes/hb_logger.hh"
#include "./includes/hb_pds.hh"


hartebeest::Pd::Pd(const char* name, hartebeest::Hca& hca_dev) {
    hca_ln = &hca_dev;
    pd = ibv_alloc_pd(hca_ln->get_device_ctx());

    assert(pd != nullptr);
    this->name = std::string(name);
}

hartebeest::Pd::~Pd() {
    
    // Delete Allocated MRs 
    for (auto it: mr_cache.get_resrc_map())
        delete it.second;

    for (auto it: qp_cache.get_resrc_map())
        delete it.second;
    
    if (pd != nullptr)
        ibv_dealloc_pd(pd);
}

struct ibv_pd* hartebeest::Pd::get_pd() {
    return pd;
}

hartebeest::Hca* hartebeest::Pd::get_hca() {
    return hca_ln;
}

hb_retcode hartebeest::Pd::create_mr(const char* key, size_t len, int rights) {

    // Make it seated at Heap.
    hartebeest::Mr* new_mr = new hartebeest::Mr(key, len);
    hb_retcode ret = mr_cache.register_resrc(key, new_mr);

    // HB_CLOGGER->info("{}, Mr({})", ret.aux_str, new_mr->get_name());  

    struct ibv_mr* ptr = ibv_reg_mr(this->pd, reinterpret_cast<void*>(new_mr->get_buffer()), len, rights);

    if (ptr == nullptr)
        return hb_retcode(PD_RETCODE_IB_REG_MR_ERR);
    
    new_mr->set_mr(ptr);

    if (ret.ret_code != CACHE_RETCODE_REGISTER_OK) {
        ret.append_str(PD_RETCODE_CREATE_MR_ERR);
        ret.ret_code = PD_RETCODE_CREATE_MR_ERR;
    }
    else {
        ret.append_str(PD_RETCODE_CREATE_MR_OK);
        ret.ret_code = PD_RETCODE_CREATE_MR_OK;
    }

    return ret;
}

hb_retcode hartebeest::Pd::create_qp(const char* qp_name, enum ibv_qp_type conn_type, struct ibv_cq* sq, struct ibv_cq* rq) {
    
    hartebeest::Qp* new_qp = new hartebeest::Qp(qp_name, conn_type, this, sq, rq);
    assert(new_qp != nullptr);

    hb_retcode ret = qp_cache.register_resrc(qp_name, new_qp);

    if (ret.ret_code != CACHE_RETCODE_REGISTER_OK) {
        ret.append_str(PD_RETCODE_CREATE_QP_ERR);
        ret.ret_code = PD_RETCODE_CREATE_QP_ERR;
    }
    else {
        ret.append_str(PD_RETCODE_CREATE_QP_OK);
        ret.ret_code = PD_RETCODE_CREATE_QP_OK;
    }

    return ret;
}

hartebeest::ResourceCache<hartebeest::Mr>& hartebeest::Pd::get_mr_cache() {
    return this->mr_cache;
}

hartebeest::ResourceCache<hartebeest::Qp>& hartebeest::Pd::get_qp_cache() {
    return this->qp_cache;
}

hartebeest::PdCache::PdCache(const char* name) : ResourceCache<Pd>(name) {
    
}