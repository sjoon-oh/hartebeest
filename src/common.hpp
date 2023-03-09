#pragma once

/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * common.hpp
 */

#include <memory>
#include <functional>
#include <map>
#include <vector>

#include <cmath>
#include <iomanip>
#include <iostream>

template <typename T>
using del_unique_ptr = std::unique_ptr<T, std::function<void(T *)>>;

template <typename T>
struct DeleteAligned {
    void operator()(T *data) const { free(data); }
};

template <typename T>
std::unique_ptr<T[], DeleteAligned<T>> allocate_aligned(int alignment,
                                                        size_t length) {
    T *raw = reinterpret_cast<T *>(aligned_alloc(alignment, sizeof(T) * length));
    if (raw == nullptr) {
        throw std::runtime_error("Insufficient memory");
    }

    return std::unique_ptr<T[], DeleteAligned<T>>{raw};
}

// namespace hartebeest {
//     enum {
//         LOCAL_READ = 0,
//         LOCAL_WRITE = IBV_ACCESS_LOCAL_WRITE,
//         REMOTE_READ = IBV_ACCESS_REMOTE_READ,
//         REMOTE_WRITE = IBV_ACCESS_REMOTE_WRITE
//     };
// }