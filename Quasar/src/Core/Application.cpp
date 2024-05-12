#include "Application.h"

namespace Quasar
{
    Application* Application::instance = nullptr;

    Application::Application(app_state state) : state{state} {
        assert(!instance);
        instance = this;

        QS_CORE_INFO("Starting Quasar Enging...")

        QS_CORE_INFO("Initializing Log...")
        if (!Log::init()) {QS_CORE_ERROR("Log failed to Initialize")}

        QS_CORE_INFO("Initializing Event System...")
        if (!Event::init()) {QS_CORE_ERROR("Event system failed to Initialize")}

        QS_EVENT.Register(EVENT_CODE_RESIZED, 0, application_on_resized);
    }

    Application::~Application() {
        
    }

    void Application::run() {
        prev_time = std::chrono::high_resolution_clock::now();
        u32 frame_count = 0;
        f32 clk_1Hz = 0.;

        while(!window.should_close()) {
            window.poll_events();
            if (state.suspended) { 
                window.wait_events();
                continue; 
            } 

            // clock update and dt
            current_time = std::chrono::high_resolution_clock::now();
            dt = get_dt();
            state.dt = dt;
            clk_1Hz += dt;

            if (clk_1Hz > 1.f) {
                clk_1Hz = 0.f;
                QS_CORE_TRACE("FPS: %d", frame_count);
                frame_count = 0;
            }
            frame_count++;
            prev_time = current_time;
        }

        // Shutdown Engine
        QS_EVENT.Unregister(EVENT_CODE_RESIZED, 0, application_on_resized);

        Log::shutdown();
    }

    b8 Application::application_on_resized(u16 code, void* sender, void* listener_inst, event_context context) {
        if (code == EVENT_CODE_RESIZED) {
            u16 width = context.data.u16[0];
            u16 height = context.data.u16[1];

            QS_CORE_TRACE("w/h: %d/%d", width, height)

            if (width == 0 || height == 0) {
                QS_CORE_INFO("Application suspended")
                QS_APP_STATE.suspended = TRUE;
            }
            else {
                QS_APP_STATE.suspended = FALSE;
            }

            return TRUE;
        }
        return FALSE;
    }


} // namespace Quasar
