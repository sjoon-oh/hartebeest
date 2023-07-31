#pragma once

/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * rets.hpp
 */

#include <string>

namespace hartebeest {

    enum EncodedRetCodes {
        UNDEFINED = 0                           ,

        HCA_RETCODE_RESET_OK                    ,
        HCA_RETCODE_RESET_ERR                   ,
        HCA_RETCODE_OPEN_OK                     ,
        HCA_RETCODE_OPEN_ERR                    ,
        HCA_RETCODE_REGISTER_OK                 ,

        HCAINITR_RETCODE_OPEN_OK                ,
        HCAINITR_RETCODE_RANGE_ERR              ,
        HCAINITR_RETCODE_UNINIT_ERR             ,
        HCAINITR_RETCODE_NOCTX_ERR              ,
        HCAINITR_RETCODE_PORT_QUERY_ERR         ,
        HCAINITR_RETCODE_NO_IB                  ,
        HCAINITR_RETCODE_BIND_PORT_OK           ,

        CACHE_RETCODE_ALREADY_EXIST_ERR         ,
        CACHE_RETCODE_NO_EXIST_ERR              ,
        CACHE_RETCODE_REGISTER_OK               ,

        PD_RETCODE_CREATE_MR_ERR                ,
        PD_RETCODE_CREATE_MR_OK                 ,
        PD_RETCODE_IB_REG_MR_ERR                ,
        PD_RETCODE_CREATE_QP_ERR                ,
        PD_RETCODE_CREATE_QP_OK                 ,

        CFGLDR_RETCODE_FILE_NOT_FOUND           ,
        CFGLDR_RETCODE_ENVVAR_NOT_FOUND         ,
        CFGLDR_RETCODE_OK                       ,
        CFGLDR_JSON_ERR                         ,
        CFGLDR_KEY_NOT_FOUND                    ,

        QP_TRANSITION_2_INIT_OK                 ,
        QP_TRANSITION_2_INIT_ERR                ,
        QP_TRANSITION_2_RTR_OK                  ,
        QP_TRANSITION_2_RTR_ERR                 ,
        QP_TRANSITION_2_RTS_OK                  ,
        QP_TRANSITION_2_RTS_ERR                 ,

        MEMCH_SET_OK                            ,
        MEMCH_SET_ERR                           ,
        MEMCH_GET_OK                            ,
        MEMCH_GET_ERR                           ,
        MEMCH_DEL_OK                            ,
        MEMCH_DEL_ERR                           ,

        COMPOUND                                // x
    };


    struct Retcode {
        uint32_t ret_code;
        std::string aux_str;

        Retcode();
        Retcode(uint32_t);
        Retcode(uint32_t, const char*);

        void append_str(uint32_t);
        void append_str(uint32_t, const char*);
    };
}

typedef hartebeest::Retcode     hb_retcode;

