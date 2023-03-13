#pragma once

/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * file-conf.hpp
 */

#include <string>

namespace hartebeest {

    const std::string RDMA_CONF_DEFAULT_PATH{"hb_rdma_post_config.json"};
    const std::string EXCH_CONF_DEFAULT_PATH{"hb_rdma_pre_config.json"};

    const std::array<std::string, 6> KEY_PRECONF{
        "port", "index", "participants", "node_id", "ip", "alias"
    };

    enum {
        PORT = 0,
        INDEX,
        PARTICIPANTS,
        NODE_ID,
        IP,
        ALIAS
    };

#define __KEY(X)    (KEY_PRECONF.at(X))

    enum { 
        ROLE_SERVER = 0,
        ROLE_CLIENT,
        ROLE_UNKNOWN = 0xff
    };

    enum {
        REQUEST_INIT_CONF_FILE = 0,
        REQUEST_ETHN_CONF_FILE,
        REQUEST_RDMA_CONF_FILE,
        REQUEST_READY
    };

    enum {
        RESPONSE_INIT_CONF_FILE = 0xf0,
        RESPONSE_ETHN_CONF_FILE,
        RESPONSE_RDMA_CONF_FILE,
        RESPONSE_READY
    };

    enum {
        STATE_UNKNOWN = 0xff,
        STATE_CONNECTED,
        STATE_DONE
    };

}