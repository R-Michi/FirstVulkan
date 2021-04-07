#include "VulkanApp.h"
#include <iostream>
#include <fstream>

#define ASSERT_VULKAN(value)\
		if(value != VK_SUCCESS)\
			asm("int $3");			// causes a break in the program

inline const char* strdevice_type(VkPhysicalDeviceType);

FirstVulkan::FirstVulkan(void)
{
	this->glfw_init();
	this->vulkan_init();
}

FirstVulkan::~FirstVulkan(void)
{
	this->vulkan_destroy();
	this->glfw_destroy();
}

void FirstVulkan::vulkan_init(void)
{
	// set information about the application itself
	this->app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	this->app_info.pNext = nullptr;
	this->app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	this->app_info.pApplicationName = "First Vulkan";
	this->app_info.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	this->app_info.pEngineName = ""; 
	this->app_info.apiVersion = VK_API_VERSION_1_2;

	// read layers at instance level
	uint32_t n_instance_layers = 0;
	vkEnumerateInstanceLayerProperties(&n_instance_layers, nullptr);
	VkLayerProperties instance_layer_properties[n_instance_layers];
	vkEnumerateInstanceLayerProperties(&n_instance_layers, instance_layer_properties);

	std::cout << "Amount of instance layers at instance level: " << n_instance_layers << std::endl;
	for (int i = 0; i < n_instance_layers; i++)
	{
		uint32_t v_spec = instance_layer_properties[i].specVersion;
		uint32_t v_implementatopm = instance_layer_properties[i].implementationVersion;
		std::cout << std::endl;
		std::cout << "Layer name:             " << instance_layer_properties[i].layerName << std::endl;
		std::cout << "Specification version:  " << VK_VERSION_MAJOR(v_spec) << "." << VK_VERSION_MINOR(v_spec) << "." << VK_VERSION_PATCH(v_spec) << std::endl;
		std::cout << "Implementation version: " << VK_VERSION_MAJOR(v_implementatopm) << "." << VK_VERSION_MINOR(v_implementatopm) << "." << VK_VERSION_PATCH(v_implementatopm) << std::endl;
		std::cout << "Description:            " << instance_layer_properties[i].description << std::endl;
	}
	std::cout << "------------------------------------------------------" << std::endl;

	// read extensions at instance level
	uint32_t n_instance_extensions = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &n_instance_extensions, nullptr);
	VkExtensionProperties instance_extension_properties[n_instance_extensions];
	vkEnumerateInstanceExtensionProperties(nullptr, &n_instance_extensions, instance_extension_properties);

	std::cout << "Amount of extensions at instance level: " << n_instance_extensions << std::endl;
	for (int i = 0; i < n_instance_extensions; i++)
	{
		uint32_t v_spec = instance_extension_properties[i].specVersion;
		std::cout << std::endl;
		std::cout << "Extension name:         " << instance_extension_properties[i].extensionName << std::endl;
		std::cout << "Specification version:  " << VK_VERSION_MAJOR(v_spec) << "." << VK_VERSION_MINOR(v_spec) << "." << VK_VERSION_PATCH(v_spec) << std::endl;
	}
	std::cout << "------------------------------------------------------" << std::endl;

	std::vector<const char*> instance_layers = {
		"VK_LAYER_LUNARG_standard_validation"
	};

	// fetch requiered extensions for glfw
	uint32_t n_glfw_extensions = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&n_glfw_extensions);

	// set information for the later created instance
	VkInstanceCreateInfo instance_info;
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pNext = nullptr;
	instance_info.flags = 0;
	instance_info.pApplicationInfo = &this->app_info;
	instance_info.enabledLayerCount = instance_layers.size();
	instance_info.ppEnabledLayerNames = instance_layers.data();
	instance_info.enabledExtensionCount = n_glfw_extensions;
	instance_info.ppEnabledExtensionNames = glfw_extensions;

	// create the actual vulkan instance
	VkResult result = vkCreateInstance(&instance_info, nullptr, &this->instance);
	ASSERT_VULKAN(result);

	// create vulkan surface from glfw
	result = glfwCreateWindowSurface(this->instance, this->window, nullptr, &this->surface);
	ASSERT_VULKAN(result);

	// get the physical devices, in this case the graphics cards
	uint32_t n_physical_devices;
	result = vkEnumeratePhysicalDevices(this->instance, &n_physical_devices, nullptr);
	ASSERT_VULKAN(result);
	VkPhysicalDevice physical_devices[n_physical_devices];
	result = vkEnumeratePhysicalDevices(this->instance, &n_physical_devices, physical_devices);
	ASSERT_VULKAN(result);

	std::cout << std::endl;
	std::cout << "Amount of physical devices: " << n_physical_devices << std::endl;
	for (int i = 0; i < n_physical_devices; i++)
	{
		std::cout << "--------------- DEVICE " << i << " ---------------" << std::endl;
		print_deviceinfo(physical_devices[i]);
		std::cout << "----------------------------------------" << std::endl << std::endl;
 	}

	// Create information about the queues the application uses
	float queue_priorities[] = { 1.0f, 1.0f, 1.0f, 1.0f };	// all queues have the highest priority
	VkDeviceQueueCreateInfo device_queue_info;
	device_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_info.pNext = nullptr;
	device_queue_info.flags = 0;
	device_queue_info.queueFamilyIndex = 0;
	device_queue_info.queueCount = 1;
	device_queue_info.pQueuePriorities = queue_priorities;

	VkPhysicalDeviceFeatures used_device_features = {};	// no features used yet

	// extensions at device level
	std::vector<const char*> device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	// create information about the logical device we are creating
	VkDeviceCreateInfo device_info;
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pNext = nullptr;
	device_info.flags = 0;
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos = &device_queue_info;
	device_info.enabledLayerCount = 0;
	device_info.ppEnabledLayerNames = nullptr;
	device_info.enabledExtensionCount = device_extensions.size();
	device_info.ppEnabledExtensionNames = device_extensions.data();
	device_info.pEnabledFeatures = &used_device_features;

	// create logical device
	result = vkCreateDevice(physical_devices[0], &device_info, nullptr, &this->device);
	ASSERT_VULKAN(result); 

	VkQueue queue;
	vkGetDeviceQueue(this->device, 0, 0, &queue);

	VkBool32 surface_support = false;
	result = vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[0], 0, surface, &surface_support);
	ASSERT_VULKAN(result);

	if (!surface_support)
	{
		std::cerr << "Surface not supported!" << std::endl;
		asm("int $3");
	}

	// create swapchain info
	VkSwapchainCreateInfoKHR swap_chain_info;
	swap_chain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swap_chain_info.pNext = nullptr;
	swap_chain_info.flags = 0;
	swap_chain_info.surface = this->surface;
	swap_chain_info.minImageCount = 3;	// TODO: check if valid
	swap_chain_info.imageFormat = VK_FORMAT_B8G8R8A8_UNORM; // TODO: check if valid
	swap_chain_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; // TODO: check if valid
	swap_chain_info.imageExtent = { WIDTH, HEIGHT };
	swap_chain_info.imageArrayLayers = 1;
	swap_chain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swap_chain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swap_chain_info.queueFamilyIndexCount = 0;
	swap_chain_info.pQueueFamilyIndices = nullptr;
	swap_chain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // image can also be rotated
	swap_chain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swap_chain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR; // TODO: check if valid
	swap_chain_info.clipped = VK_TRUE;
	swap_chain_info.oldSwapchain = VK_NULL_HANDLE; // is needed if swap chain is modified, e.g. when the window resizes

	// chreate actual swapchain
	result = vkCreateSwapchainKHR(this->device, &swap_chain_info, nullptr, &this->swapchain);
	ASSERT_VULKAN(result);

	// retrieve images for drawing from swapchain
	vkGetSwapchainImagesKHR(this->device, this->swapchain, &this->n_images_swapchain, nullptr);
	VkImage images_swapchain[n_images_swapchain];
	result = vkGetSwapchainImagesKHR(this->device, this->swapchain, &this->n_images_swapchain, images_swapchain);
	ASSERT_VULKAN(result);

	// create image view for every image of the swapchain
	this->image_views = new VkImageView[this->n_images_swapchain];
	for (int i = 0; i < this->n_images_swapchain; i++)
	{
		// create image view info 
		VkImageViewCreateInfo image_view_info;
		image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_info.pNext = nullptr;
		image_view_info.flags = 0;
		image_view_info.image = images_swapchain[i];
		image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_info.format = VK_FORMAT_B8G8R8A8_UNORM; // TODO: check if valid
		image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_info.subresourceRange.baseMipLevel = 0;
		image_view_info.subresourceRange.levelCount = 1;
		// can be used for VR
		image_view_info.subresourceRange.baseArrayLayer = 0;
		image_view_info.subresourceRange.layerCount = 1;

		// create actual image view
		result = vkCreateImageView(this->device, &image_view_info, nullptr, this->image_views + i);
		ASSERT_VULKAN(result);
	}

	/* Creating shaders can be improved and more automatized. */
	std::vector<char> shadercode_main_vert, shadercode_main_frag;
	// read shader code
	this->read_shader("../../../shader/spir-v/vert.spv", shadercode_main_vert);
	this->read_shader("../../../shader/spir-v/frag.spv", shadercode_main_frag);

	// create shader modules for every shader
	result = this->create_shader_moudle(shadercode_main_vert, &this->shadermodule_main_vert);
	ASSERT_VULKAN(result);
	this->create_shader_moudle(shadercode_main_vert, &this->shadermodule_main_frag);
	ASSERT_VULKAN(result);

	// create shader stage infos
	VkPipelineShaderStageCreateInfo shader_stage_main_vert;
	shader_stage_main_vert.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_main_vert.pNext = nullptr;
	shader_stage_main_vert.flags = 0;
	shader_stage_main_vert.stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_stage_main_vert.module = this->shadermodule_main_vert;
	shader_stage_main_vert.pName = "main";					// main function of shader
	shader_stage_main_vert.pSpecializationInfo = nullptr;	// can be used to optimize constants and expressions where the constants are used

	VkPipelineShaderStageCreateInfo shader_stage_main_frag;
	shader_stage_main_frag.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_main_frag.pNext = nullptr;
	shader_stage_main_frag.flags = 0;
	shader_stage_main_frag.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stage_main_frag.module = this->shadermodule_main_frag;
	shader_stage_main_frag.pName = "main";					// main function of shader
	shader_stage_main_vert.pSpecializationInfo = nullptr;	// can be used to optimize constants and expressions where the constants are used

	VkPipelineShaderStageCreateInfo shader_stages_main[] = {
		shader_stage_main_vert,
		shader_stage_main_frag
	}; 

	// create vertex shader input info
	VkPipelineVertexInputStateCreateInfo vertex_input_info;
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.pNext = nullptr;
	vertex_input_info.flags = 0;
	vertex_input_info.vertexBindingDescriptionCount = 0;		// i think thats the equivalent to glBindBuffer
	vertex_input_info.pVertexBindingDescriptions = nullptr;
	vertex_input_info.vertexAttributeDescriptionCount = 0;		// and this the equivalent to glVertexAttribPointer
	vertex_input_info.pVertexAttributeDescriptions = nullptr;

	// create input assembly info
	VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
	input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_info.pNext = nullptr;
	input_assembly_info.flags = 0;
	input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // equivalent: GL_TRIANGLES 
	input_assembly_info.primitiveRestartEnable = VK_FALSE;

	// setup viewport similar to glViewPort
	VkViewport view_port;
	view_port.x = 0.0f;
	view_port.y = 0.0f;
	view_port.width = WIDTH;
	view_port.height = HEIGHT;
	view_port.minDepth = 0.0f;
	view_port.maxDepth = 1.0f;

	// defines what sould be shown at the screen, everything else will be cut off -> name scissor
	VkRect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = { WIDTH, HEIGHT };

	// create viewport state info
	VkPipelineViewportStateCreateInfo viewport_state_info;
	viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_info.pNext = nullptr;
	viewport_state_info.flags = 0;
	viewport_state_info.viewportCount = 1; 
	viewport_state_info.pViewports = &view_port;
	viewport_state_info.scissorCount = 1;
	viewport_state_info.pScissors = &scissor;

	// create rasterizer state info
	VkPipelineRasterizationStateCreateInfo rasterizer_info;
	rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer_info.pNext = nullptr;
	rasterizer_info.flags = 0;
	rasterizer_info.depthClampEnable = VK_FALSE;
	rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
	rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;		// back face culling
	rasterizer_info.frontFace = VK_FRONT_FACE_CLOCKWISE;	// in OpenGL counter clockwise
	rasterizer_info.depthBiasEnable = VK_FALSE;
	rasterizer_info.depthBiasConstantFactor = 0;
	rasterizer_info.depthBiasClamp = 0.0f;
	rasterizer_info.depthBiasSlopeFactor = 0.0f;
	rasterizer_info.lineWidth = 1.0f;

	// create multisample info -> antialiasing
	VkPipelineMultisampleStateCreateInfo multisample_state_info;
	multisample_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_info.pNext = nullptr;
	multisample_state_info.flags = 0;
	multisample_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisample_state_info.sampleShadingEnable = VK_FALSE; // activate / deactivate multisampling
	multisample_state_info.minSampleShading = 1.0f;
	multisample_state_info.pSampleMask = nullptr;
	multisample_state_info.alphaToCoverageEnable = VK_FALSE;
	multisample_state_info.alphaToOneEnable = VK_FALSE;

	// configure color blending -> used to render transparency
	VkPipelineColorBlendAttachmentState color_blend_attachment;
	color_blend_attachment.blendEnable = VK_TRUE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.colorWriteMask = 0x0000000F; // enable every color channel (RGBA) 

	VkPipelineColorBlendStateCreateInfo color_blend_info;
	color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_info.pNext = nullptr;
	color_blend_info.flags = 0;
	color_blend_info.logicOpEnable = VK_FALSE;
	color_blend_info.logicOp = VK_LOGIC_OP_NO_OP;
	color_blend_info.attachmentCount = 1;
	color_blend_info.pAttachments = &color_blend_attachment;
	color_blend_info.blendConstants[0] = 0.0f;	// r
	color_blend_info.blendConstants[1] = 0.0f;	// g
	color_blend_info.blendConstants[2] = 0.0f;	// b
	color_blend_info.blendConstants[3] = 0.0f;	// a

	// create pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_info;
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.pNext = nullptr;
	pipeline_layout_info.flags = 0;
	pipeline_layout_info.setLayoutCount = 0;
	pipeline_layout_info.pSetLayouts = nullptr;
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = nullptr;

	result = vkCreatePipelineLayout(this->device, &pipeline_layout_info, nullptr, &this->pipeline_layout);
	ASSERT_VULKAN(result);
}

void FirstVulkan::glfw_init(void)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);	// tell glfw that we use vulkan
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	this->window = glfwCreateWindow(WIDTH, HEIGHT, "First Vulkan", nullptr, nullptr);
}

void FirstVulkan::vulkan_destroy(void)
{
	vkDeviceWaitIdle(this->device);

	vkDestroyPipelineLayout(this->device, this->pipeline_layout, nullptr);
	vkDestroyShaderModule(this->device, this->shadermodule_main_vert, nullptr);
	vkDestroyShaderModule(this->device, this->shadermodule_main_frag, nullptr);
	for (int i = 0; i < this->n_images_swapchain; i++)
		vkDestroyImageView(this->device, this->image_views[i], nullptr);
	delete[] this->image_views;
	vkDestroySwapchainKHR(this->device, this->swapchain, nullptr);
	vkDestroyDevice(this->device, nullptr);
	vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
	vkDestroyInstance(this->instance, nullptr);
}

void FirstVulkan::glfw_destroy(void)
{
	glfwDestroyWindow(this->window);
	glfwTerminate();
}

void FirstVulkan::run(void)
{
	while (!glfwGetKey(this->window, GLFW_KEY_ESCAPE) && !glfwWindowShouldClose(this->window))
	{
		glfwPollEvents();
	}
}

void FirstVulkan::print_deviceinfo(const VkPhysicalDevice& device)
{
	/* READ MATCHING DEVICE IN AN REAL APPLICATION */
	// read properties of physical device
	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(device, &device_properties);
	uint32_t version = device_properties.apiVersion;
	std::cout << "Device name:               " << device_properties.deviceName << std::endl;
	std::cout << "Driver version:            " << device_properties.driverVersion << std::endl;
	std::cout << "Vendor ID:                 " << device_properties.vendorID << std::endl;
	std::cout << "Device ID:                 " << device_properties.deviceID << std::endl;
	std::cout << "Device Type:               " << strdevice_type(device_properties.deviceType) << std::endl;
	std::cout << "Supported Vulkan-version:  " << VK_VERSION_MAJOR(version) << "." << VK_VERSION_MINOR(version) << "." << VK_VERSION_PATCH(version) << std::endl;
	std::cout << "Discrete queue priorities: " << device_properties.limits.discreteQueuePriorities << std::endl;

	/* CHECK IF FEATURES YOU WANT ARE SUPPORTED FOR A REAL APPLICATION */
	// read features of physical device
	VkPhysicalDeviceFeatures device_features;
	vkGetPhysicalDeviceFeatures(device, &device_features);
	std::cout << "Geometry shader support:   " << ((device_features.geometryShader) ? "YES" : "NO") << std::endl;

	// read memory properties pf physical device (just to demonstrate how it works)
	VkPhysicalDeviceMemoryProperties device_memprop;
	vkGetPhysicalDeviceMemoryProperties(device, &device_memprop);

	/* READ BEST SUITED QUEUE FAMILIES FOR A REAL APPLICATION*/
	// read queue families
	uint32_t n_queue_families;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &n_queue_families, nullptr);
	VkQueueFamilyProperties family_prop[n_queue_families];
	vkGetPhysicalDeviceQueueFamilyProperties(device, &n_queue_families, family_prop);
	
	std::cout << "Amount of queue families:  " << n_queue_families << std::endl;
	for (int i = 0; i < n_queue_families; i++)
	{
		VkQueueFlags queue_flags = family_prop[i].queueFlags;
		std::cout << std::endl;
		std::cout << "Queue family #" << i << std::endl;
		std::cout << "Can compute graphics:             " << ((queue_flags & VK_QUEUE_GRAPHICS_BIT) ? "Yes" : "No") << std::endl;
		std::cout << "Can compute non-graphical stuff:  " << ((queue_flags & VK_QUEUE_COMPUTE_BIT) ? "Yes" : "No") << std::endl;
		std::cout << "Can transfer data:                " << ((queue_flags & VK_QUEUE_TRANSFER_BIT) ? "Yes" : "No") << std::endl;
		std::cout << "Number of queues of the family:   " << family_prop[i].queueCount << std::endl;
		std::cout << "Number of valid timestamps:       " << family_prop[i].timestampValidBits << std::endl;
	}
	std::cout << std::endl;

	/* FOR REAL APPLICATION CHECK THE CAPABILITIES AND FORMATS, IF THEY ARE SUITED FOR WHAT YOU WANT TO DO */
	// read surface capabilities
	VkSurfaceCapabilitiesKHR surface_capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, this->surface, &surface_capabilities);
	std::cout << "Surface capabilities:" << std::endl;
	std::cout << "\tminimal image count:        " << surface_capabilities.minImageCount << std::endl;
	std::cout << "\tmaximal image count:        " << surface_capabilities.maxImageCount << std::endl;
	std::cout << "\tcurrent extent:             " << surface_capabilities.currentExtent.width << "/" << surface_capabilities.currentExtent.height << std::endl;
	std::cout << "\tminimal image extent:       " << surface_capabilities.minImageExtent.width << "/" << surface_capabilities.minImageExtent.height << std::endl;
	std::cout << "\tmaximal image extent:       " << surface_capabilities.maxImageExtent.width << "/" << surface_capabilities.maxImageExtent.height << std::endl;
	std::cout << "\tmaximal image array layers: " << surface_capabilities.maxImageArrayLayers << std::endl;
	std::cout << "\tsupported transformations:  " << surface_capabilities.supportedTransforms << std::endl;
	std::cout << "\tcurrent transform:          " << surface_capabilities.currentTransform << std::endl;
	std::cout << "\tsupported composite alpha:  " << surface_capabilities.supportedCompositeAlpha << std::endl;
	std::cout << "\tsupported usage flags:      " << surface_capabilities.supportedUsageFlags << std::endl;

	uint32_t n_formats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface, &n_formats, nullptr);
	VkSurfaceFormatKHR surface_formats[n_formats];
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface, &n_formats, surface_formats);

	std::cout << std::endl;
	std::cout << "Amount of normats: " << n_formats << std::endl;
	for (int i = 0; i < n_formats; i++)
	{
		std::cout << "Format: " << surface_formats[i].format << std::endl;
	}

	/* ALSO THE PRESENTATION MODE SHOULD BE CHECKED FOR YOUR REQUIREMENTS */
	// read presetation modes
	uint32_t n_presentation_modes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->surface, &n_presentation_modes, nullptr);
	VkPresentModeKHR presentation_modes[n_presentation_modes];
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->surface, &n_presentation_modes, presentation_modes);

	std::cout << std::endl;
	std::cout << "Amount of presentation modes: " << n_presentation_modes << std::endl;
	for (int i = 0; i < n_presentation_modes; i++)
	{
		std::cout << "Presentation mode: " << presentation_modes[i] << std::endl;
	}
}

size_t FirstVulkan::read_shader(const char* path, std::vector<char>& buff)
{
	std::fstream shader(path, std::ios::in | std::ios::binary | std::ios::ate); // std::ios::ate begin reading backwards -> cursor at the last character
	if (!shader) return 0;

	size_t n_bytes = shader.tellg();	// actual cursor position -> number of bytes in a file
	shader.seekg(0);						// set cursor back to the first charactor of the file
	buff.resize(n_bytes);
	shader.read(buff.data(), n_bytes);
	shader.close(); 
	return n_bytes;
}

VkResult FirstVulkan::create_shader_moudle(const std::vector<char>& code, VkShaderModule* shader_module)
{
	// create shader info
	VkShaderModuleCreateInfo shader_info;
	shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_info.pNext = nullptr;
	shader_info.flags = 0;
	shader_info.codeSize = code.size();
	shader_info.pCode = (uint32_t*)code.data();

	return vkCreateShaderModule(this->device, &shader_info, nullptr, shader_module);
}

inline const char* strdevice_type(VkPhysicalDeviceType type)
{
	switch (type)
	{
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		return "Integrated GPU, GPU is embedded";
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		return "External GPU, GPU is a seperate processor";
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		return "Virtual GPU, GPU is a virtual node in a virtualization environment";
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		return "CPU, the device is running on the same processor as the host";
	default:
		return "Unknown device";
	}
}