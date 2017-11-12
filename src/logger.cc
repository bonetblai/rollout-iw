// (c) 2017 Blai Bonet

#include "logger.h"

std::ostream* Logger::output_stream_ = nullptr;
Logger::mode_t Logger::current_mode_ = Logger::Silent;
int Logger::current_debug_threshold_ = 0;
bool Logger::use_color_ = false;

