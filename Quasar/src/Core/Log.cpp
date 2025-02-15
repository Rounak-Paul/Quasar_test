#include "Log.h"

#include <qspch.h>

namespace Quasar
{
    Log Log::s_instance;

    const  char* level_strings[6] = {"\033[1;31m[FATAL]: ", "\033[1;31m[ERROR]: ", "\033[1;33m[WARN] : ", "\033[1;32m[INFO] : ", "\033[1;34m[DEBUG]: ", "\033[1;36m[TRACE]: "};

    b8 Log::Init() {
        return true;
    }

    void Log::Shutdown() {

    }

    void Log::CoreLogOutput(LogLevel level, const char* msg, ...) {
        char out_msg[32000];

        #if QS_PLATFORM_WINDOWS
        va_list arg_ptr;
        #else 
        __builtin_va_list arg_ptr;
        #endif

        va_start(arg_ptr, msg);
        vsnprintf(out_msg, 32000, msg, arg_ptr);
        va_end(arg_ptr);

        char out_msg1[32000];
        snprintf(out_msg1, sizeof(out_msg1), "%s%s%s%s\n","\033[1;45m[QUASAR]\033[0m ", level_strings[level], out_msg, "\033[0m");

        std::cout << out_msg1;
    }
    void Log::AppLogOutput(LogLevel level, const char* msg, ...) {
        char out_msg[32000];

        #if QS_PLATFORM_WINDOWS
        va_list arg_ptr;
        #else 
        __builtin_va_list arg_ptr;
        #endif

        va_start(arg_ptr, msg);
        vsnprintf(out_msg, 32000, msg, arg_ptr);
        va_end(arg_ptr);

        char out_msg1[32000];
        snprintf(out_msg1, sizeof(out_msg1), "%s%s%s%s\n","\033[1;42m[APP]   \033[0m ", level_strings[level], out_msg, "\033[0m");
        std::cout << out_msg1;
    }


} // namespace Quasar
