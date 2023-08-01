#pragma once
/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_memc.hh
 */

#include <string>
#include <regex>

#include <libmemcached/memcached.h>
// https://awesomized.github.io/libmemcached/libmemcached/index.html

#include "./hb_retcode.hh"

namespace hartebeest {

    class MemcHandle {
    protected:
        std::string memc_ip{""};
        std::string memc_port{""};

        memcached_st* memc_serv_hdl;

    public:
        MemcHandle();
        virtual ~MemcHandle();

        virtual hb_retcode set(const char*, const char*) = 0;
        virtual hb_retcode get(const char*, std::string&) = 0;
    };

    enum {
        HB_MEMC_KEY_PREF_INIT   = 0         ,
        HB_MEMC_KEY_PREF_MRINFO             ,
        HB_MEMC_KEY_PREF_QPINFO             ,
        HB_MEMC_KEY_PREF_MRREADY            ,
        HB_MEMC_KEY_PREF_QPREADY            ,
    };

    class Exchanger : public MemcHandle {
    private:
        std::string nid;
        std::string participants;
        // MemcHandle();
        // ~MemcHandle();

    public:
        Exchanger();
        ~Exchanger();

        hb_retcode set(const char*, const char*);
        hb_retcode get(const char*, std::string&);

        hb_retcode prefix_set(const char*, const int, const char*);
        hb_retcode prefix_get(const char*, const int, std::string&);

        hb_retcode del(const char*);

        static Exchanger& get_instance() {
            static Exchanger exchgr;
            return exchgr;
        }
    };
}

// #define HB_MEMC         hartebeest::MemcHandle::get_instance()
#define HB_EXCHANGER    hartebeest::Exchanger::get_instance()