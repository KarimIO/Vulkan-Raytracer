cmake_minimum_required(VERSION 3.26)

project(VulkanRaytracer)

# Enable C++ Modules
set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API 2182bf5c-ef0d-489a-91da-49dbc3090d2a)
set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
set(CMAKE_CXX_STANDARD 23)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /experimental:module")

find_package(Vulkan REQUIRED)
add_subdirectory( libraries/glfw )

add_executable(VulkanRaytracer)
target_sources(VulkanRaytracer PRIVATE
	FILE_SET cxx_modules TYPE CXX_MODULES
	BASE_DIRS
		${PROJECT_SOURCE_DIR}
	FILES
		src/Buffer.ixx
		src/Camera.ixx
		src/ComputeDescriptorSet.ixx
		src/ComputePipeline.ixx
		src/DebugWindow.ixx
		src/DescriptorPool.ixx
		src/DescriptorSet.ixx
		src/GraphicsPipeline.ixx
		src/RaytracerEngine.ixx
		src/RaytracerTargetImage.ixx
		src/ReadFile.ixx
		src/Renderer.ixx
		src/Sampler.ixx
		src/Texture.ixx
		src/UniformBuffer.ixx
		src/VulkanCore.ixx
)

target_sources(
	VulkanRaytracer PRIVATE
		src/Main.ixx
		src/StbImpl.cpp
		"${CMAKE_CURRENT_SOURCE_DIR}/libraries/imgui/imgui.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/libraries/imgui/imgui.cpp"
		
		"${CMAKE_CURRENT_SOURCE_DIR}/libraries/imgui/imgui_demo.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/libraries/imgui/imgui_draw.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/libraries/imgui/imgui_tables.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/libraries/imgui/imgui_widgets.cpp"

		"${CMAKE_CURRENT_SOURCE_DIR}/libraries/imgui/backends/imgui_impl_vulkan.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/libraries/imgui/backends/imgui_impl_glfw.cpp"
)

target_compile_definitions(
	VulkanRaytracer PRIVATE
		GLFW_INCLUDE_VULKAN
)

target_include_directories(
	VulkanRaytracer PRIVATE
		libraries/
		libraries/imgui/
		libraries/glfw/include
		libraries/glm/
		libraries/stb/
		${Vulkan_INCLUDE_DIR}
)

target_link_directories(
	VulkanRaytracer PRIVATE
		libraries/glfw/src/Debug
		libraries/glfw/src/Release
)

target_link_libraries(
	VulkanRaytracer PRIVATE
		glfw3.lib
		${Vulkan_LIBRARIES}
)

add_custom_target(
	BuildShaders
	COMMENT
	    "Compile Shader Modules\n"
	VERBATIM
)

add_custom_command(
	TARGET
		BuildShaders
	COMMAND
		glslc -fshader-stage=vertex ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/Fullscreen.vert.glsl -o ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/Fullscreen.vert.spv
	COMMENT
		"Creating Fullscreen Vertex Shader"
)

add_custom_command(
	TARGET
		BuildShaders
	COMMAND
		${Vulkan_GLSLC_EXECUTABLE} -fshader-stage=fragment ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/Fullscreen.frag.glsl -o ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/Fullscreen.frag.spv
	COMMENT
		"Creating Fullscreen Fragment Shader"
)

add_custom_command(
	TARGET
		BuildShaders
	COMMAND
		${Vulkan_GLSLC_EXECUTABLE} -fshader-stage=compute ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/Raytracing.comp.glsl -o ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/Raytracing.comp.spv
	COMMENT
		"Creating Raytrace Compute Shader"
)
