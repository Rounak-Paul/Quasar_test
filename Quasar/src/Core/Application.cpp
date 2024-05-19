#include "Application.h"

#include <Renderer/RendererAPI.h>

namespace Quasar
{
    Application* Application::s_instance = nullptr;

    Application::Application(AppState state) : m_state{state} {
        assert(!s_instance);
        s_instance = this;

        QS_CORE_INFO("Starting Quasar Enging...")

        QS_CORE_INFO("Initializing Log...")
        if (!Log::Init()) {QS_CORE_ERROR("Log failed to Initialize")}

        QS_CORE_INFO("Initializing Event System...")
        if (!Event::Init()) {QS_CORE_ERROR("Event system failed to Initialize")}

        QS_CORE_INFO("Initializing Renderer...")
        if (!QS_RENDERER_API.Init(state.app_name)) {QS_CORE_ERROR("Renderer failed to Initialize")}

        QS_EVENT.Register(EVENT_CODE_RESIZED, 0, ApplicationOnResized);
    }

    Application::~Application() {
        
    }

    void Application::Run() {
        m_prevTime = std::chrono::high_resolution_clock::now();
        u32 frameCount = 0;
        f32 clk1Hz = 0.;

        while(!m_window.ShouldClose()) {
            if (m_state.suspended) { 
                m_window.WaitEvents();
                continue; 
            } 
            m_window.PollEvents();

            // clock update and dt
            m_currentTime = std::chrono::high_resolution_clock::now();
            dt = GetDT();
            m_state.dt = dt;
            clk1Hz += dt;

            if (clk1Hz > 1.f) {
                clk1Hz = 0.f;
                QS_CORE_TRACE("FPS: %d", frameCount);
                frameCount = 0;
            }
            frameCount++;
            m_prevTime = m_currentTime;
        }

        // Shutdown Engine
        QS_EVENT.Unregister(EVENT_CODE_RESIZED, 0, ApplicationOnResized);

        QS_RENDERER_API.Shutdown();
        QS_EVENT.Shutdown();
        Log::Shutdown();
    }

    b8 Application::ApplicationOnResized(u16 code, void* sender, void* listenerInst, EventContext context) {
        if (code == EVENT_CODE_RESIZED) {
            u16 width = context.data.u16[0];
            u16 height = context.data.u16[1];

            QS_CORE_TRACE("w/h: %d/%d", width, height)

            if (width == 0 || height == 0) {
                QS_CORE_INFO("Application suspended")
                QS_APP_STATE.suspended = true;
            }
            else {

                QS_APP_STATE.suspended = false;
            }

            return true;
        }
        return false;
    }


} // namespace Quasar
