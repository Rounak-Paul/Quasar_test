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

        static RendererAPI& GetInstance() {return *s_instance;}

        static b8 Init(String appName);
        void Shutdown();

        private:
        static RendererAPI* s_instance;
        RendererBackend::Backend m_backend;
    };
    #define QS_RENDERER_API RendererAPI::GetInstance()
} // namespace Quasar
