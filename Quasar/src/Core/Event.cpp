#include "Event.h"

namespace Quasar
{
    Event* Event::instance = nullptr;

    Event::Event() {
        assert(!instance);
        instance = this;
    }

    Event::~Event() {

    }

    b8 Event::init() {
        event_system_state event_state;
        Event();
        return true;
    }

    void Event::shutdown() {
        
    }

    b8 Event::Register(u16 code, void* listener, PFN_on_event on_event) {
        size_t registered_count = event_state.registered[code].events.size();
        for(size_t i = 0; i < registered_count; ++i) {
            if(event_state.registered[code].events[i].listener == listener) {
                QS_CORE_WARN("Duplicate event listener was issued!");
                return false;
            }
        }

        // If at this point, no duplicate was found. Proceed with registration.
        registered_event event;
        event.listener = listener;
        event.callback = on_event;
        event_state.registered[code].events.push_back(event);

        return true;
    }

    b8 Event::Unregister(u16 code, void* listener, PFN_on_event on_event) {
        // On nothing is registered for the code, boot out.
        if(event_state.registered[code].events.size() == 0) {
            QS_CORE_WARN("Event list is empty");
            return false;
        }

        u64 registered_count = event_state.registered[code].events.size();
        for (u64 i = 0; i < registered_count; ++i) {
            registered_event &e = event_state.registered[code].events[i];
            if (e.listener == listener && e.callback == on_event) {
                // Found the element to remove
                event_state.registered[code].events.erase(event_state.registered[code].events.begin() + i);
                return true; 
            }
        }

        // Not found.
        return false;
    }

    b8 Event::Execute(u16 code, void* sender, event_context context) {
        // If nothing is registered for the code, boot out.
        if(event_state.registered[code].events.size() == 0) {
            return false;
        }

        u64 registered_count = event_state.registered[code].events.size();
        for(auto item : event_state.registered[code].events) {
            if(item.callback(code, sender, item.listener, context)) {
                // Message has been handled, do not send to other listeners.
                return true;
            }
        }

        // Not found.
        return false;
    }
} // namespace Quasar
