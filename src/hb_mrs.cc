/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_mrs.cc
 */

#include <cassert>

#include "./includes/hb_alloc.hh"
#include "./includes/hb_mrs.hh"

hartebeest::Mr::Mr(size_t len) {
    name = std::string("ANONYMOUS-MR");

    buffer = hartebeest::alloc_buffer(len, 64);
    assert(buffer != nullptr);
}

hartebeest::Mr::Mr(const char* key, size_t len) {
    name = std::string(key);

    buffer = hartebeest::alloc_buffer(len, 64);
    assert(buffer != nullptr);
}

hartebeest::Mr::~Mr() {
    if (is_mr_created())
        ibv_dereg_mr(mr);

    if (is_allocated())
        hartebeest::free_buffer(buffer);
}

bool hartebeest::Mr::is_allocated() const {
    return (buffer != nullptr);
}

bool hartebeest::Mr::is_mr_created() const {
    return (mr != nullptr);
}

const char* hartebeest::Mr::get_name() const {
    return name.c_str();
}

uint8_t* hartebeest::Mr::get_buffer() const {
    return buffer;
}

struct ibv_mr* hartebeest::Mr::get_mr() const {
    return mr;
}

void hartebeest::Mr::set_mr(struct ibv_mr* ptr) {
    mr = ptr;
}
