#include <Quasar.h>
#include <EntryPoint.h>

class App : public Quasar::Application
{
public:
	App(Quasar::app_state state)
        : Application(state) { 
        }
		
	~App() { }
};

Quasar::Application* Quasar::create_application()
{
    Quasar::app_state state;
    state.width = 800;
    state.height = 600;
    state.app_name = "Editor - Quasar Engine";
	return new App(state);
};