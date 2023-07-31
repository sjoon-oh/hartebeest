/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hca.hpp
 */

#include <cassert>
#include <iostream>
#include <cstdio>

#include <cstdint>
#include <infiniband/verbs.h>

#include "./includes/hb_retcode.hh"
#include "./includes/hb_logger.hh"
#include "./includes/hb_hca.hh"

hartebeest::Hca::Hca() {
    std::memset(&hca_attr, 0, sizeof(struct ibv_device_attr));
}

hartebeest::Hca::Hca(struct ibv_device* dev_hdl) {
    device_register(dev_hdl);
}

hartebeest::Hca::~Hca() {
    if (device_reset().ret_code != HCA_RETCODE_RESET_OK)
        ;
}

hb_retcode hartebeest::Hca::device_register(struct ibv_device* dev_hdl) {
    hca_dev = dev_hdl;
    return hb_retcode(HCA_RETCODE_REGISTER_OK);
}

hb_retcode hartebeest::Hca::device_reset() {
    if (hca_ctx != nullptr) {
        ibv_close_device(hca_ctx);
        return hb_retcode(HCA_RETCODE_RESET_ERR);
    }

    std::memset(&hca_attr, 0, sizeof(hca_attr));
    return hb_retcode(HCA_RETCODE_RESET_OK);
}

hb_retcode hartebeest::Hca::device_open() {
    hca_ctx = ibv_open_device(hca_dev);

    if (ibv_query_device(hca_ctx, &hca_attr) == 0)
        return hb_retcode(HCA_RETCODE_OPEN_OK);
    else
        return hb_retcode(HCA_RETCODE_OPEN_ERR);
}

struct ibv_context* hartebeest::Hca::get_device_ctx() {
    return hca_ctx;
}

struct ibv_device_attr& hartebeest::Hca::get_device_attr() {
    return hca_attr;
}

void hartebeest::Hca::set_device_pid(uint8_t pid) {
    hca_pid = pid;
};
void hartebeest::Hca::set_device_plid(uint16_t plid) {
    hca_plid = plid;
}

const uint8_t hartebeest::Hca::get_device_pid() const {
    return hca_pid;
}

const uint16_t hartebeest::Hca::get_device_plid() const {
    return hca_plid;
}

hartebeest::HcaInitializer::HcaInitializer() {
    n_hca_devs = 0;
    dev_list = ibv_get_device_list(&n_hca_devs);

    assert(n_hca_devs != 0);

    for (int idx = 0; idx < n_hca_devs; idx++)
        hca_vec.push_back(Hca(dev_list[idx]));
}

hb_retcode hartebeest::HcaInitializer::open_device(int idx) {
    if (idx >= n_hca_devs)
        return hb_retcode(HCAINITR_RETCODE_RANGE_ERR);

    if (hca_vec.size() == 0)
        return hb_retcode(HCAINITR_RETCODE_UNINIT_ERR);
    
    return hca_vec.at(idx).device_open();
}

hb_retcode hartebeest::HcaInitializer::bind_port(int idx, uint8_t port_num) {

    hartebeest::Hca& hca_dev = hca_vec.at(idx);

    if (idx >= n_hca_devs || idx < 0)
        return hb_retcode(HCAINITR_RETCODE_RANGE_ERR);

    if (hca_dev.get_device_ctx() == nullptr)
        return hb_retcode(HCAINITR_RETCODE_RANGE_ERR);

    uint8_t n_hca_ports = hca_dev.get_device_attr().phys_port_cnt;

    struct ibv_port_attr port_attr;
    std::memset(&port_attr, 0, sizeof(ibv_port_attr));
    if (ibv_query_port(hca_dev.get_device_ctx(), port_num, &port_attr))
        return hb_retcode(HCAINITR_RETCODE_PORT_QUERY_ERR);

    if (port_attr.phys_state != IBV_PORT_ACTIVE &&
        port_attr.phys_state != IBV_PORT_ACTIVE_DEFER) {
        hb_retcode(HCAINITR_RETCODE_PORT_QUERY_ERR);
    }

    if (port_attr.link_layer != IBV_LINK_LAYER_INFINIBAND) 
        return hb_retcode(HCAINITR_RETCODE_NO_IB);

    hca_dev.set_device_pid(port_num);
    hca_dev.set_device_plid(port_attr.lid);

    return hb_retcode(HCAINITR_RETCODE_BIND_PORT_OK);
}

const int hartebeest::HcaInitializer::get_n_hca_devs() const {
    return n_hca_devs;
}

hartebeest::Hca& hartebeest::HcaInitializer::get_hca(int idx) {
    assert((idx < n_hca_devs) && (idx >= 0));
    return hca_vec.at(idx);
}