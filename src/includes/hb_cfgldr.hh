#pragma once

/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_cfgldr.hh
 */

#include <string>
#include <map>

#include <infiniband/verbs.h>
#include "./hb_retcode.hh"

namespace hartebeest {
#define MAX_ENVVARS     3

    struct ConfVar {
        const char* varname;
        char* val;
    };

    struct ConfPair {
        const char* key;
        int val;
    };

    struct ConfDict {
        const char* key;
        int n_elem;
        struct ConfPair* conf;
    };

    enum {
        PDEF_CQ_ATTR        = 0 ,
        PDEF_QP_INIT_ATTR       ,
        PDEF_QP_ATTR
    };

    class ConfigLoader {
    private:
        std::string fname;
        
        std::map<std::string, char*> sysvar_cache;
        std::map<std::string, int*> attr_cache;

        bool is_sysvar_cached(const char*);
        bool is_attr_cached(const char*);

    public:
        ConfigLoader(const char* = "./hb_config.json");

        hb_retcode init_sysvars();
        hb_retcode init_params();
        
        char* get_sysvar(const char*);
        int* get_attr(const char*);

        static ConfigLoader& get_instance() {
            static ConfigLoader single_loader;
            return single_loader;
        }
    };
}

#define HB_CFG_LOADER  hartebeest::ConfigLoader::get_instance()