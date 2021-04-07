#pragma once

#define GLFW_INCLUDE_VULKAN	// includes vulkan internally in GLFW
#include <GLFW/glfw3.h>
#include <vector>

class FirstVulkan 
{
private:
	VkApplicationInfo app_info;
	VkInstance instance;
	VkDevice device;			// logical device
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	uint32_t n_images_swapchain;
	VkImageView* image_views;
	VkShaderModule shadermodule_main_vert, shadermodule_main_frag;
	VkPipelineLayout pipeline_layout;
	
	GLFWwindow* window;
	
	static constexpr uint32_t WIDTH = 400;
	static constexpr uint32_t HEIGHT = 300; 

	void vulkan_init(void);
	void glfw_init(void);
	void vulkan_destroy(void);
	void glfw_destroy(void);

	void print_deviceinfo(const VkPhysicalDevice&);
	size_t read_shader(const char* path, std::vector<char>& buff);
	VkResult create_shader_moudle(const std::vector<char>& code, VkShaderModule* shader_module);

public:
	FirstVulkan(void);
	virtual ~FirstVulkan(void);

	void run(void);
};