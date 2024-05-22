#include "RendererAPI.h"

#include <Core/Application.h>

namespace Quasar
{
    RendererAPI* RendererAPI::s_instance = nullptr;

    b8 RendererAPI::Init(String appName) {
        assert(!s_instance);
        s_instance = new RendererAPI();

        #ifdef QS_PLATFORM_APPLE
            u16 width = QS_MAIN_WINDOW.GetExtent().width*2;
            u16 height = QS_MAIN_WINDOW.GetExtent().height*2;
        #else
            u16 width = QS_MAIN_WINDOW.GetExtent().width;
            u16 height = QS_MAIN_WINDOW.GetExtent().height;
        #endif
        s_instance->m_backend = std::make_unique<RendererBackend::Backend>();
        if(!s_instance->m_backend->Init(appName, width, height)) {
            return false;
        }

        return true;
    }

    void RendererAPI::Shutdown() {
        m_backend->Shutdown();
    }

    void RendererAPI::DrawFrame() {
        m_backend->DrawFrame();
    }
    
    void RendererAPI::Resize() {
        m_backend->Resize();
    }
} // namespace Quasar
