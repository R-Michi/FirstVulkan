#include "VulkanApp.h"
#include <iostream>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>

#define ASSERT_VULKAN(value)\
		if(value != VK_SUCCESS)\
			asm("int $3");			// causes a break in the program

inline const char* strdevice_type(VkPhysicalDeviceType);

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

void FirstVulkan::vertex_t::get_binding_description(VkVertexInputBindingDescription& description)
{
	description = {};
	description.binding = 0;
	description.stride = sizeof(vertex_t);
	description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void FirstVulkan::vertex_t::get_attrib_descriptions(std::vector<VkVertexInputAttributeDescription>& descriptions)
{
	descriptions.resize(2, {});

	descriptions[0].location = 0;
	descriptions[0].binding = 0;
	descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;	// x, y, z - float
	descriptions[0].offset = 0;

	descriptions[1].location = 1;
	descriptions[1].binding = 0;
	descriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;	// r, g, b, a - float
	descriptions[1].offset = 3 * sizeof(float);
}

FirstVulkan::FirstVulkan(void)
{
	this->vertices = {
		{ {-0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ {-0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { 0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }

	};

	this->indices = {
		0, 1, 2, 0, 3, 1
	};

	this->width		= 400;
	this->height	= 300;
	this->swapchain = VK_NULL_HANDLE;
	this->glfw_init();
	this->vulkan_init();
	this->t_app_start = glfwGetTime();
}

FirstVulkan::~FirstVulkan(void)
{
	this->vulkan_destroy();
	this->glfw_destroy();
}

void FirstVulkan::vulkan_create_app_info(void)
{
	// set information about the application itself
	this->app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	this->app_info.pNext = nullptr;
	this->app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	this->app_info.pApplicationName = "First Vulkan";
	this->app_info.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	this->app_info.pEngineName = "";
	this->app_info.apiVersion = VK_API_VERSION_1_2;
}

void FirstVulkan::vulkan_create_instance(void)
{
	// read layers at instance level
	uint32_t n_instance_layers = 0;
	vkEnumerateInstanceLayerProperties(&n_instance_layers, nullptr);
	VkLayerProperties instance_layer_properties[n_instance_layers];
	vkEnumerateInstanceLayerProperties(&n_instance_layers, instance_layer_properties);
	this->print_instance_layers(instance_layer_properties, n_instance_layers);

	// read extensions at instance level
	uint32_t n_instance_extensions = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &n_instance_extensions, nullptr);
	VkExtensionProperties instance_extension_properties[n_instance_extensions];
	vkEnumerateInstanceExtensionProperties(nullptr, &n_instance_extensions, instance_extension_properties);
	this->print_instance_extensions(instance_extension_properties, n_instance_extensions);

	// activate layers
	std::vector<const char*> instance_layers = {
		"VK_LAYER_LUNARG_standard_validation",	// vulkans validation layer
		"VK_LAYER_KHRONOS_validation"
	};

	// fetch requiered extensions for glfw
	uint32_t n_glfw_extensions = 0;
	const char** temp_extensions = glfwGetRequiredInstanceExtensions(&n_glfw_extensions);
	std::vector<const char*> instance_extensions;
	for (size_t i = 0; i < n_glfw_extensions; i++)			// glfw extensions
		instance_extensions.push_back(temp_extensions[i]);
	instance_extensions.push_back("VK_EXT_debug_utils");

	// set information for the later created instance
	VkInstanceCreateInfo instance_info = {};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pNext = nullptr;
	instance_info.flags = 0;
	instance_info.pApplicationInfo = &this->app_info;
	instance_info.enabledLayerCount = instance_layers.size();
	instance_info.ppEnabledLayerNames = instance_layers.data();
	instance_info.enabledExtensionCount = instance_extensions.size();
	instance_info.ppEnabledExtensionNames = instance_extensions.data();

	// create the actual vulkan instance
	VkResult result = vkCreateInstance(&instance_info, nullptr, &this->instance);
	ASSERT_VULKAN(result);
}

void FirstVulkan::vulkan_create_glfw_window_surface(void)
{
	// create vulkan surface from glfw
	VkResult result = glfwCreateWindowSurface(this->instance, this->window, nullptr, &this->surface);
	ASSERT_VULKAN(result);
}

void FirstVulkan::vulkan_create_device(void)
{
	// get the physical devices, in this case the graphics cards
	uint32_t n_physical_devices;
	VkResult result = vkEnumeratePhysicalDevices(this->instance, &n_physical_devices, nullptr);
	ASSERT_VULKAN(result);
	this->physical_devices = new VkPhysicalDevice[n_physical_devices];
	result = vkEnumeratePhysicalDevices(this->instance, &n_physical_devices, this->physical_devices);
	ASSERT_VULKAN(result);
	this->print_deviceinfo(this->physical_devices, n_physical_devices);

	// Create information about the queues the application uses
	float queue_priorities[] = { 1.0f, 1.0f, 1.0f, 1.0f };	// all queues have the highest priority
	VkDeviceQueueCreateInfo device_queue_info = {};
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
	VkDeviceCreateInfo device_info = {};
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
}

void FirstVulkan::vulkan_create_queues(void)
{
	vkGetDeviceQueue(this->device, 0, 0, &this->queue);
}

void FirstVulkan::vulkan_check_surface_support(void)
{
	VkBool32 surface_support = false;
	VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[0], 0, this->surface, &surface_support);
	ASSERT_VULKAN(result);

	if (!surface_support)
	{
		std::cerr << "Surface not supported!" << std::endl;
		asm("int $3");
	}
}

void FirstVulkan::vulkan_create_swapchain(void)
{
	// create swapchain info
	VkSwapchainCreateInfoKHR swap_chain_info = {};
	swap_chain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swap_chain_info.pNext = nullptr;
	swap_chain_info.flags = 0;
	swap_chain_info.surface = this->surface;
	swap_chain_info.minImageCount = 3;				// TODO: check if valid
	swap_chain_info.imageFormat = COLOR_FORMAT;		// TODO: check if valid
	swap_chain_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; // TODO: check if valid
	swap_chain_info.imageExtent = { width, height };
	swap_chain_info.imageArrayLayers = 1;
	swap_chain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swap_chain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swap_chain_info.queueFamilyIndexCount = 0;
	swap_chain_info.pQueueFamilyIndices = nullptr;
	swap_chain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // image can also be rotated
	swap_chain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swap_chain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR; // TODO: check if valid
	swap_chain_info.clipped = VK_TRUE;
	swap_chain_info.oldSwapchain = this->swapchain; // is needed if swap chain is modified, e.g. when the window resizes

	// chreate actual swapchain
	VkResult result = vkCreateSwapchainKHR(this->device, &swap_chain_info, nullptr, &this->swapchain);
	ASSERT_VULKAN(result);
}

void FirstVulkan::vulkan_create_image_views(void)
{
	// retrieve images for drawing from swapchain
	vkGetSwapchainImagesKHR(this->device, this->swapchain, &this->n_images_swapchain, nullptr);
	VkImage images_swapchain[n_images_swapchain];
	VkResult result = vkGetSwapchainImagesKHR(this->device, this->swapchain, &this->n_images_swapchain, images_swapchain);
	ASSERT_VULKAN(result);

	// create image view for every image of the swapchain
	this->image_views = new VkImageView[this->n_images_swapchain];
	for (int i = 0; i < this->n_images_swapchain; i++)
	{
		// create image view info 
		VkImageViewCreateInfo image_view_info = {};
		image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_info.pNext = nullptr;
		image_view_info.flags = 0;
		image_view_info.image = images_swapchain[i];
		image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_info.format = COLOR_FORMAT; // TODO: check if valid
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
}

void FirstVulkan::vulkan_create_render_pass(void)
{
	// attachment description for framebuffer
	VkAttachmentDescription attachment_description = {};
	attachment_description.flags = 0;
	attachment_description.format = COLOR_FORMAT;
	attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;		// layout before render pass
	attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;	// layout after render pass

	// attachment reference for attachment description
	VkAttachmentReference attachment_reference = {};
	attachment_reference.attachment = 0;
	attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // layout during render pass

	// sub pass equals one single draw call
	VkSubpassDescription subpass_description = {};
	subpass_description.flags = 0;
	subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_description.inputAttachmentCount = 0;		// shader inputs
	subpass_description.pInputAttachments = nullptr;
	subpass_description.colorAttachmentCount = 1;
	subpass_description.pColorAttachments = &attachment_reference;
	subpass_description.pResolveAttachments = nullptr;
	subpass_description.pDepthStencilAttachment = nullptr;
	subpass_description.preserveAttachmentCount = 0;
	subpass_description.pPreserveAttachments = nullptr;

	// subpass dependency to make sure that the image transformation happens after the color output
	VkSubpassDependency subpass_dependency = {};
	subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;	// interal subpass
	subpass_dependency.dstSubpass = 0;						// our subpass, depends on the internal pass to finish!
	subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	// transformation happens after the color output
	subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.srcAccessMask = 0;					// destination (output image) must be read-write
	subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpass_dependency.dependencyFlags = 0;

	// info for render pass
	VkRenderPassCreateInfo renderpass_info = {};
	renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderpass_info.pNext = nullptr;
	renderpass_info.flags = 0;
	renderpass_info.attachmentCount = 1;
	renderpass_info.pAttachments = &attachment_description;
	renderpass_info.subpassCount = 1;
	renderpass_info.pSubpasses = &subpass_description;
	renderpass_info.dependencyCount = 1;
	renderpass_info.pDependencies = &subpass_dependency;

	// create final render pass -> can countain multiple sub passes (draw calls)
	VkResult result = vkCreateRenderPass(this->device, &renderpass_info, nullptr, &this->renderpass);
	ASSERT_VULKAN(result);
}

void FirstVulkan::vulkan_create_shader_modules(void)
{
	/* Creating shaders can be improved and more automatized. */
	std::vector<char> shadercode_main_vert, shadercode_main_frag;
	// read shader code
	this->read_shader("../../../shader/spir-v/main_vert.spv", shadercode_main_vert);
	this->read_shader("../../../shader/spir-v/main_frag.spv", shadercode_main_frag);

	// create shader modules for every shader
	VkResult result = this->create_shader_moudle(shadercode_main_vert, &this->shadermodule_main_vert);
	ASSERT_VULKAN(result);
	this->create_shader_moudle(shadercode_main_frag, &this->shadermodule_main_frag);
	ASSERT_VULKAN(result);
}

void FirstVulkan::vulkan_create_descriptor_set_layout(void)
{
	VkDescriptorSetLayoutBinding descr_set_binding = {};
	descr_set_binding.binding = 0;
	descr_set_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descr_set_binding.descriptorCount = 1;
	descr_set_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descr_set_binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo descr_set_info = {};
	descr_set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descr_set_info.pNext = nullptr;
	descr_set_info.flags = 0;
	descr_set_info.bindingCount = 1;
	descr_set_info.pBindings = &descr_set_binding;
	
	VkResult result = vkCreateDescriptorSetLayout(this->device, &descr_set_info, nullptr, &this->descriptor_set_layout);
	ASSERT_VULKAN(result);
}

void FirstVulkan::vulkan_create_pipeline(void)
{
	// create shader stage infos
	VkPipelineShaderStageCreateInfo shader_stage_main_vert = {};
	shader_stage_main_vert.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_main_vert.pNext = nullptr;
	shader_stage_main_vert.flags = 0;
	shader_stage_main_vert.stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_stage_main_vert.module = this->shadermodule_main_vert;
	shader_stage_main_vert.pName = "main";					// main function of shader
	shader_stage_main_vert.pSpecializationInfo = nullptr;	// can be used to optimize constants and expressions where the constants are used

	VkPipelineShaderStageCreateInfo shader_stage_main_frag = {};
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

	// get binding descriptions
	VkVertexInputBindingDescription vertex_binding_descr = {};
	vertex_t::get_binding_description(vertex_binding_descr);

	// get attribute descriptions, equals glVertexAttribPointer
	std::vector<VkVertexInputAttributeDescription> vertex_attrib_descr;
	vertex_t::get_attrib_descriptions(vertex_attrib_descr);

	// create vertex shader input info
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.pNext = nullptr;
	vertex_input_info.flags = 0;
	vertex_input_info.vertexBindingDescriptionCount = 1;		// i think thats the equivalent to glBindBuffer
	vertex_input_info.pVertexBindingDescriptions = &vertex_binding_descr;
	vertex_input_info.vertexAttributeDescriptionCount = vertex_attrib_descr.size();
	vertex_input_info.pVertexAttributeDescriptions = vertex_attrib_descr.data();

	// create input assembly info
	VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
	input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_info.pNext = nullptr;
	input_assembly_info.flags = 0;
	input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // equivalent: GL_TRIANGLES 
	input_assembly_info.primitiveRestartEnable = VK_FALSE;

	// setup viewport similar to glViewPort
	VkViewport view_port;
	view_port.x = 0.0f;
	view_port.y = 0.0f;
	view_port.width = width;
	view_port.height = height;
	view_port.minDepth = 0.0f;
	view_port.maxDepth = 1.0f;

	// defines what sould be shown at the screen, everything else will be cut off -> name scissor
	VkRect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = { width, height };

	// create viewport state info
	VkPipelineViewportStateCreateInfo viewport_state_info = {};
	viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_info.pNext = nullptr;
	viewport_state_info.flags = 0;
	viewport_state_info.viewportCount = 1;
	viewport_state_info.pViewports = &view_port;
	viewport_state_info.scissorCount = 1;
	viewport_state_info.pScissors = &scissor;

	// create rasterizer state info
	VkPipelineRasterizationStateCreateInfo rasterizer_info = {};
	rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer_info.pNext = nullptr;
	rasterizer_info.flags = 0;
	rasterizer_info.depthClampEnable = VK_FALSE;
	rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
	rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;		// back face culling
	rasterizer_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// in OpenGL counter clockwise
	rasterizer_info.depthBiasEnable = VK_FALSE;
	rasterizer_info.depthBiasConstantFactor = 0;
	rasterizer_info.depthBiasClamp = 0.0f;
	rasterizer_info.depthBiasSlopeFactor = 0.0f;
	rasterizer_info.lineWidth = 1.0f;

	// create multisample info -> antialiasing
	VkPipelineMultisampleStateCreateInfo multisample_state_info = {};
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
	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.blendEnable = VK_TRUE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.colorWriteMask = 0x0000000F; // enable every color channel (RGBA) 

	VkPipelineColorBlendStateCreateInfo color_blend_info = {};
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

	std::vector<VkDynamicState> dynamic_pipeline_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_state_info;
	dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_info.pNext = nullptr;
	dynamic_state_info.flags = 0;
	dynamic_state_info.dynamicStateCount = dynamic_pipeline_states.size();
	dynamic_state_info.pDynamicStates = dynamic_pipeline_states.data();

	// create pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.pNext = nullptr;
	pipeline_layout_info.flags = 0;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &this->descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = nullptr;

	VkResult result = vkCreatePipelineLayout(this->device, &pipeline_layout_info, nullptr, &this->pipeline_layout);
	ASSERT_VULKAN(result);

	// setup graphics pipeline
	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.pNext = nullptr;
	pipeline_info.flags = 0;	// flags can allow inheritance 
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_stages_main;
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly_info;
	pipeline_info.pTessellationState = nullptr;
	pipeline_info.pViewportState = &viewport_state_info;
	pipeline_info.pRasterizationState = &rasterizer_info;
	pipeline_info.pMultisampleState = &multisample_state_info;
	pipeline_info.pDepthStencilState = nullptr;
	pipeline_info.pColorBlendState = &color_blend_info;
	pipeline_info.pDynamicState = &dynamic_state_info;	// can enable or disable certain things dynamically like in OpenGL (glEnable, glDisable)
	pipeline_info.layout = this->pipeline_layout;
	pipeline_info.renderPass = this->renderpass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;	// used for inheritance
	pipeline_info.basePipelineIndex = -1;				// invalid index

	// finally, create pipeline
	result = vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &this->pipeline);
	ASSERT_VULKAN(result);
}

void FirstVulkan::vulkan_create_framebuffers(void)
{
	this->fbos_swapchain = new VkFramebuffer[n_images_swapchain];
	for (size_t i = 0; i < n_images_swapchain; i++)
	{
		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.pNext = nullptr;
		framebuffer_info.flags = 0;
		framebuffer_info.renderPass = this->renderpass;
		framebuffer_info.attachmentCount = 1;
		framebuffer_info.pAttachments = this->image_views + i;
		framebuffer_info.width = width;
		framebuffer_info.height = height;
		framebuffer_info.layers = 1;

		VkResult result = vkCreateFramebuffer(this->device, &framebuffer_info, nullptr, this->fbos_swapchain + i);
		ASSERT_VULKAN(result);
	}
}

void FirstVulkan::vulkan_create_command_pool(void)
{
	// info for command pool
	VkCommandPoolCreateInfo cmd_pool_info = {};
	cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmd_pool_info.pNext = nullptr;
	cmd_pool_info.flags = 0;			// can allow to update command buffers on per buffer basis, or can allow to record the command buffers every iteration (OpenGL)
	cmd_pool_info.queueFamilyIndex = 0;	// TODO: check if valid, family MUST have graphics bit anebled

	// create command pool
	VkResult result = vkCreateCommandPool(this->device, &cmd_pool_info, nullptr, &this->cmd_pool);
	ASSERT_VULKAN(result);
}

void FirstVulkan::vulkan_create_command_buffers(void)
{
	// allocation info for command buffer(s)
	VkCommandBufferAllocateInfo cmd_buffer_alloc_info = {};
	cmd_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_buffer_alloc_info.pNext = nullptr;
	cmd_buffer_alloc_info.commandPool = this->cmd_pool;
	cmd_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_buffer_alloc_info.commandBufferCount = this->n_images_swapchain;	// needed for every image in the swapchain 

	// create final command buffers
	this->cmd_buffers = new VkCommandBuffer[this->n_images_swapchain];
	VkResult result = vkAllocateCommandBuffers(this->device, &cmd_buffer_alloc_info, this->cmd_buffers);
	ASSERT_VULKAN(result);
}

void FirstVulkan::vulkan_create_semaphores(void)
{
	// create info for semaphores
	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_info.pNext = nullptr;
	semaphore_info.flags = 0;

	// semaphores for rendering
	VkResult result = vkCreateSemaphore(this->device, &semaphore_info, nullptr, &this->semaphore_img_aviable);
	ASSERT_VULKAN(result);
	result = vkCreateSemaphore(this->device, &semaphore_info, nullptr, &this->semaphore_rendering_done);
	ASSERT_VULKAN(result);
}

void FirstVulkan::vulkan_recrate_swapchain(void)
{
	/* Recreation of swapchain and everything that is connected to it
	   is needed for resizeable windows. */

	VkResult result = vkDeviceWaitIdle(this->device);	// wait until device has finished the last work
	ASSERT_VULKAN(result);

	// destroy the old objects...
	vkFreeCommandBuffers(this->device, this->cmd_pool, this->n_images_swapchain, this->cmd_buffers);	// command buffer depends on the number of images in the swapchain
	delete[] this->cmd_buffers;
	for (size_t i = 0; i < this->n_images_swapchain; i++)
		vkDestroyFramebuffer(this->device, this->fbos_swapchain[i], nullptr);	// framebuffer depends on the window size
	delete[] this->fbos_swapchain;
	for (size_t i = 0; i < this->n_images_swapchain; i++)
		vkDestroyImageView(this->device, this->image_views[i], nullptr);		// image view depends on the window size
	delete[] this->image_views;

	// ...and create them new
	VkSwapchainKHR old_swapchain = this->swapchain;	// Save old swapchain because VkSwapchainCreateInfoKHR must inherit from the old_swapchain in order to create the new one.

	this->vulkan_create_swapchain();				// Old swapchain is saved in this->swapchain and then gets overwritten. New swapchain interits from the old swapchain.
	this->vulkan_create_image_views();
	this->vulkan_create_framebuffers();				// recreate framebuffers
	this->vulkan_create_command_buffers();			// recreate command buffers
	this->vulkan_record_command_buffers();			// command buffers must also recorded new

	vkDestroySwapchainKHR(this->device, old_swapchain, nullptr);	// Delete old swapchain, in this->swapchain is saved the new swapchain.
}

uint32_t FirstVulkan::vulkan_find_mem_type_index(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties device_mem_properties = {};
	vkGetPhysicalDeviceMemoryProperties(this->physical_devices[0], &device_mem_properties); // TODO:: check if valid

	for (uint32_t i = 0; i < device_mem_properties.memoryTypeCount; i++)
	{
		// get bit at i
		if ((type_filter & (1 << i)) && (device_mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}

	throw std::runtime_error("Found no correct memory type!");
}

void FirstVulkan::vulkan_create_buffer(VkDeviceSize size, VkBufferUsageFlags usage_flags, VkBuffer& buffer, VkMemoryPropertyFlags mem_flags, VkDeviceMemory& device_mem)
{
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.pNext = nullptr;
	buffer_info.flags = 0;
	buffer_info.size = size;
	buffer_info.usage = usage_flags;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_info.queueFamilyIndexCount = 0;
	buffer_info.pQueueFamilyIndices = nullptr;

	VkResult result = vkCreateBuffer(this->device, &buffer_info, nullptr, &buffer);
	ASSERT_VULKAN(result);

	VkMemoryRequirements mem_requierements = {};
	vkGetBufferMemoryRequirements(this->device, buffer, &mem_requierements);

	VkMemoryAllocateInfo mem_alloc_info = {};
	mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc_info.pNext = nullptr;
	mem_alloc_info.allocationSize = mem_requierements.size;
	mem_alloc_info.memoryTypeIndex = this->vulkan_find_mem_type_index(mem_requierements.memoryTypeBits, mem_flags);

	// allocate actual memory for buffer
	result = vkAllocateMemory(this->device, &mem_alloc_info, nullptr, &device_mem);
	ASSERT_VULKAN(result);

	// connect memory with our vertex buffer
	vkBindBufferMemory(this->device, buffer, device_mem, 0);
}

void FirstVulkan::vulkan_copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
	// command buffer for copy operation
	VkCommandBufferAllocateInfo cmd_buffer_info = {};
	cmd_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_buffer_info.pNext = nullptr;
	cmd_buffer_info.commandPool = this->cmd_pool;
	cmd_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_buffer_info.commandBufferCount = 1;

	VkCommandBuffer cmd_buffer = {};
	VkResult result = vkAllocateCommandBuffers(this->device, &cmd_buffer_info, &cmd_buffer);
	ASSERT_VULKAN(result);

	// begin info for copy operation
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = nullptr;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;	// one-time usage of the command buffer
	begin_info.pInheritanceInfo = nullptr;

	// begin recording copy operation
	result = vkBeginCommandBuffer(cmd_buffer, &begin_info);
	ASSERT_VULKAN(result);

	// copy information
	VkBufferCopy buffer_copy = {};
	buffer_copy.srcOffset = 0;
	buffer_copy.dstOffset = 0;
	buffer_copy.size = size;
	
	vkCmdCopyBuffer(cmd_buffer, src, dst, 1, &buffer_copy);

	result = vkEndCommandBuffer(cmd_buffer);
	ASSERT_VULKAN(result);

	// submit copy command (actually the command buffer) to the command queue
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = 0;
	submit_info.pWaitSemaphores = nullptr;
	submit_info.pWaitDstStageMask = nullptr;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buffer;
	submit_info.signalSemaphoreCount = 0;
	submit_info.pSignalSemaphores = nullptr;

	result = vkQueueSubmit(this->queue, 1, &submit_info, VK_NULL_HANDLE);
	ASSERT_VULKAN(result);

	// wait for command to be submitted
	vkQueueWaitIdle(queue);

	// free command buffer
	vkFreeCommandBuffers(this->device, this->cmd_pool, 1, &cmd_buffer);
}

void FirstVulkan::vulkan_create_vertex_buffer(void)
{
	// this can be done more optimized with creating and uploading multiple buffers at once
	this->create_and_upload_buffer<vertex_t>(this->vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, this->vertex_buffer, this->vertex_buffer_memory);
	this->create_and_upload_buffer<uint32_t>(this->indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, this->index_buffer, this->index_buffer_memory);
}

void FirstVulkan::vulkan_create_uniform_buffer(void)
{
	VkDeviceSize buff_size = sizeof(this->MVP);
	this->vulkan_create_buffer(buff_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, this->uniform_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, this->uniform_buffer_memory);
}

void FirstVulkan::vulkan_create_descriptor_pool(void)
{
	VkDescriptorPoolSize descr_pool_size = {};
	descr_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descr_pool_size.descriptorCount = 1;

	VkDescriptorPoolCreateInfo descr_pool_info = {};
	descr_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descr_pool_info.pNext = nullptr;
	descr_pool_info.flags = 0;
	descr_pool_info.maxSets = 1;
	descr_pool_info.poolSizeCount = 1;
	descr_pool_info.pPoolSizes = &descr_pool_size;

	VkResult result = vkCreateDescriptorPool(this->device, &descr_pool_info, nullptr, &this->descriptor_pool);
	ASSERT_VULKAN(result);
}

void FirstVulkan::vulkan_create_descriptor_set(void)
{
	VkDescriptorSetAllocateInfo descr_set_alloc_info = {};
	descr_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descr_set_alloc_info.pNext = nullptr;
	descr_set_alloc_info.descriptorPool = this->descriptor_pool;
	descr_set_alloc_info.descriptorSetCount = 1;
	descr_set_alloc_info.pSetLayouts = &this->descriptor_set_layout;

	VkResult result = vkAllocateDescriptorSets(this->device, &descr_set_alloc_info, &this->descriptor_set);
	ASSERT_VULKAN(result);

	VkDescriptorBufferInfo descr_buffer_info = {};
	descr_buffer_info.buffer = this->uniform_buffer;
	descr_buffer_info.offset = 0;
	descr_buffer_info.range = sizeof(MVP);

	VkWriteDescriptorSet write_descr_set = {};
	write_descr_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descr_set.pNext = nullptr;
	write_descr_set.dstSet = this->descriptor_set;
	write_descr_set.dstBinding = 0;
	write_descr_set.dstArrayElement = 0;
	write_descr_set.descriptorCount = 1;
	write_descr_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_descr_set.pImageInfo = nullptr;
	write_descr_set.pBufferInfo = &descr_buffer_info;
	write_descr_set.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(this->device, 1, &write_descr_set, 0, nullptr);
}

void FirstVulkan::vulkan_record_command_buffers(void)
{
	// specifies how commands should be recorded (loaded into the buffer)
	VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
	cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_buffer_begin_info.pNext = nullptr;
	cmd_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // allowes to have the same command multiple times in a queue
	cmd_buffer_begin_info.pInheritanceInfo = nullptr; // used for secondary command buffers

	// start recording command buffers for every image in the swapchain or for every command buffer respectively
	for (size_t i = 0; i < this->n_images_swapchain; i++)
	{
		VkResult result = vkBeginCommandBuffer(this->cmd_buffers[i], &cmd_buffer_begin_info);
		ASSERT_VULKAN(result);

		// specifies the begin of the render pass
		VkRenderPassBeginInfo render_pass_begin_info = {};
		render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin_info.pNext = nullptr;
		render_pass_begin_info.renderPass = this->renderpass;
		render_pass_begin_info.framebuffer = this->fbos_swapchain[i];
		render_pass_begin_info.renderArea.offset = { 0, 0 };			// render full screen (framebuffer) 
		render_pass_begin_info.renderArea.extent = { width, height };
		VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };			// equivalent to glClearColor
		render_pass_begin_info.clearValueCount = 1;
		render_pass_begin_info.pClearValues = &clear_color;

		// start render pass												// we only use primary command buffers
		vkCmdBeginRenderPass(this->cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		// use our pipeline to render
		vkCmdBindPipeline(this->cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);

		VkViewport viewport;
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = this->width;
		viewport.height = this->height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(this->cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor;
		scissor.offset = { 0, 0 };
		scissor.extent = { this->width, this->height };
		vkCmdSetScissor(this->cmd_buffers[i], 0, 1, &scissor);

		VkDeviceSize offsets[] = { 0 };

		// actual draw command
		vkCmdBindVertexBuffers(this->cmd_buffers[i], 0, 1, &this->vertex_buffer, offsets);
		vkCmdBindIndexBuffer(this->cmd_buffers[i], this->index_buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(this->cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline_layout, 0, 1, &this->descriptor_set, 0, nullptr);

		//vkCmdDraw(this->cmd_buffers[i], this->vertices.size(), 1, 0, 0)
		vkCmdDrawIndexed(this->cmd_buffers[i], this->indices.size(), 1, 0, 0, 0);

		vkCmdEndRenderPass(this->cmd_buffers[i]);

		result = vkEndCommandBuffer(this->cmd_buffers[i]);
		ASSERT_VULKAN(result);
	}
}

void FirstVulkan::vulkan_init(void)
{
	this->vulkan_create_app_info();
	this->vulkan_create_instance();
	this->vulkan_create_glfw_window_surface();
	this->vulkan_create_device();
	this->vulkan_create_queues();
	this->vulkan_check_surface_support();
	this->vulkan_create_swapchain();
	this->vulkan_create_image_views();
	this->vulkan_create_render_pass();
	this->vulkan_create_shader_modules();
	this->vulkan_create_descriptor_set_layout();
	this->vulkan_create_pipeline();
	this->vulkan_create_framebuffers();
	this->vulkan_create_command_pool();
	this->vulkan_create_vertex_buffer();
	this->vulkan_create_uniform_buffer();
	this->vulkan_create_descriptor_pool();
	this->vulkan_create_descriptor_set();	
	this->vulkan_create_command_buffers();
	this->vulkan_record_command_buffers();
	this->vulkan_create_semaphores();
}

void FirstVulkan::glfw_on_window_resize(GLFWwindow* window, int width, int height)
{
	VkSurfaceCapabilitiesKHR surface_capabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->physical_devices[0], this->surface, &surface_capabilities);

	// prevent image from getting bigger as the GPU can render
	if (width > surface_capabilities.maxImageExtent.width)		width = surface_capabilities.maxImageExtent.width;	
	if (height > surface_capabilities.maxImageExtent.height)	height = surface_capabilities.maxImageExtent.height;
	if (width == 0 || height == 0)								return;	// image dimensions must not be 0

	// update image dimensions
	this->width = width;
	this->height = height;

	// recreate swapchain and everything that is connected to it
	this->vulkan_recrate_swapchain();
}

void FirstVulkan::glfw_init(void)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);	// tell glfw that we use vulkan
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	this->window = glfwCreateWindow(width, height, "First Vulkan", nullptr, nullptr);
}

void FirstVulkan::vulkan_destroy(void)
{
	vkDeviceWaitIdle(this->device);

	vkDestroyDescriptorSetLayout(this->device, this->descriptor_set_layout, nullptr);
	vkDestroyDescriptorPool(this->device, this->descriptor_pool, nullptr);
	vkFreeMemory(this->device, this->uniform_buffer_memory, nullptr);
	vkDestroyBuffer(this->device, this->uniform_buffer, nullptr);

	vkFreeMemory(this->device, this->vertex_buffer_memory, nullptr);
	vkDestroyBuffer(this->device, this->vertex_buffer, nullptr);
	vkFreeMemory(this->device, this->index_buffer_memory, nullptr);
	vkDestroyBuffer(this->device, this->index_buffer, nullptr);

	vkDestroySemaphore(this->device, this->semaphore_img_aviable, nullptr);
	vkDestroySemaphore(this->device, this->semaphore_rendering_done, nullptr);

	vkFreeCommandBuffers(this->device, this->cmd_pool, this->n_images_swapchain, this->cmd_buffers);
	delete[] this->cmd_buffers;

	vkDestroyCommandPool(this->device, this->cmd_pool, nullptr);

	for (size_t i = 0; i < this->n_images_swapchain; i++)
		vkDestroyFramebuffer(this->device, this->fbos_swapchain[i], nullptr);
	delete[] this->fbos_swapchain;

	vkDestroyPipeline(this->device, this->pipeline, nullptr);

	vkDestroyRenderPass(this->device, this->renderpass, nullptr);

	vkDestroyPipelineLayout(this->device, this->pipeline_layout, nullptr);

	vkDestroyShaderModule(this->device, this->shadermodule_main_vert, nullptr);
	vkDestroyShaderModule(this->device, this->shadermodule_main_frag, nullptr);

	for (size_t i = 0; i < this->n_images_swapchain; i++)
		vkDestroyImageView(this->device, this->image_views[i], nullptr);
	delete[] this->image_views;

	vkDestroySwapchainKHR(this->device, this->swapchain, nullptr);

	vkDestroyDevice(this->device, nullptr);
	delete[] this->physical_devices;

	vkDestroySurfaceKHR(this->instance, this->surface, nullptr);

	vkDestroyInstance(this->instance, nullptr);

}

void FirstVulkan::glfw_destroy(void)
{
	glfwDestroyWindow(this->window);
	glfwTerminate();
}

void FirstVulkan::update_mvp(void)
{
	const double t_app_cur = glfwGetTime();
	const double deltatime = (t_app_cur - this->t_app_start);

	glm::mat4 model(1.0f);
	model = glm::rotate(model, static_cast<float>(deltatime) * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 view = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)this->width / (float)this->height, 0.01f, 100.0f);
	projection[1][1] *= -1.0f;	// invert screen y axis

	this->MVP = projection * view * model;

	void* data;
	vkMapMemory(this->device, this->uniform_buffer_memory, 0, sizeof(this->MVP), 0, &data);
	memcpy(data, &this->MVP, sizeof(MVP));
	vkUnmapMemory(this->device, this->uniform_buffer_memory);
}

void FirstVulkan::draw_frame(void)
{
	int w, h;
	glfwGetWindowSize(this->window, &w, &h);
	if (w != this->width || h != this->height)
		this->glfw_on_window_resize(this->window, w, h);

	// get next image for rendering
	uint32_t image_index;																		// 1) first step: get image
	vkAcquireNextImageKHR(this->device, this->swapchain, std::numeric_limits<uint64_t>::max(), this->semaphore_img_aviable, VK_NULL_HANDLE, &image_index);

	// start rendering process
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &this->semaphore_img_aviable;			// 2) wait until next image is aviable
	VkPipelineStageFlags wait_stage_mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };	// wait for image at the color blending step (last step in the pipeline)
	submit_info.pWaitDstStageMask = wait_stage_mask;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = this->cmd_buffers + image_index;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &this->semaphore_rendering_done;	// 3) next setep: rendering

	VkResult result = vkQueueSubmit(this->queue, 1, &submit_info, VK_NULL_HANDLE);
	ASSERT_VULKAN(result);

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = nullptr;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &this->semaphore_rendering_done;		// 4) wait until rendering has finished
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &this->swapchain;
	present_info.pImageIndices = &image_index;
	present_info.pResults = nullptr;

	result = vkQueuePresentKHR(this->queue, &present_info);
	ASSERT_VULKAN(result);
}

void FirstVulkan::run(void)
{
	while (!glfwGetKey(this->window, GLFW_KEY_ESCAPE) && !glfwWindowShouldClose(this->window))
	{
		double t_begin = glfwGetTime();
		glfwPollEvents();

		this->update_mvp();
		this->draw_frame();

		double t_end = glfwGetTime();
		std::cout.precision(3);
		std::cout << "Framerate: " << 1.0 / ((t_end - t_begin)) << "FPS\r";
	}
}

void FirstVulkan::print_deviceinfo(const VkPhysicalDevice* devices, size_t n)
{
	/* READ MATCHING DEVICE IN AN REAL APPLICATION */
	std::cout << std::endl;
	std::cout << "Amount of physical devices: " << n << std::endl;
	for (size_t i = 0; i < n; i++)
	{
		std::cout << "--------------- DEVICE " << i << " ---------------" << std::endl;
		// read properties of physical device
		VkPhysicalDeviceProperties device_properties = {};
		vkGetPhysicalDeviceProperties(devices[i], &device_properties);
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
		VkPhysicalDeviceFeatures device_features = {};
		vkGetPhysicalDeviceFeatures(devices[i], &device_features);
		std::cout << "Geometry shader support:   " << ((device_features.geometryShader) ? "YES" : "NO") << std::endl;

		// read memory properties pf physical device (just to demonstrate how it works)
		VkPhysicalDeviceMemoryProperties device_memprop;
		vkGetPhysicalDeviceMemoryProperties(devices[i], &device_memprop);

		/* READ BEST SUITED QUEUE FAMILIES FOR A REAL APPLICATION*/
		// read queue families
		uint32_t n_queue_families;
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &n_queue_families, nullptr);
		VkQueueFamilyProperties family_prop[n_queue_families];
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &n_queue_families, family_prop);

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
		VkSurfaceCapabilitiesKHR surface_capabilities = {};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[i], this->surface, &surface_capabilities);
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
		vkGetPhysicalDeviceSurfaceFormatsKHR(devices[i], this->surface, &n_formats, nullptr);
		VkSurfaceFormatKHR surface_formats[n_formats];
		vkGetPhysicalDeviceSurfaceFormatsKHR(devices[i], this->surface, &n_formats, surface_formats);

		std::cout << std::endl;
		std::cout << "Amount of normats: " << n_formats << std::endl;
		for (int i = 0; i < n_formats; i++)
		{
			std::cout << "Format: " << surface_formats[i].format << std::endl;
		}

		/* ALSO THE PRESENTATION MODE SHOULD BE CHECKED FOR YOUR REQUIREMENTS */
		// read presetation modes
		uint32_t n_presentation_modes = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(devices[i], this->surface, &n_presentation_modes, nullptr);
		VkPresentModeKHR presentation_modes[n_presentation_modes];
		vkGetPhysicalDeviceSurfacePresentModesKHR(devices[i], this->surface, &n_presentation_modes, presentation_modes);

		std::cout << std::endl;
		std::cout << "Amount of presentation modes: " << n_presentation_modes << std::endl;
		for (int i = 0; i < n_presentation_modes; i++)
		{
			std::cout << "Presentation mode: " << presentation_modes[i] << std::endl;
		}
		std::cout << "----------------------------------------" << std::endl << std::endl;
	}
}

void FirstVulkan::print_instance_layers(const VkLayerProperties* layers, size_t n)
{
	std::cout << "Amount of instance layers at instance level: " << n << std::endl;
	for (int i = 0; i < n; i++)
	{
		uint32_t v_spec = layers[i].specVersion;
		uint32_t v_implementatopm = layers[i].implementationVersion;
		std::cout << std::endl;
		std::cout << "Layer name:             " << layers[i].layerName << std::endl;
		std::cout << "Specification version:  " << VK_VERSION_MAJOR(v_spec) << "." << VK_VERSION_MINOR(v_spec) << "." << VK_VERSION_PATCH(v_spec) << std::endl;
		std::cout << "Implementation version: " << VK_VERSION_MAJOR(v_implementatopm) << "." << VK_VERSION_MINOR(v_implementatopm) << "." << VK_VERSION_PATCH(v_implementatopm) << std::endl;
		std::cout << "Description:            " << layers[i].description << std::endl;
	}
	std::cout << "------------------------------------------------------" << std::endl;
}

void FirstVulkan::print_instance_extensions(const VkExtensionProperties* extensions, size_t n)
{
	std::cout << "Amount of extensions at instance level: " << n << std::endl;
	for (int i = 0; i < n; i++)
	{
		uint32_t v_spec = extensions[i].specVersion;
		std::cout << std::endl;
		std::cout << "Extension name:         " << extensions[i].extensionName << std::endl;
		std::cout << "Specification version:  " << VK_VERSION_MAJOR(v_spec) << "." << VK_VERSION_MINOR(v_spec) << "." << VK_VERSION_PATCH(v_spec) << std::endl;
	}
	std::cout << "------------------------------------------------------" << std::endl;
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