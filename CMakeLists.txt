# requiered CMAKE version to build the project
cmake_minimum_required (VERSION 3.8)

# current project
project("FirstVulkan")

include_directories("C:/VulkanSDK/1.2.170.0/Include"
					"./lib/glfw-3.3.3/include"
					"./lib/glm")

link_directories("C:/VulkanSDK/1.2.170.0/Lib"
				 "./lib/glfw-3.3.3/lib"
				 "./lib/glm/glm/lib")

add_executable(first_vulkan "main.cpp" "VulkanApp.cpp")
target_link_libraries(first_vulkan PRIVATE "-lglm_static" "-lglfw3" "-lvulkan-1")

# additional work
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
