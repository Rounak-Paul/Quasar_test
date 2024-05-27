file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/**/*.cpp
    src/*.cpp
    src/**/*.h
    src/*.h
    src/**/*.inl
)
add_library(Quasar SHARED ${SOURCES})
set_target_properties(Quasar PROPERTIES LINKER_LANGUAGE CXX)
target_compile_definitions(Quasar PRIVATE QS_BUILD_DLL)

add_subdirectory(Vendor/GLFW)
add_subdirectory(Vendor/GLM)

target_precompile_headers(Quasar PUBLIC src/qspch.h)

target_include_directories(Quasar PUBLIC 
    src
    ${PROJECT_SOURCE_DIR}/Quasar/Vendor/GLFW/include
    ${PROJECT_SOURCE_DIR}/Quasar/Vendor/GLM
    ${VULKAN_PATH}/include
    ${PROJECT_SOURCE_DIR}/Quasar/Vendor/STB
    ${PROJECT_SOURCE_DIR}/Quasar/Vendor/TINYOBJLOADER
)

target_link_directories(Quasar PUBLIC ${VULKAN_PATH}/lib)

target_link_libraries(Quasar PUBLIC 
    glfw
    glm
)

if(APPLE)
    target_link_libraries(Quasar PUBLIC 
        ${VULKAN_PATH}/lib/libvulkan.1.dylib
        ${VULKAN_PATH}/lib/libvulkan.1.3.275.dylib
    )
elseif(WIN32)
    target_link_libraries(Quasar PUBLIC 
        ${VULKAN_PATH}/lib/vulkan-1.lib
    )
endif()