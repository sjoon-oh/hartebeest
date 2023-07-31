#pragma once
/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_qps.hh
 */

#include <vector>
#include <string>

#include <infiniband/verbs.h> // OFED IB verbs

#include "./hb_retcode.hh"
#include "./hb_logger.hh"

#include "./hb_pds.hh"

namespace hartebeest {

    enum QpType {
        QP_TYPE_UNKNOWN,
        QP_TYPE_LOCAL,
        QP_TYPE_REMOTE
    };

    class Pd; // Somewhere.
    class Qp {
    private:
        std::string name;
        enum QpType type = QP_TYPE_UNKNOWN;

        struct ibv_qp* qp = nullptr;
        uint8_t pid;
        uint16_t plid;
        
    public:
        Qp(const char*, Pd*, struct ibv_cq*, struct ibv_cq*);
        ~Qp();

        void set_type(enum QpType);

        bool is_qp_created() const;
        bool is_involved() const;

        int query_state();

        // State transition interfaces
        hb_retcode transit_init();
        hb_retcode transit_rtr(Qp*);
        hb_retcode transit_rts();

        uint8_t get_pid();
        uint16_t get_plid();

        struct ibv_qp* get_qp();
        
        std::string flatten_info();
        void unflatten_info(const char*);
    };
}