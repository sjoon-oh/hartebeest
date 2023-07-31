/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_retcode.cc
 */

#include <cstdint>

#include "./includes/hb_retcode.hh"

namespace hartebeest {
    
    const char* hb_retcstr[] = {
        "UNDEFINED"                             ,

        "HCA: DEVICE RESET OK"                  ,
        "HCA: DEVICE RESET ERROR"               ,
        "HCA: DEVICE OPEN OK"                   ,
        "HCA: DEVICE OPEN ERROR"                ,
        "HCA: DEVICE REGISTER OK"               ,

        "HCAINITR: DEVICE OPEN OK"              ,
        "HCAINITR: DEVICE LIST RANGE ERROR"     ,
        "HCAINITR: NO DEVICE FOUND ERROR"       ,
        "HCAINITR: NO HCA CONTEXT FOUND ERROR"  ,
        "HCAINITR: PORT QUERY ERROR"            ,
        "HCAINITR: NOT INFINIBAND LAYER"        ,
        "HCAINITR: PORT BINDING OK"             ,

        "CACHE: ALREADY REGISTERD"              ,
        "CACHE: RESRC NOT FOUND"                ,
        "CACHE: REGISTERED OK"                  ,
        
        "PD: MR CREATE ERROR"                   ,
        "PD: MR CRATE OK"                       ,
        "PD: ib_reg_mr ERROR"                   ,
        "PD: QP CREATE ERROR"                   ,
        "PD: QP CREATE OK"                      ,

        "CFGLDR: CONFIGURATION FILE NOT FOUND"  ,
        "CFGLDR: ENVVAR NOT FOUND"              ,
        "CFGLDR: GENERAL OK"                    ,
        "CFGLDR: JSON PARSER THROW"             ,
        "CFGLDR: KEY NOT FOUND"                 ,
        
        "QP: TRANSITION TO INIT OK"             ,
        "QP: TRANSITION TO INIT ERROR"          ,
        "QP: TRANSITION TO RTR OK"              ,
        "QP: TRANSITION TO RTR ERROR"           ,
        "QP: TRANSITION TO RTS OK"              ,
        "QP: TRANSITION TO RTS ERROR"           ,


        "MEMCACHED: SET OK"                     ,
        "MEMCACHED: SET FAILED"                 ,
        "MEMCACHED: GET OK"                     ,
        "MEMCACHED: GET FAILED"                 ,
        "MEMCACHED: DEL OK"                     ,
        "MEMCACHED: DEL FAILED"                 ,
        
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
