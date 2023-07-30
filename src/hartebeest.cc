/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hartebeest.cc 
 */

#include "./includes/hartebeest.hh"

hartebeest::HartebeestCore::HartebeestCore(int idx) : hca_idx(idx) {

    hb_retcode hb_rc = HB_CFG_LOADER.init_sysvars();
    hb_rc = HB_CFG_LOADER.init_params();

    int n_hca = HB_HCA_INITR.get_n_hca_devs();

    assert(n_hca > 0);
    hb_rc = HB_HCA_INITR.open_device(idx);
    HB_CLOGGER->info("{}", hb_rc.aux_str);

    hb_rc = HB_HCA_INITR.bind_port(idx);
    HB_CLOGGER->info("{}", hb_rc.aux_str);
}

hartebeest::HartebeestCore::~HartebeestCore() {
}

void hartebeest::HartebeestCore::init() {
    // Does nothing, but for the call the get_instance().
    if (1)
        ;
}

bool hartebeest::HartebeestCore::create_pd(const char* pd_key) {

    HB_CLOGGER->info("Creating protection domain: {}", pd_key);
    hartebeest::Pd* new_pd = new hartebeest::Pd(
        pd_key, HB_HCA_INITR.get_hca(hca_idx)
    );
    hb_retcode hb_rc = HB_PD_CACHE.register_resrc(pd_key, new_pd);
    HB_CLOGGER->info("{}", hb_rc.aux_str);

    if (hb_rc.ret_code == CACHE_RETCODE_REGISTER_OK)
        return true;

    return false;
}

bool hartebeest::HartebeestCore::create_mr(const char* pd_key, const char* mr_key, size_t buflen, int rights) {

    HB_CLOGGER->info("Creating memory region: {}, to {}", mr_key, pd_key);
    hartebeest::Pd* registered_pd = HB_PD_CACHE.get_resrc(pd_key);
    assert(registered_pd != nullptr);

    hb_retcode hb_rc = registered_pd->create_mr(mr_key, buflen, rights);
    HB_CLOGGER->info("{}", hb_rc.aux_str);

    if (hb_rc.ret_code == PD_RETCODE_CREATE_MR_OK)
        return true;
    
    return false;
}
