#pragma once
#include <qspch.h>
#include "Event.h"
#include "Keycodes.h"

namespace Quasar
{
    typedef struct keyboard_state {
        b32 keys[QS_KEY_MAX];
    } keyboard_state;

    typedef struct mouse_state {
        u8 buttons[QS_MBTN_MAX];
    } mouse_state;

    class QS_API Input {
        public:
        // To be defined in Platform
        ~Input() = default;
        static b8 Init();
        void Shutdown();
        static Input& GetInstance() {return *instance;}
        static inline b32 GetKeyState(KeyCode key) { return keyboard_state.keys[key]; };
        static inline b32 GetMbtnState(MouseCode btn) { return mouse_state.buttons[btn]; };
		static glm::vec2 GetMousePosition();
		static f32 GetMouseX();
		static f32 GetMouseY();
        
        private:
        Input() {};
        static Input* instance;
        static keyboard_state keyboard_state;
        static mouse_state mouse_state;

        static void IsKeyPressed(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void IsMbtnPressed(GLFWwindow* window, int button, int action, int mods);
    };

    #define QS_INPUT Input::GetInstance()
} // namespace Quasar