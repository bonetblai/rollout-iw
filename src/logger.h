// (c) 2017 Blai Bonet

#ifndef LOGGER_H
#define LOGGER_H

#include <cassert>
#include <iostream>
#include <string>
#include "utils.h"

// Inspired by logger in Arcade Learning Environment

class Logger {
  public:
    enum mode_t {
      Debug = 0,
      Info = 1,
      Warning = 2,
      Error = 3,
      Stats = 4,
      Silent = 5
    };

    enum color_t {
      color_normal = 0,
      color_red = 1,
      color_green = 2,
      color_yellow = 3,
      color_blue = 4,
      color_magenta = 5,
      color_cyan = 6
    };

    struct Mode {
        mode_t mode_;
        int debug_severity_;
        bool continuation_;
        Mode(mode_t mode, int debug_severity = 0, bool continuation = false)
          : mode_(mode),
            debug_severity_(debug_severity),
            continuation_(continuation) {
        }
    };

    struct Continuation : public Mode {
        Continuation(const Mode &mode)
          : Mode(mode.mode_, mode.debug_severity_, true) {
        }
        Continuation(mode_t mode, int debug_severity)
          : Mode(mode, debug_severity, true) {
        }
    };

    struct DebugMode : public Mode {
        DebugMode(int severity, bool continuation = false)
          : Mode(Logger::Debug, severity, continuation) {
        }
    };

  public:
    Logger() { }
    ~Logger() { }

    static void set_output_stream(std::ostream &output_stream) {
        Logger::output_stream_ = &output_stream;
    }
    static void set_mode(mode_t mode) {
        Logger::current_mode_ = mode;
    }
    static int current_mode() {
        return Logger::current_mode_;
    }
    static void set_debug_threshold(int debug_threshold) {
        Logger::current_debug_threshold_ = debug_threshold;
    }
    static int current_debug_threshold() {
        return Logger::current_debug_threshold_;
    }
    static void set_use_color(bool use_color) {
        Logger::use_color_ = use_color;
    }
    static bool use_color() {
        return Logger::use_color_;
    }

    static bool available() {
        return Logger::output_stream_ != nullptr;
    }
    static std::ostream& output_stream() {
        return *Logger::output_stream_;
    }

    static std::string prefix(mode_t log) {
        std::string str;
        if( log == Debug )
            str += Logger::blue() + "Logger::Debug: ";
        else if( log == Info )
            str += Logger::green() + "Logger::Info: ";
        else if( log == Warning )
            str += Logger::magenta() + "Logger::Warning: ";
        else if( log == Error )
            str += Logger::red() + "Logger::Error: ";
        else if( log == Stats )
            str += Logger::yellow() + "Logger::Stats: ";
        return str + Logger::normal();
    }

    static std::string color(color_t color) {
        if( Logger::use_color_ ) {
            if( color == Logger::color_normal )
                return Utils::normal();
            else if( color == Logger::color_red )
                return Utils::red();
            else if( color == Logger::color_green )
                return Utils::green();
            else if( color == Logger::color_yellow )
                return Utils::yellow();
            else if( color == Logger::color_blue )
                return Utils::blue();
            else if( color == Logger::color_magenta )
                return Utils::magenta();
            else if( color == Logger::color_cyan )
                return Utils::cyan();
        }
        return "";
    }
    static std::string normal() {
        return Logger::color(Logger::color_normal);
    }
    static std::string red() {
        return Logger::color(Logger::color_red);
    }
    static std::string green() {
        return Logger::color(Logger::color_green);
    }
    static std::string yellow() {
        return Logger::color(Logger::color_yellow);
    }
    static std::string blue() {
        return Logger::color(Logger::color_blue);
    }
    static std::string magenta() {
        return Logger::color(Logger::color_magenta);
    }
    static std::string cyan() {
        return Logger::color(Logger::color_cyan);
    }

  protected:
    static std::ostream *output_stream_;
    static mode_t current_mode_;
    static int current_debug_threshold_;
    static bool use_color_;
};

template<typename T>
inline Logger::Mode operator<<(Logger::mode_t log, const T &value) {
    if( Logger::available() && (log >= Logger::current_mode()) )
        Logger::output_stream() << Logger::prefix(log) << value;
    return Logger::Mode(log, 0, true);
}

template<typename T>
inline Logger::Mode operator<<(Logger::Mode mode, const T &value) {
    if( Logger::available() && (mode.mode_ >= Logger::current_mode()) ) {
        if( (mode.mode_ == Logger::Debug) && (mode.debug_severity_ >= Logger::current_debug_threshold()) ) {
            if( !mode.continuation_ )
                Logger::output_stream() << Logger::prefix(mode.mode_);
            Logger::output_stream() << value;
        } else if( mode.mode_ != Logger::Debug ) {
            if( !mode.continuation_ )
                Logger::output_stream() << Logger::prefix(mode.mode_);
            Logger::output_stream() << value;
        }
    }
    return Logger::Mode(mode.mode_, mode.debug_severity_, true);
}

inline Logger::Mode operator<<(Logger::mode_t log, std::ostream& (*manip)(std::ostream&)) {
    if( Logger::available() && (log >= Logger::current_mode()) )
        manip(Logger::output_stream());
    return Logger::Mode(log, 0, true);
}

inline Logger::Mode operator<<(Logger::Mode mode, std::ostream& (*manip)(std::ostream&)) {
    if( Logger::available() && (mode.mode_ >= Logger::current_mode()) ) {
        if( (mode.mode_ == Logger::Debug) && (mode.debug_severity_ >= Logger::current_debug_threshold()) ) {
            manip(Logger::output_stream());
        } else if( mode.mode_ != Logger::Debug ) {
            manip(Logger::output_stream());
        }
    }
    return Logger::Mode(mode.mode_, mode.debug_severity_, true);
}

#endif

