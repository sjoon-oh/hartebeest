/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_cfgldr.cc
 */

#include <fstream>
#include <cstdlib>
#include <climits>

#include <iostream>

#include "../extern/nlohmann/json.hpp" // External Lib

#include "./includes/hb_logger.hh"
#include "./includes/hb_retcode.hh"
#include "./includes/hb_cfgldr.hh"

#define ARRSZ(arr, type)    (sizeof(arr) / sizeof(type))

// Essential Env. Vars.
namespace hartebeest {
    struct ConfVar system_vars[MAX_ENVVARS] = {
        {"HARTEBEEST_PARTICIPANTS",     0},
        {"HARTEBEEST_NID",              0}, // Node ID
        {"HARTEBEEST_EXC_IP_PORT",      0},
        {"HARTEBEEST_CONF_PATH",        0}
    };

    const char* pdef_cq_attr_key = "cq_attr";
    struct ConfPair pdef_cq_attr[] = {
        {"cq_depth",                    128},
    };

    const char* pdef_qp_init_attr_key = "qp_init_attr";
    struct ConfPair pdef_qp_init_attr[] = {
        {"rc:cap.max_send_wr",          128},
        {"rc:cap.max_recv_wr",          128},
        {"rc:cap.max_send_sge",         16},
        {"rc:cap.max_recv_sge",         16},
        {"rc:cap.max_inline_data",      256},

        {"uc:cap.max_send_wr",          128},
        {"uc:cap.max_recv_wr",          128},
        {"uc:cap.max_send_sge",         16},
        {"uc:cap.max_recv_sge",         16},
        {"uc:cap.max_inline_data",      256},

        {"ud:cap.max_send_wr",          128},
        {"ud:cap.max_recv_wr",          128},
        {"ud:cap.max_send_sge",         16},
        {"ud:cap.max_recv_sge",         16},
        {"ud:cap.max_inline_data",      256},
    };
    
    const char* pdef_qp_attr_key = "qp_attr";
    struct ConfPair pdef_qp_attr[] = {
        {"rc:path_mtu",                 IBV_MTU_4096},
        {"rc:rq_psn",                   3185},
        {"rc:sq_psn",                   3185},
        {"rc:ah_attr.is_global",        0},
        {"rc:ah_attr.sl",               0},
        {"rc:ah_attr.src_path_bits",    0},
        {"rc:max_dest_rd_atomic",       16},
        {"rc:min_rnr_timer",            12},
        {"rc:timeout",                  14},
        {"rc:retry_cnt",                7},
        {"rc:rnr_retry",                7},
        {"rc:max_rd_atomic",            1},
        {"rc:max_dest_rd_atomic",       16},

        {"uc:path_mtu",                 IBV_MTU_4096},
        {"uc:rq_psn",                   3185},
        {"uc:sq_psn",                   3185},
        {"uc:ah_attr.is_global",        0},
        {"uc:ah_attr.sl",               0},
        {"uc:ah_attr.src_path_bits",    0},
        {"uc:max_dest_rd_atomic",       16},
        {"uc:min_rnr_timer",            12},
        {"uc:timeout",                  14},
        {"uc:retry_cnt",                7},
        {"uc:rnr_retry",                7},
        {"uc:max_rd_atomic",            1},
        {"uc:max_dest_rd_atomic",       16},

        {"ud:path_mtu",                 IBV_MTU_4096},
        {"ud:rq_psn",                   3185},
        {"ud:sq_psn",                   3185},
        {"ud:ah_attr.is_global",        0},
        {"ud:ah_attr.sl",               0},
        {"ud:ah_attr.src_path_bits",    0},
        {"ud:max_dest_rd_atomic",       16},
        {"ud:min_rnr_timer",            12},
        {"ud:timeout",                  14},
        {"ud:retry_cnt",                7},
        {"ud:rnr_retry",                7},
        {"ud:max_rd_atomic",            1},
        {"ud:max_dest_rd_atomic",       16},
    };

    struct ConfDict pdef_cfs[] = {
        {pdef_cq_attr_key, ARRSZ(pdef_cq_attr, ConfPair), pdef_cq_attr},
        {pdef_qp_init_attr_key, ARRSZ(pdef_qp_init_attr, ConfPair), pdef_qp_init_attr},
        {pdef_qp_attr_key, ARRSZ(pdef_qp_attr, ConfPair), pdef_qp_attr}
    };
}

bool hartebeest::ConfigLoader::is_attr_cached(const char* key) {
    return (attr_cache.find(key) != attr_cache.end());
}

bool hartebeest::ConfigLoader::is_sysvar_cached(const char* key) {
    return (sysvar_cache.find(key) != sysvar_cache.end());
}


hartebeest::ConfigLoader::ConfigLoader(const char* path) {
    fname = std::string(path);
}

hb_retcode hartebeest::ConfigLoader::init_sysvars() {

    HB_CLOGGER->info("Initializing envvars...");
    for (int i = 0; i < MAX_ENVVARS; i++) {
        system_vars[i].val = getenv(system_vars[i].varname);        
        if (system_vars[i].val == NULL)
            return hb_retcode(CFGLDR_RETCODE_ENVVAR_NOT_FOUND);
        
        HB_CLOGGER->info("{}={}", system_vars[i].varname, system_vars[i].val);
        sysvar_cache.insert(
            std::pair<std::string, char*>(std::string(system_vars[i].varname), system_vars[i].val)
        );
    }
    
    return hb_retcode(CFGLDR_RETCODE_OK);
}

hb_retcode hartebeest::ConfigLoader::init_params() {

    if (system_vars[SYSVAR_CONF_PATH].val != nullptr) {
        fname = std::string(
            system_vars[SYSVAR_CONF_PATH].val
        );
    }
    
    HB_CLOGGER->info("Initializing paramters...");
    nlohmann::json cfgs;
    std::ifstream conf_file(fname);

    

    // Open file.
    if (conf_file.fail())
        return hb_retcode(CFGLDR_RETCODE_FILE_NOT_FOUND);
    conf_file >> cfgs;

    try {

        int n_attr = ARRSZ(pdef_cfs, ConfDict);
        struct hartebeest::ConfDict* attr;

        for (int i = 0; i < n_attr; i++) {
            attr = &pdef_cfs[i];

            if (!cfgs.contains(attr->key)) {
                continue;
            }

            nlohmann::json& sub_cfgs = cfgs[attr->key];
            
            for (int j = 0; j < attr->n_elem; j++) {
                const char* key = attr->conf[j].key;
                if (sub_cfgs.contains(key)) {
                    attr->conf[j].val = sub_cfgs[key];
                    HB_CLOGGER->info("Substitute {}, [{}, {}]", attr->key, key, attr->conf[i].val);
                }

                assert(is_attr_cached(attr->key) == false);
                attr_cache.insert(
                    std::pair<std::string, int*>(std::string(key), &(attr->conf[j].val))
                );

                assert(is_attr_cached(key) == true);
            }
        }

    } catch (...) {
        return hb_retcode(CFGLDR_JSON_ERR);
    }

    return hb_retcode(CFGLDR_RETCODE_OK);
}

char* hartebeest::ConfigLoader::get_sysvar(const char* envvar) {
    
    const char* val = nullptr;
    if (!is_sysvar_cached(envvar)) {
        return nullptr;
    }

    return sysvar_cache.find(envvar)->second;
}

int* hartebeest::ConfigLoader::get_attr(const char* attr_key) {
    
    const char* val = nullptr;
    if (!is_attr_cached(attr_key)) {
        return nullptr;
    }

    return attr_cache.find(attr_key)->second;
}
