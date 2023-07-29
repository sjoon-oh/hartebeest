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

    class Pd; // Somewhere.
    class Qp {
    private:
        std::string name;
        struct ibv_qp* qp = nullptr;
        
        Pd* involv = nullptr;
        
    public:
        Qp(const char*, Pd*, struct ibv_cq*, struct ibv_cq*);
        ~Qp();

        bool is_qp_created() const;
        bool is_involved() const;

        int query_state();

        hb_retcode transit_init();


        struct ibv_qp* get_qp();
    };
}