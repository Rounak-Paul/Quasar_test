#include "RendererAPI.h"

#include <Core/Application.h>

namespace Quasar
{
    RendererAPI* RendererAPI::instance = nullptr;

    RendererAPI::RendererAPI() {
        assert(!instance);
        instance = this;
    }

    b8 RendererAPI::init(String app_name) {
        RendererAPI();

        #ifdef QS_PLATFORM_APPLE
            u16 width = QS_MAIN_WINDOW.get_extent().width*2;
            u16 height = QS_MAIN_WINDOW.get_extent().height*2;
        #else
            u16 width = QS_MAIN_WINDOW.get_extent().width;
            u16 height = QS_MAIN_WINDOW.get_extent().height;
        #endif
        if(!QS_RENDERER_API.backend.init(app_name, width, height)) {
            return false;
        }

        return true;
    }

    void RendererAPI::shutdown() {

    }
} // namespace Quasar
