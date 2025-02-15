cmake_minimum_required(VERSION 3.20)
set (CMAKE_CXX_STANDARD 20)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
else()
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

project(QuasarEngine)

set(CMAKE_CXX_FLAGS_RELEASE "")
set(CMAKE_C_FLAGS_RELEASE "")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQS_DEBUG")

if(APPLE)
    message("Building on Apple macOS or iOS")
    set(VULKAN_PATH "/Users/duke/VulkanSDK/1.3.275.0/macOS")
    add_definitions(-DQS_PLATFORM_APPLE)
    set(ENV{VK_ICD_FILENAMES} "${VULKAN_PATH}/share/vulkan/icd.d/MoltenVK_icd.json")
    set(ENV{VK_LAYER_PATH} "${VULKAN_PATH}/share/vulkan/explicit_layer.d")
elseif(WIN32)
    message("Building on Windows")
    add_compile_options("$<$<CONFIG:Release>:/Od>")
    set(VULKAN_PATH $ENV{VULKAN_SDK})
    add_definitions(-DQS_PLATFORM_WINDOWS)
elseif(UNIX)
    message("Building on Unix-like OS (including Linux)")
    add_definitions(-DQS_PLATFORM_LINUX)
endif()

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
add_subdirectory(Quasar)
add_subdirectory(Editor)

if(WIN32)
add_custom_target(
    custom-build ALL
    COMMAND "${PROJECT_SOURCE_DIR}/build-shaders.bat" ${VULKAN_PATH}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "custom build jobs"
)
else()
add_custom_target(
    custom-build ALL
    COMMAND chmod +x "${PROJECT_SOURCE_DIR}/build-shaders.sh"
    COMMAND "${PROJECT_SOURCE_DIR}/build-shaders.sh" ${VULKAN_PATH}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "custom build jobs"
)
endif()