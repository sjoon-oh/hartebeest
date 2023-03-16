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