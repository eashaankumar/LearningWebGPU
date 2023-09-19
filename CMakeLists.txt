cmake_minimum_required(VERSION 3.0...3.25)

add_subdirectory(glm)
add_subdirectory(glfw)
add_subdirectory(webgpu) # Add after glfw to avoid dawn's implementation
add_subdirectory(glfw3webgpu)

include_directories(entt)
include_directories(headers)

add_executable(App 
        main.cpp
        src/utils.cpp src/renderer.cpp src/engine.cpp
        entt/entt.hpp headers/utils.hpp headers/renderer.hpp headers/engine.hpp
    )

set_target_properties(App PROPERTIES
    CXX_STANDARD 17
    COMPILE_WARNING_AS_ERROR ON
)

target_link_libraries(App PRIVATE glm glfw webgpu glfw3webgpu)

if (MSVC)
    target_compile_options(App PRIVATE /W4)
else()
    target_compile_options(App PRIVATE -Wall -Wextra -pedantic)
endif()


if(XCODE)
    set_target_properties(App PROPERTIES
        XCODE_GENERATE_SCHEME ON
        XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal")
endif()


target_copy_webgpu_binaries(App)