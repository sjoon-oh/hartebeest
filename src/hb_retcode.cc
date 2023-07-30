/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_retcode.cc
 */

#include <cstdint>

#include "./includes/hb_retcode.hh"

namespace hartebeest {
    
    const char* hb_retcstr[] = {
        "UNDEFINED"                             ,   // 0

        "HCA: DEVICE RESET OK"                  ,   // 1
        "HCA: DEVICE RESET ERROR"               ,   // 2
        "HCA: DEVICE OPEN OK"                   ,   // 3
        "HCA: DEVICE OPEN ERROR"                ,   // 4
        "HCA: DEVICE REGISTER OK"               ,   // 5

        "HCAINITR: DEVICE OPEN OK"              ,   // 6
        "HCAINITR: DEVICE LIST RANGE ERROR"     ,   // 7
        "HCAINITR: NO DEVICE FOUND ERROR"       ,   // 8
        "HCAINITR: NO HCA CONTEXT FOUND ERROR"  ,   // 9
        "HCAINITR: PORT QUERY ERROR"            ,   // 10
        "HCAINITR: NOT INFINIBAND LAYER"        ,   // 11
        "HCAINITR: PORT BINDING OK"             ,   // 12

        "CACHE: ALREADY REGISTERD"              ,   // 13
        "CACHE: RESRC NOT FOUND"                ,   // 14 
        "CACHE: REGISTERED OK"                  ,   // 15
        
        "PD: MR CREATE ERROR"                   ,   // 16
        "PD: MR CRATE OK"                       ,   // 17
        "PD: ib_reg_mr ERROR"                   ,   // 18

        "CFGLDR: CONFIGURATION FILE NOT FOUND"  ,   // 19
        "CFGLDR: ENVVAR NOT FOUND"              ,   // 20
        "CFGLDR: GENERAL OK"                    ,   // 21
        "CFGLDR: JSON PARSER THROW"             ,   // 22
        "CFGLDR: KEY NOT FOUND"                 ,   // 23
        
        "QP: TRANSITION TO INIT OK"             ,   // 24
        "QP: TRANSITION TO INIT ERROR"          ,   // 25

        "MEMCACHED: SET OK"                     ,   // 26
        "MEMCACHED: SET FAILED"                 ,   // 27
        "MEMCACHED: GET OK"                     ,   // 28
        "MEMCACHED: GET FAILED"                 ,   // 29
        
        "COMPOUND"                                  // x
    };
}

hartebeest::Retcode::Retcode() : ret_code(UNDEFINED) {
    aux_str = hartebeest::hb_retcstr[ret_code];
}

hartebeest::Retcode::Retcode(uint32_t ret_code) 
    : ret_code(ret_code) {
        aux_str = hartebeest::hb_retcstr[ret_code];
}

hartebeest::Retcode::Retcode(uint32_t ret_code, const char* aux_str) : ret_code(ret_code) {
    this->aux_str = std::string(aux_str);
}

void hartebeest::Retcode::append_str(uint32_t ret_code) {
    ret_code = COMPOUND;
    
    std::string next_identifier(", ");
    std::string append_str(hartebeest::hb_retcstr[ret_code]);

    aux_str.append(next_identifier);
    aux_str.append(append_str);
}

void hartebeest::Retcode::append_str(uint32_t ret_code, const char* aux_str) {
    ret_code = COMPOUND;

    std::string next_identifier(", ");
    std::string append_str(aux_str);

    this->aux_str.append(next_identifier);
    this->aux_str.append(append_str);
}
