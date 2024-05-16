#pragma once
#include <qspch.h>

#include "VulkanBackend/VulkanBackend.h"

namespace Quasar
{
    class RendererAPI {
        public:
        RendererAPI();
        ~RendererAPI() = default;

        RendererAPI(const RendererAPI&) = delete;
		RendererAPI& operator=(const RendererAPI&) = delete;

        static RendererAPI& get_instance() {return *instance;}

        static b8 init(String app_name);
        void shutdown();

        private:
        static RendererAPI* instance;
        RendererBackend::Backend backend;
    };
    #define QS_RENDERER_API RendererAPI::get_instance()
} // namespace Quasar
