#include "Event.h"

namespace Quasar
{
    Event* Event::s_instance = nullptr;

    Event::Event() {
        assert(!s_instance);
        s_instance = this;
    }

    Event::~Event() {

    }

    b8 Event::Init() {
        Event();
        EventSystemState event_state;
        return true;
    }

    void Event::Shutdown() {
        
    }

    b8 Event::Register(u16 code, void* listener, PFN_on_event on_event) {
        size_t registeredCount = m_eventState.registered[code].events.size();
        for(size_t i = 0; i < registeredCount; ++i) {
            if(m_eventState.registered[code].events[i].listener == listener) {
                QS_CORE_WARN("Duplicate event listener was issued!");
                return false;
            }
        }

        // If at this point, no duplicate was found. Proceed with registration.
        RegisteredEvent event;
        event.listener = listener;
        event.callback = on_event;
        m_eventState.registered[code].events.push_back(event);

        return true;
    }

    b8 Event::Unregister(u16 code, void* listener, PFN_on_event on_event) {
        // On nothing is registered for the code, boot out.
        if(m_eventState.registered[code].events.size() == 0) {
            QS_CORE_WARN("Event list is empty");
            return false;
        }

        u64 registeredCount = m_eventState.registered[code].events.size();
        for (u64 i = 0; i < registeredCount; ++i) {
            RegisteredEvent &e = m_eventState.registered[code].events[i]; // Use reference to modify the element if needed
            if (e.listener == listener && e.callback == on_event) {
                // Found the element to remove
                m_eventState.registered[code].events.erase(m_eventState.registered[code].events.begin() + i);
                return true; // Assuming TRUE is a typo and you meant true (lowercase)
            }
        }

        // Not found.
        return false;
    }

    b8 Event::Execute(u16 code, void* sender, EventContext context) {
        // If nothing is registered for the code, boot out.
        if(m_eventState.registered[code].events.size() == 0) {
            return false;
        }

        u64 registered_count = m_eventState.registered[code].events.size();
        for(auto item : m_eventState.registered[code].events) {
            if(item.callback(code, sender, item.listener, context)) {
                // Message has been handled, do not send to other listeners.
                return true;
            }
        }

        // Not found.
        return false;
    }
} // namespace Quasar
