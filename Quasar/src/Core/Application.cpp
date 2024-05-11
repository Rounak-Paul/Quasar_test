#include "Application.h"

namespace Quasar
{
    Application* Application::instance = nullptr;

    Application::Application(app_state state) : state{state} {
        assert(!instance);
        instance = this;
    }

    Application::~Application() {
        
    }

    void Application::run() {
        prev_time = std::chrono::high_resolution_clock::now();
        u8 frame_count = 0;
        f32 clk_1Hz = 0.;

        while(!window.should_close()) {
            window.poll_events();
            if (state.suspended) { continue; } 

            // clock update and dt
            current_time = std::chrono::high_resolution_clock::now();
            dt = get_dt();
            state.dt = dt;
            clk_1Hz += dt;

            if (clk_1Hz > 1.f) {
                clk_1Hz = 0.f;
                std::cout << "FPS: " << 1/dt << std::endl;
            }
            frame_count++;
            prev_time = current_time;
        }
    }


} // namespace Quasar
