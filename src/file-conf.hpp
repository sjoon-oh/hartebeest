#pragma once

/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * file-conf.hpp
 */

#include <string>

namespace hartebeest {

    const std::string RDMA_MY_CONF_PATH{"hb-rdma-my-conf.json"};
    const std::string RDMA_CONF_DEFAULT_PATH{"hb-rdma-post-conf.json"};
    const std::string EXCH_CONF_DEFAULT_PATH{"hb-rdma-pre-conf.json"};

    const std::array<std::string, 6> KEY_PRECONF{
        "port", "index", "participants", "node_id", "ip", "alias"
    };

    const std::array<std::string, 2> KEY_POSTCONF{
        "node_id", "qp_conn"
    };

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
        STATE_FILLED,
        STATE_DISTRIBUTED
    };




}