/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_memc.cc
 */

#include <cassert>
#include <string>
#include <regex>
#include <vector>

#include <iostream>

#include <libmemcached/memcached.h>

#include "./includes/hb_logger.hh"
#include "./includes/hb_cfgldr.hh"
#include "./includes/hb_memc.hh"

namespace hartebeest {
    const char* pdef_memc_key_prefix[] = {
        "hartebeest-init",
        "hartebeest-mrinfo",
        "hartebeest-qpinfo", 
        "hartebeest-mr-ready",
        "hartebeest-qp-ready"
    };

    std::string make_key(int pref_idx, const char* divstr) {
        return (std::string(pdef_memc_key_prefix[pref_idx]) + "-" + std::string(divstr));
    }
}

hartebeest::MemcHandle::MemcHandle() {
    memc_serv_hdl = memcached_create(nullptr);

    if (memc_serv_hdl == nullptr)
        HB_CLOGGER->warn("Memcached handle acquisition failed.");

    assert(memc_serv_hdl != nullptr);
}

hartebeest::MemcHandle::~MemcHandle() {
    if (memc_serv_hdl != nullptr)
        memcached_free(memc_serv_hdl);
}

hartebeest::Exchanger::Exchanger() : MemcHandle() {

    char* sysvar = HB_CFG_LOADER.get_sysvar("HARTEBEEST_NID");
    assert(sysvar != nullptr);
    nid = std::string(sysvar);

    sysvar = HB_CFG_LOADER.get_sysvar("HARTEBEEST_PARTICIPANTS");
    assert(sysvar != nullptr);
    participants = std::string(sysvar);

    HB_CLOGGER->info("Exchanger: nid({}), participants({})", nid, participants);

    sysvar = HB_CFG_LOADER.get_sysvar("HARTEBEEST_EXC_IP_PORT");
    assert(sysvar != nullptr);

    std::string memc_ip_port(sysvar);
    std::regex regex(":");

    std::vector<std::string> splits(
        std::sregex_token_iterator(memc_ip_port.begin(), memc_ip_port.end(), regex, -1),
        std::sregex_token_iterator());

    assert(splits.size() == 2);

    memc_ip = splits.at(0);
    memc_port = splits.at(1);

    memcached_return_t memc_ret;
    memc_ret = memcached_server_add(memc_serv_hdl, memc_ip.c_str(), std::stoi(memc_port.c_str()));

    assert(memc_ret == MEMCACHED_SUCCESS);
    HB_CLOGGER->info("Memcached server {} at port {} added: OK", memc_ip, memc_port);
}

hartebeest::Exchanger::~Exchanger() {

}

hb_retcode hartebeest::Exchanger::set(const char* key, const char* val) {
    assert(memc_serv_hdl != nullptr);

    int keylen = std::strlen(key);
    int vallen = std::strlen(val);

    memcached_return_t memc_ret;
    memc_ret = memcached_set(
        memc_serv_hdl, key, keylen, val, vallen, static_cast<time_t>(0), static_cast<uint32_t>(0)
    );

    if (memc_ret != MEMCACHED_SUCCESS) {
        HB_CLOGGER->warn("Memcached SET <{}, {}> failed", key, val);
        return hb_retcode(MEMCH_SET_ERR);
    }

    return hb_retcode(MEMCH_SET_OK);
}

hb_retcode hartebeest::Exchanger::get(const char* key, std::string& result) {
    assert(memc_serv_hdl != nullptr);

    memcached_return_t memc_ret;
    size_t vallen_ret;
    uint32_t flags_ret;

    char* val_ret = memcached_get(
        memc_serv_hdl, 
        key, std::strlen(key), &vallen_ret, &flags_ret, &memc_ret
    );

    if (memc_ret != MEMCACHED_SUCCESS) {
        return hb_retcode(MEMCH_GET_ERR);
    }

    if (memc_ret == MEMCACHED_SUCCESS) {
        result = std::string(val_ret, vallen_ret);
        return hb_retcode(MEMCH_GET_OK);
    }

    return hb_retcode();
}

hb_retcode hartebeest::Exchanger::prefix_set(const char* key, const int pref_idx, const char* val) {
    return set(make_key(pref_idx, key).c_str(), val);
};

hb_retcode hartebeest::Exchanger::prefix_get(const char* key, const int pref_idx, std::string& ret) {
    return this->get(make_key(pref_idx, key).c_str(), ret);
};

hb_retcode hartebeest::Exchanger::del(const char* key) {
    assert(memc_serv_hdl != nullptr);

    memcached_return_t memc_ret;
    size_t vallen_ret;
    uint32_t flags_ret;

    memc_ret = memcached_delete(
        memc_serv_hdl, 
        key, std::strlen(key), static_cast<time_t>(0)
    );

    if (memc_ret != MEMCACHED_SUCCESS) {
        HB_CLOGGER->warn("Memcached DELETE <{}, ?> failed", key);
        return hb_retcode(MEMCH_DEL_ERR);
    }

    return hb_retcode(MEMCH_DEL_OK);
}