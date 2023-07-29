#pragma once
/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_alloc.hh
 */

#include <cstdlib>
#include <infiniband/verbs.h> // OFED IB verbs

namespace hartebeest {

    uint8_t* alloc_buffer(size_t, int = 64);
    void free_buffer(uint8_t*);
}
