#pragma once 

extern Quasar::Application* Quasar::create_application();

int main(int argc, char** argv)
{
	auto app = Quasar::create_application();
	app->run();
	delete app;

    return 0;
}