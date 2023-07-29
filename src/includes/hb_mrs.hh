#pragma once
/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_mrs.hh
 */

#include <vector>
#include <string>

#include <infiniband/verbs.h> // OFED IB verbs

#include "./hb_retcode.hh"
#include "./hb_logger.hh"

namespace hartebeest {

    class Mr {
    private:
        std::string name;
        uint8_t* buffer = nullptr;
        struct ibv_mr* mr = nullptr;

    public:
        Mr(size_t);
        Mr(const char*, size_t);
        ~Mr();

        bool is_allocated() const;
        bool is_mr_created() const;
        
        const char* get_name() const;
        uint8_t* get_buffer() const;
        struct ibv_mr* get_mr() const;

        void set_mr(struct ibv_mr*);
    };
}