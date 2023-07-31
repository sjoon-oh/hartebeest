#pragma once

/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hca.hpp
 */

#include <memory>
// #include <vector>
#include <infiniband/verbs.h> // OFED IB verbs

#include "./hb_retcode.hh"
#include "./hb_logger.hh"

namespace hartebeest {

    class Hca final {
    private:
        struct ibv_device*      hca_dev = nullptr;
        struct ibv_context*     hca_ctx = nullptr;
        struct ibv_device_attr  hca_attr;

        uint8_t                 hca_pid;
        uint16_t                hca_plid;

    public:
        Hca();
        Hca(struct ibv_device*);
        ~Hca();

        hb_retcode device_register(struct ibv_device*);
        hb_retcode device_reset();
        hb_retcode device_open();

        struct ibv_context* get_device_ctx();
        struct ibv_device_attr& get_device_attr();
        
        const uint8_t get_device_pid() const;
        const uint16_t get_device_plid() const;

        void set_device_pid(uint8_t);
        void set_device_plid(uint16_t);
    };

    class HcaInitializer final {
        int n_hca_devs;
        struct ibv_device** dev_list = nullptr;
        std::vector<Hca> hca_vec{};

    public:
        HcaInitializer();
        static HcaInitializer& get_instance() {
            static HcaInitializer hca_initializer;
            return hca_initializer;
        }

        hb_retcode open_device(int);
        hb_retcode bind_port(int, uint8_t);

        const int get_n_hca_devs() const;
        Hca& get_hca(int);
    };
}

#define HB_HCA_INITR    hartebeest::HcaInitializer::get_instance()