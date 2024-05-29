//> includes
#include "backend.h"

#include "vk_initializers.h"
#include "vk_types.h"

#include <chrono>
#include <thread>
//< includes

//> init

namespace Quasar::Renderer {

constexpr bool bUseValidationLayers = false;

Backend* loadedEngine = nullptr;

Backend& Backend::Get() { return *loadedEngine; }

b8 Backend::init()
{
    // only one engine initialization is allowed with the application.
    assert(loadedEngine == nullptr);
    loadedEngine = this;

    // everything went fine
    _isInitialized = true;

    return true;
}
//< init

//> extras
void Backend::shutdown()
{
    if (_isInitialized) {

        
    }

    // clear engine pointer
    loadedEngine = nullptr;
}

void Backend::draw()
{
    // nothing yet
}
//< extras

}
