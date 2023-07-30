#pragma once

/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * rets.hpp
 */

#include <string>

namespace hartebeest {

    enum EncodedRetCodes {
        UNDEFINED = 0                           ,       // 0

        HCA_RETCODE_RESET_OK                    ,       // 1
        HCA_RETCODE_RESET_ERR                   ,       // 2
        HCA_RETCODE_OPEN_OK                     ,       // 3
        HCA_RETCODE_OPEN_ERR                    ,       // 4
        HCA_RETCODE_REGISTER_OK                 ,       // 5

        HCAINITR_RETCODE_OPEN_OK                ,       // 6
        HCAINITR_RETCODE_RANGE_ERR              ,       // 7
        HCAINITR_RETCODE_UNINIT_ERR             ,       // 8
        HCAINITR_RETCODE_NOCTX_ERR              ,       // 9
        HCAINITR_RETCODE_PORT_QUERY_ERR         ,       // 10
        HCAINITR_RETCODE_NO_IB                  ,       // 11
        HCAINITR_RETCODE_BIND_PORT_OK           ,       // 12

        CACHE_RETCODE_ALREADY_EXIST_ERR         ,       // 13
        CACHE_RETCODE_NO_EXIST_ERR              ,       // 14
        CACHE_RETCODE_REGISTER_OK               ,       // 15

        PD_RETCODE_CREATE_MR_ERR                ,       // 16
        PD_RETCODE_CREATE_MR_OK                 ,       // 17
        PD_RETCODE_IB_REG_MR_ERR                ,       // 18

        CFGLDR_RETCODE_FILE_NOT_FOUND           ,       // 19
        CFGLDR_RETCODE_ENVVAR_NOT_FOUND         ,       // 20
        CFGLDR_RETCODE_OK                       ,       // 21
        CFGLDR_JSON_ERR                         ,       // 22
        CFGLDR_KEY_NOT_FOUND                    ,       // 23

        QP_TRANSITION_2_INIT_OK                 ,       // 24
        QP_TRANSITION_2_INIT_ERR                ,       // 25

        MEMCH_SET_OK                            ,       // 26
        MEMCH_SET_ERR                           ,       // 27
        MEMCH_GET_OK                            ,       // 28
        MEMCH_GET_ERR                           ,       // 29

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

