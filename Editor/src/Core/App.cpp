#include <Quasar.h>
#include <EntryPoint.h>

class App : public Quasar::Application
{
public:
	App(Quasar::AppState state)
        : Application(state) { 
        }
		
	~App() { }
};

Quasar::Application* Quasar::CreateApplication()
{
    Quasar::AppState state;
    state.width = 800;
    state.height = 600;
    state.app_name = "Editor - Quasar Engine";
	return new App(state);
};