#pragma once

/* github.com/sjoon-oh/hartebeest
 * Author: Sukjoon Oh, sjoon@kaist.ac.kr
 * hb_logger.hpp
 */

#include <memory>

#include "../../extern/spdlog/spdlog.h"
#include "../../extern/spdlog/sinks/stdout_color_sinks.h"
#include "../../extern/spdlog/sinks/basic_file_sink.h"


namespace hartebeest {
    class ConsoleLogger {
        std::string name;
        spdlog::logger* shared_logger;
    
    public:
        ConsoleLogger(const char*);
        static ConsoleLogger& get_instance() {
            static ConsoleLogger global_logger("HB");
            return global_logger;
        }

        spdlog::logger* get_logger();
    };
}

#define HB_CLOGGER \
    hartebeest::ConsoleLogger::get_instance().get_logger()
