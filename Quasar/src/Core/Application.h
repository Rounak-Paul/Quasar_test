#pragma once
#include <qspch.h>
#include <chrono>
#include <Defines.h>

#include "Window.h"
#include <Core/Event.h>

namespace Quasar
{
    typedef struct QS_API app_state {
        std::string app_name;
        u32 width;
        u32 height;

        f32 dt;

        b8 suspended = FALSE;
    } app_state;

    class QS_API Application
    {
    public:
        Application(app_state state);
        ~Application();

        Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

        static Application& get_instance() {return *instance;}
        Window& get_main_window() {return window;}
        app_state& get_app_state() {return state;}
        void run();
    
    private:
        static Application* instance;
        app_state state;
        Window window{state.width, state.height, state.app_name.c_str()};

        static b8 application_on_resized(u16 code, void* sender, void* listener_inst, event_context context);

        // time
        std::chrono::high_resolution_clock::time_point prev_time;
        std::chrono::high_resolution_clock::time_point current_time;
        f32 dt;
        inline f32 get_dt() {
            return static_cast<std::chrono::duration<f32>>((current_time - prev_time)).count();
        }
    };

    Application* create_application();

#define QS_APPLICATION Application::get_instance()
#define QS_APP_STATE QS_APPLICATION.get_app_state()
#define QS_MAIN_WINDOW QS_APPLICATION.get_main_window()
} // namespace Quasar