#pragma once
/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_cache.hpp
 */

#include <string>
#include <map>

#include "./hb_retcode.hh"
#include "./hb_logger.hh"

/* ResourceCache holds just the pointers, not the actual data structure.
 * - map wrapper.
 */

namespace hartebeest {

    template <class T>
    class ResourceCache {
    private:
        std::string name;
        std::map<std::string, T*> resrc_cache;

    public:
        ResourceCache();
        ResourceCache(const char*);
        ~ResourceCache() = default;

        hb_retcode register_resrc(const char*, T*);
        hb_retcode deregister_resrc(const char*);

        bool is_registered(const char*);
        const char* get_name() const;

        T* get_resrc(const char*);
        std::map<std::string, T*>& get_resrc_map();

        // For debug
        void out_cache_status();
    };
}


template <class T>
hartebeest::ResourceCache<T>::ResourceCache() {
    name = std::string("ANONYMOUS-RESRC-CACHE");
}

template <class T>
hartebeest::ResourceCache<T>::ResourceCache(const char* key) {
    name = std::string(key);
}

template <class T>
hb_retcode hartebeest::ResourceCache<T>::register_resrc(const char* key, T* resrc) {
    if (is_registered(key))
        return hb_retcode(CACHE_RETCODE_ALREADY_EXIST_ERR);
    
    resrc_cache.insert(
        std::pair<std::string, T*>(std::string(key), resrc)
    );
    return hb_retcode(CACHE_RETCODE_REGISTER_OK);
}

template <class T>
hb_retcode hartebeest::ResourceCache<T>::deregister_resrc(const char* key) {
    if (!is_registered(key)) {
        return hb_retcode(CACHE_RETCODE_NO_EXIST_ERR);
    }

    // std::map<std::string, T*>::iterator it = resrc_cache.find(key);
    resrc_cache.erase(key);
    return hb_retcode(CACHE_RETCODE_REGISTER_OK);
}

template <class T>
bool hartebeest::ResourceCache<T>::is_registered(const char* key) {
    return (resrc_cache.find(std::string(key)) != resrc_cache.end());
}

template <class T>
const char* hartebeest::ResourceCache<T>::get_name() const {
    return name.c_str();
}

template <class T>
T* hartebeest::ResourceCache<T>::get_resrc(const char* key) {
    if (!is_registered(key)) {
        return nullptr;
    }

    return resrc_cache.find(key)->second;
}

template <class T>
std::map<std::string, T*>& hartebeest::ResourceCache<T>::get_resrc_map() {
    return this->resrc_cache;
}

template <class T>
void hartebeest::ResourceCache<T>::out_cache_status() {
    
    uintptr_t ptr;
    for (auto& it: resrc_cache) {
        ptr = reinterpret_cast<uintptr_t>(it.second);
        HB_CLOGGER->info("Status {} <{}, 0x{:x}>", name, it.first, ptr);
    }
}