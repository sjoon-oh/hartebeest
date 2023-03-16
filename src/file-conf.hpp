#pragma once

/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * file-conf.hpp
 */

#include <string>

namespace hartebeest {

    const std::string RDMA_MY_CONF_PATH{"this-node-conf.json"};
    const std::string RDMA_CONF_DEFAULT_PATH{"post-all-conf.json"};
    const std::string EXCH_CONF_DEFAULT_PATH{"pre-all-conf.json"};

    enum { 
        ROLE_SERVER = 0,
        ROLE_CLIENT,
        ROLE_UNKNOWN = 0xff
    };

    enum {
        STATE_UNKNOWN = 0xff00,
        STATE_FILLED,
        STATE_DISTRIBUTED
    };

}