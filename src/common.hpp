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

// For MrManager
typedef std::map<std::string, std::pair<size_t, size_t>>    buffer_info_t;
typedef std::unique_ptr<uint8_t[], DeleteAligned<uint8_t>>  buffer_t;
typedef std::vector<std::unique_ptr<uint8_t[], DeleteAligned<uint8_t>>> \
                                                            buffer_list_t;

typedef std::map<std::string, size_t>                       pd_info_t;
typedef std::vector<del_unique_ptr<struct ibv_pd>>          pd_list_t;

// Byte
[[maybe_unused]] static constexpr size_t operator"" _B(unsigned long long x) {
return x;
}

// KibiByte
[[maybe_unused]] static constexpr size_t operator"" _KiB(long double x) {
return std::llround(x * 1024);
}

[[maybe_unused]] static constexpr size_t operator"" _KiB(unsigned long long x) {
return x * 1024;
}

// MebiByte
[[maybe_unused]] static constexpr size_t operator"" _MiB(long double x) {
return std::llround(x * 1024 * 1024);
}

[[maybe_unused]] static constexpr size_t operator"" _MiB(unsigned long long x) {
return x * 1024 * 1024;
}

// GibiByte
[[maybe_unused]] static constexpr size_t operator"" _GiB(long double x) {
return std::llround(x * 1024 * 1024 * 1024);
}

[[maybe_unused]] static constexpr size_t operator"" _GiB(unsigned long long x) {
return x * 1024 * 1024 * 1024;
}