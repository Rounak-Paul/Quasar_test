#pragma once

#include "Defines.h"

namespace Quasar
{
    typedef enum log_level {
        LOG_LEVEL_FATAL = 0,
        LOG_LEVEL_ERROR = 1,
        LOG_LEVEL_WARN = 2,
        LOG_LEVEL_INFO = 3,
        LOG_LEVEL_DEBUG = 4,
        LOG_LEVEL_TRACE = 5
    } log_level;

    class QS_API Log {
        public:
        static b8 init();
        static void shutdown();
        static void core_log_output(log_level level, const char* msg, ...);
        static void app_log_output(log_level level, const char* msg, ...);
        static Log& get_instance() {return instance;} 

        private:
        static Log instance;
    };
} // namespace Quasar

#define QS_CORE_FATAL(msg, ...) Log::get_instance().core_log_output(LOG_LEVEL_FATAL, msg, ##__VA_ARGS__);
#define QS_CORE_ERROR(msg, ...) Log::get_instance().core_log_output(LOG_LEVEL_ERROR, msg, ##__VA_ARGS__);
#define QS_CORE_WARN(msg, ...) Log::get_instance().core_log_output(LOG_LEVEL_WARN, msg, ##__VA_ARGS__);
#define QS_CORE_INFO(msg, ...) Log::get_instance().core_log_output(LOG_LEVEL_INFO, msg, ##__VA_ARGS__);

#ifdef QS_DEBUG
    #define QS_CORE_DEBUG(msg, ...) Log::get_instance().core_log_output(LOG_LEVEL_DEBUG, msg, ##__VA_ARGS__);
    #define QS_CORE_TRACE(msg, ...) Log::get_instance().core_log_output(LOG_LEVEL_TRACE, msg, ##__VA_ARGS__);
#else 
    #define QS_CORE_DEBUG(msg, ...)
    #define QS_CORE_TRACE(msg, ...)
#endif

#define QS_APP_FATAL(msg, ...) Quasar::Log::get_instance().app_log_output(Quasar::LOG_LEVEL_FATAL, msg, ##__VA_ARGS__);
#define QS_APP_ERROR(msg, ...) Quasar::Log::get_instance().app_log_output(Quasar::LOG_LEVEL_ERROR, msg, ##__VA_ARGS__);
#define QS_APP_WARN(msg, ...) Quasar::Log::get_instance().app_log_output(Quasar::LOG_LEVEL_WARN, msg, ##__VA_ARGS__);
#define QS_APP_INFO(msg, ...) Quasar::Log::get_instance().app_log_output(Quasar::LOG_LEVEL_INFO, msg, ##__VA_ARGS__);

#ifdef QS_DEBUG
    #define QS_APP_DEBUG(msg, ...) Quasar::Log::get_instance().app_log_output(Quasar::LOG_LEVEL_DEBUG, msg, ##__VA_ARGS__);
    #define QS_APP_TRACE(msg, ...) Quasar::Log::get_instance().app_log_output(Quasar::LOG_LEVEL_TRACE, msg, ##__VA_ARGS__);
#else 
    #define QS_APP_DEBUG(msg, ...)
    #define QS_APP_TRACE(msg, ...)
#endif