#include "Log.h"


namespace Quasar
{
    Log Log::instance;

    const  char* level_strings[6] = {"\033[1;31m[FATAL]: ", "\033[1;31m[ERROR]: ", "\033[1;33m[WARN] : ", "\033[1;32m[INFO] : ", "\033[1;34m[DEBUG]: ", "\033[1;36m[TRACE]: "};

    b8 Log::init() {
        return TRUE;
    }

    void Log::shutdown() {
        
    }

        void Log::core_log_output(log_level level, const char* msg, ...) {
            char out_message[32000];

            #if QS_PLATFORM_WINDOWS
            va_list arg_ptr;
            #else
            __builtin_va_list arg_ptr;
            #endif
            
            va_start(arg_ptr, msg);
            vsnprintf(out_message, 32000, msg, arg_ptr);
            va_end(arg_ptr);

            char out_message2[32000];
            snprintf(out_message2, sizeof(out_message2), "%s%s%s%s\n","\033[1;45m[QUASAR]\033[0m ", level_strings[level], out_message, "\033[0m");

            // TODO: platform-specific output.
            std::cout << out_message2;

        }

        void Log::app_log_output(log_level level, const char* msg, ...) {
            char out_message[32000];

            #if QS_PLATFORM_WINDOWS
            va_list arg_ptr;
            #else
            __builtin_va_list arg_ptr;
            #endif
            
            va_start(arg_ptr, msg);
            vsnprintf(out_message, 32000, msg, arg_ptr);
            va_end(arg_ptr);

            char out_message2[32000];
            snprintf(out_message2, sizeof(out_message2), "%s%s%s%s\n", "\033[1;42m[APP]   \033[0m ", level_strings[level], out_message, "\033[0m");

            // TODO: platform-specific output.
            std::cout << out_message2;

        }
} // namespace Quasar
