#pragma once
#include <qspch.h>

#include <Renderer/VulkanBackend/backend.h>

namespace Quasar
{
    class RendererAPI {
        public:
        RendererAPI() {};
        ~RendererAPI() {};

        RendererAPI(const RendererAPI&) = delete;
		RendererAPI& operator=(const RendererAPI&) = delete;

        static RendererAPI& GetInstance() {return *s_instance;}

        static b8 Init(String appName);
        void Shutdown();

        void DrawFrame(f32 dt);
        void Resize();

        private:
        static RendererAPI* s_instance;
        Scope<Renderer::Backend> m_backend;
    };
    #define QS_RENDERER_API RendererAPI::GetInstance()
} // namespace Quasar
