/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_logger.cc
 */

#include <string>

#include "../extern/spdlog/spdlog.h"
#include "./includes/hb_logger.hh"

hartebeest::ConsoleLogger::ConsoleLogger(const char* id) {
    name = std::string(id);
    shared_logger = spdlog::stdout_color_mt(name).get();
    spdlog::set_pattern("[%n:%^%l%$] %v");

}


spdlog::logger* hartebeest::ConsoleLogger::get_logger() {
    return shared_logger;
};