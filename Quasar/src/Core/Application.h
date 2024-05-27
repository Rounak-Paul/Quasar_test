#pragma once
#include <qspch.h>
#include <chrono>

#include "Window.h"
#include <Core/Event.h>

namespace Quasar
{
    typedef struct QS_API AppState {
        String app_name;
        u32 width;
        u32 height;

        f32 dt;

        b8 suspended = false;
    } AppState;

    class QS_API Application
    {
    public:
        Application(AppState state);
        ~Application();

        Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

        static Application& GetInstance() {return *s_instance;}
        Window& GetMainWindow() {return m_window;}
        AppState& GetAppState() {return m_state;}
        void Run();
    
    private:
        static Application* s_instance;
        AppState m_state;
        Window m_window{m_state.width, m_state.height, m_state.app_name.c_str()};

        static b8 ApplicationOnResized(u16 code, void* sender, void* listenerInst, EventContext context);

        // time
        std::chrono::high_resolution_clock::time_point m_prevTime;
        std::chrono::high_resolution_clock::time_point m_currentTime;
        f32 dt;
        inline f32 GetDT() {
            return static_cast<std::chrono::duration<f32>>((m_currentTime - m_prevTime)).count();
        }
    };

    Application* CreateApplication();

#define QS_APPLICATION Application::GetInstance()
#define QS_APP_STATE QS_APPLICATION.GetAppState()
#define QS_MAIN_WINDOW QS_APPLICATION.GetMainWindow()
} // namespace Quasar