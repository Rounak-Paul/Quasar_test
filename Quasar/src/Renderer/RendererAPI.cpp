#include "RendererAPI.h"

#include <Core/Application.h>

namespace Quasar
{
    RendererAPI* RendererAPI::s_instance = nullptr;

    RendererAPI::RendererAPI() {
        assert(!s_instance);
        s_instance = this;
    }

    b8 RendererAPI::Init(String appName) {
        RendererAPI();

        #ifdef QS_PLATFORM_APPLE
            u16 width = QS_MAIN_WINDOW.GetExtent().width*2;
            u16 height = QS_MAIN_WINDOW.GetExtent().height*2;
        #else
            u16 width = QS_MAIN_WINDOW.get_extent().width;
            u16 height = QS_MAIN_WINDOW.get_extent().height;
        #endif
        if(!QS_RENDERER_API.m_backend.Init(appName, width, height)) {
            return false;
        }

        return true;
    }

    void RendererAPI::Shutdown() {

    }
} // namespace Quasar
