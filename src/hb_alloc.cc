/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_alloc.cc
 */

#include "./includes/hb_alloc.hh"

uint8_t* hartebeest::alloc_buffer(size_t len, int align) {
    return reinterpret_cast<uint8_t*>(aligned_alloc(len, align));
}

void hartebeest::free_buffer(uint8_t* buf) {
    free(buf);
}