#pragma once

#define GLFW_INCLUDE_VULKAN	// includes vulkan internally in GLFW
#include <GLFW/glfw3.h>
#include <vector>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

class FirstVulkan 
{
	struct vertex_t
	{
		glm::vec3 pos;
		glm::vec4 color;
		glm::vec2 uv_coord;

		static void get_binding_description(VkVertexInputBindingDescription& description);
		static void get_attrib_descriptions(std::vector<VkVertexInputAttributeDescription>& descriptions);
	};

private:
	VkApplicationInfo app_info;
	VkInstance instance;
	VkPhysicalDevice* physical_devices;
	VkDevice device;			// logical device
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	uint32_t n_images_swapchain;
	VkImageView* image_views;
	VkFramebuffer* fbos_swapchain;
	VkShaderModule shadermodule_main_vert, shadermodule_main_frag;
	VkPipelineLayout pipeline_layout;
	VkRenderPass renderpass;
	VkPipeline pipeline;
	VkCommandPool cmd_pool;
	VkCommandBuffer* cmd_buffers;
	VkSemaphore semaphore_img_aviable;		// first render step
	VkSemaphore semaphore_rendering_done;	// second render step
	VkQueue queue;

	VkBuffer vertex_buffer;
	VkBuffer index_buffer;
	VkBuffer uniform_buffer;
	VkDeviceMemory vertex_buffer_memory;
	VkDeviceMemory index_buffer_memory;
	VkDeviceMemory uniform_buffer_memory;

	VkImage texture1_image;
	VkDeviceMemory texture1_memory;
	VkImageView texture1_view;
	VkImageLayout texture1_layout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	VkSampler texture1_sampler;

	VkImage depth_image;
	VkDeviceMemory depth_memory;
	VkImageView depth_image_view;


	VkDebugReportCallbackEXT debug_report_callback; 
	
	GLFWwindow* window;
	uint32_t width;
	uint32_t height; 

	static constexpr VkFormat COLOR_FORMAT = VK_FORMAT_B8G8R8A8_UNORM; // TODO: check if valid

	std::vector<vertex_t> vertices;
	std::vector<uint32_t> indices;
	glm::mat4 MVP;
	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSet descriptor_set;
	double t_app_start;

	void vulkan_create_app_info(void);
	void vulkan_create_instance(void);
	void vulkan_create_glfw_window_surface(void);
	void vulkan_create_device(void);
	void vulkan_create_queues(void);
	void vulkan_check_surface_support(void);
	void vulkan_create_swapchain(void);
	void vulkan_create_image_views(void); 
	void vulkan_create_render_pass(void);
	void vulkan_create_shader_modules(void);
	void vulkan_create_descriptor_set_layout(void);
	void vulkan_create_pipeline(void);
	void vulkan_create_framebuffers(void);
	void vulkan_create_command_pool(void);
	void vulkan_create_command_buffers(void);
	void vulkan_create_semaphores(void);
	void vulkan_load_texture(void);
	void vulkan_create_vertex_buffer(void);
	void vulkan_create_uniform_buffer(void);
	void vulkan_create_descriptor_pool(void);
	void vulkan_create_descriptor_set(void);
	void vulkan_record_command_buffers(void);
	void vulkan_recrate_swapchain(void);
	uint32_t vulkan_find_mem_type_index(uint32_t type_filter, VkMemoryPropertyFlags properties);
	void vulkan_create_buffer(VkDeviceSize size, VkBufferUsageFlags usage_flags, VkBuffer& buffer, VkMemoryPropertyFlags mem_flags, VkDeviceMemory& device_mem);
	void vulkan_create_depth_image(VkPhysicalDevice physicalDevice, VkQueue queue);
	void vulkan_copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
	void vulkan_change_layout(VkCommandPool cmd_pool, VkQueue queue, VkImage img, VkFormat format, VkImageLayout& old_layout, VkImageLayout new_layout);
	void vulkan_write_buffer_to_image(VkCommandPool cmd_pool, VkQueue queue, VkBuffer buff, int w, int h);
	bool vulkan_is_format_supported(VkPhysicalDevice device, VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags flags);
	VkFormat vulkan_find_supported_format(VkPhysicalDevice device, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags flags);
	VkFormat vulkan_find_depth_format(VkPhysicalDevice physical_device);
	bool vulkan_is_stencil_format(VkFormat format);
	void vulkan_init(void);

	template<typename T>
	void create_and_upload_buffer(std::vector<T> data, VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& mem)
	{
		VkDeviceSize buffer_size = data.size() * sizeof(T);

		// create staging buffer
		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory = {};	// GPU buffer must read from staging buffer
		this->vulkan_create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, staging_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer_memory);

		// load data into the staging buffer
		void* _data;
		vkMapMemory(this->device, staging_buffer_memory, 0, buffer_size, 0, &_data);
		memcpy(_data, data.data(), buffer_size);
		vkUnmapMemory(this->device, staging_buffer_memory);
		// create vertex buffer														// GPU must write to vertex buffer						// Buffer is in VRAM
		this->vulkan_create_buffer(buffer_size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mem);

		// copy staging buffer to vertex buffer
		this->vulkan_copy_buffer(staging_buffer, buffer, buffer_size);

		// destroy temporary staging buffer
		vkDestroyBuffer(this->device, staging_buffer, nullptr);
		vkFreeMemory(this->device, staging_buffer_memory, nullptr);
	}

	void glfw_on_window_resize(GLFWwindow* window, int width, int height);
	void glfw_init(void);

	void vulkan_destroy(void);
	void glfw_destroy(void);

	void update_mvp(void);
	void draw_frame(void);

	void print_deviceinfo(const VkPhysicalDevice* devices, size_t n);
	void print_instance_layers(const VkLayerProperties* layers, size_t n);
	void print_instance_extensions(const VkExtensionProperties* extensions, size_t n);

	size_t read_shader(const char* path, std::vector<char>& buff);
	VkResult create_shader_moudle(const std::vector<char>& code, VkShaderModule* shader_module);

public:
	FirstVulkan(void);
	virtual ~FirstVulkan(void);

	void run(void);
};