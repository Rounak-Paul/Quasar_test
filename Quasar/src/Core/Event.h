#pragma once
#include <qspch.h>

#define MAX_MESSAGE_CODES 16384

namespace Quasar
{
    typedef struct EventContext {
        // 128 bytes
        union {
            i64 i64[2];
            u64 u64[2];
            f64 f64[2];

            i32 i32[4];
            u32 u32[4];
            f32 f32[4];

            i16 i16[8];
            u16 u16[8];

            i8 i8[16];
            u8 u8[16];

            char c[16];
        } data;
    } EventContext;

    typedef b8 (*PFN_on_event)(u16 code, void* sender, void* listener_inst, EventContext data);

    typedef struct RegisteredEvent {
        void* listener;
        PFN_on_event callback;
    } RegisteredEvent;

    typedef struct EventCodeEntry {
        std::vector<RegisteredEvent> events;
    } EventCodeEntry;

    typedef struct EventSystemState {
        // Lookup table for event codes.
        EventCodeEntry registered[MAX_MESSAGE_CODES];
    } EventSystemState;

    class QS_API Event {
        public:
        Event() {};
        ~Event() {};

        Event(const Event&) = delete;
		Event& operator=(const Event&) = delete;

        static b8 Init();
        void Shutdown();

        static Event& GetInstance() {return *s_instance;}

        b8 Register(u16 code, void* listener, PFN_on_event on_event);
        b8 Unregister(u16 code, void* listener, PFN_on_event on_event);
        b8 Execute(u16 code, void* sender, EventContext context);

        private:
        EventSystemState* m_eventState;
        static Event* s_instance;
    };

    // System internal event codes. Application should use codes beyond 255.
    typedef enum SystemEventCode {
        // Shuts the application down on the next frame.
        EVENT_CODE_APPLICATION_QUIT = 0x01,

        // Keyboard key pressed.
        /* Context usage:
        * u16 key_code = data.data.u16[0];
        */
        EVENT_CODE_KEY_PRESSED = 0x02,

        // Keyboard key released.
        /* Context usage:
        * u16 key_code = data.data.u16[0];
        */
        EVENT_CODE_KEY_RELEASED = 0x03,

        // Mouse button pressed.
        /* Context usage:
        * u16 button = data.data.u16[0];
        */
        EVENT_CODE_BUTTON_PRESSED = 0x04,

        // Mouse button released.
        /* Context usage:
        * u16 button = data.data.u16[0];
        */
        EVENT_CODE_BUTTON_RELEASED = 0x05,

        // Mouse moved.
        /* Context usage:
        * u16 x = data.data.u16[0];
        * u16 y = data.data.u16[1];
        */
        EVENT_CODE_MOUSE_MOVED = 0x06,

        // Mouse moved.
        /* Context usage:
        * u8 z_delta = data.data.u8[0];
        */
        EVENT_CODE_MOUSE_WHEEL = 0x07,

        // Resized/resolution changed from the OS.
        /* Context usage:
        * u16 width = data.data.u16[0];
        * u16 height = data.data.u16[1];
        */
        EVENT_CODE_RESIZED = 0x08,

        // Change the render mode for debugging purposes.
        /* Context usage:
        * i32 mode = context.data.i32[0];
        */
        EVENT_CODE_SET_RENDER_MODE = 0x0A,

        EVENT_CODE_DEBUG0 = 0x10,
        EVENT_CODE_DEBUG1 = 0x11,
        EVENT_CODE_DEBUG2 = 0x12,
        EVENT_CODE_DEBUG3 = 0x13,
        EVENT_CODE_DEBUG4 = 0x14,

        EVENT_CODE_MAX = 0xFF
    } SystemEventCode;

    #define QS_EVENT Event::GetInstance()
} // namespace Quasar
