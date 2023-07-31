/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_mrs.cc
 */

#include <cassert>
#include <string>
#include <sstream>
#include <cstdlib>

#include <infiniband/verbs.h> // OFED IB verbs

#include "./includes/hb_alloc.hh"
#include "./includes/hb_mrs.hh"

hartebeest::Mr::Mr(size_t len) {
    name = std::string("ANONYMOUS-MR");

    if (len > 0) {
        buffer = hartebeest::alloc_buffer(len, 64);
        assert(buffer != nullptr);

        type = hartebeest::MR_TYPE_LOCAL;
    }
    else {
        type = hartebeest::MR_TYPE_UNKNOWN;
    }
}

hartebeest::Mr::Mr(const char* key, size_t len) : Mr(len) {
    name = std::string(key);
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

void hartebeest::Mr::set_type(enum hartebeest::MrType mr_type) {
    type = mr_type;
}

std::string hartebeest::Mr::flatten_info() {
    std::ostringstream stream;

    assert(mr != nullptr);
    stream 
        << name << ":"
        << std::hex << reinterpret_cast<uintptr_t>(mr->addr) << ":"
        << mr->length << ":"
        << mr->lkey << ":"
        << mr->rkey;

    return stream.str();
}

void hartebeest::Mr::unflatten_info(const char* fetch) {
    assert(is_allocated() == false);

    std::string imported(fetch);

    std::replace(imported.begin(), imported.end(), ':', ' ');
    std::stringstream stream(imported);
    
    mr = reinterpret_cast<struct ibv_mr*>(std::malloc(sizeof(struct ibv_mr)));
    std::memset(mr, 0, sizeof(struct ibv_mr));

    uintptr_t buf_addr;

    stream >> name;
    stream >> std::hex >> buf_addr;
    stream >> std::hex >> mr->length;
    stream >> std::hex >> mr->lkey;
    stream >> std::hex >> mr->rkey;

    mr->addr = reinterpret_cast<void*>(buf_addr);
    buffer = reinterpret_cast<uint8_t*>(buf_addr);
}