
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string.h>

#include <iostream>
#include <chrono>

#include "main.h"
#include "utils.h"
#include "dump.h"
#include "gfx_pipeline.h"
#include "frame_buf.h"
#include "cmd_buf.h"
#include "gpu_buf.h"

const std::vector<Vertex> vertices =
{
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f},   {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f},  {1.0f, 1.0f, 1.0f}},
};

const std::vector<uint16_t> indices =
{
    0, 1, 2, 2, 3, 0
};

static void
init_gui(handles_t *handles)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	handles->window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
}

static void
cleanup_gui(handles_t *handles)
{
	glfwDestroyWindow(handles->window);
	glfwTerminate();
}

/*
 * Get the vulkan extensions we need to enable.
 */
static std::vector<const char*>
get_vulkan_extensions()
{
	std::vector<const char*> exts;

	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++)
	{
		exts.push_back(glfwExtensions[i]);
	}

	/* for consuming validation layers output */
	exts.push_back("VK_EXT_debug_report");

	return exts;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData) {

	std::cerr << "validation layer: " << msg << std::endl;

	return VK_FALSE;
}

static const char * const*
get_layers(uint32_t *count)
{
	*count = 1;
	static const char *layers[1] = { "VK_LAYER_LUNARG_standard_validation" };
	return layers;
}

static const char * const*
get_device_extensions(uint32_t *count)
{
	*count = 1;
	static const char *extensions[1] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	return extensions;
}


static bool
is_device_suitable(handles_t *handles, VkPhysicalDevice device)
{
	uint32_t count;

	/*
	 * must be discrete device
	 */
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(device, &props);
	if (props.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		return false;
	}

	/*
	 * must support VK_KHR_swapchain extension
	 */
	bool supportSwapchain = false;
	vkEnumerateDeviceExtensionProperties(device, NULL, &count, NULL);
	std::vector<VkExtensionProperties> extensions(count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());

	for (auto ext = extensions.begin(); ext != extensions.end(); ++ext)
	{
		if (strncmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, ext->extensionName, strlen(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) == 0)
		{
			supportSwapchain = true;
			break;
		}
	}

	if (!supportSwapchain)
	{
		return false;
	}

	/*
     * check that VK_FORMAT_B8G8R8A8_UNORM surface format is
     * supported
	 */
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, handles->surface, &count, NULL);
	if (count < 1)
	{
		printf("no surface formats supported\n");
		return false;
	}
    std::vector<VkSurfaceFormatKHR> formats(count);

    bool VK_FORMAT_B8G8R8A8_UNORM_supported = false;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, handles->surface, &count, formats.data());
    for (auto format = formats.begin(); format != formats.end(); ++format)
	{
        if (VK_FORMAT_B8G8R8A8_UNORM == format->format)
        {
            VK_FORMAT_B8G8R8A8_UNORM_supported = true;
            break;
        }
    }

    if (!VK_FORMAT_B8G8R8A8_UNORM_supported)
    {
        printf("VK_FORMAT_B8G8R8A8_UNORM format not supported\n");
        return false;
    }

	/*
	 * presentation modes
	 */
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, handles->surface, &count, NULL);
	if (count < 1)
	{
		printf("no presentation modes supported\n");
		return false;
	}
	std::vector<VkPresentModeKHR> presentationModes(count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, handles->surface, &count, presentationModes.data());

	printf("Using %s for rendering\n", props.deviceName);
	return true;

}

static void
get_phy_device(handles_t *handles, VkPhysicalDevice *device)
{
	uint32_t deviceCount = 0;

	vkEnumeratePhysicalDevices(handles->instance, &deviceCount, NULL);
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(handles->instance, &deviceCount, devices.data());

	for (auto dev = devices.begin(); dev != devices.end(); ++dev)
	{
		if (is_device_suitable(handles, *dev))
		{
			*device = *dev;
			return;
		}
	}

	bail_out("No suitable GPU found");
}

static void
get_queue_families(handles_t *handles,
	               uint32_t *gfxFamilyIndex,
	               uint32_t *presentationFamilyIndex)
{
	uint32_t count;
	int32_t gfxIdx = -1;
	int32_t presIdx = -1;

	vkGetPhysicalDeviceQueueFamilyProperties(handles->phyDevice, &count, NULL);
	std::vector<VkQueueFamilyProperties> qFamilies(count);
	vkGetPhysicalDeviceQueueFamilyProperties(handles->phyDevice, &count, qFamilies.data());

	for (uint32_t i = 0; i < qFamilies.size(); i += 1)
	{
		auto props = qFamilies[i];

		if (gfxIdx == -1 && props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			gfxIdx = i;
		}

		if (presIdx == -1)
		{
			VkBool32 pSupported;
			check_res(vkGetPhysicalDeviceSurfaceSupportKHR(
				handles->phyDevice,
				i,
				handles->surface,
				&pSupported),
				"vkGetPhysicalDeviceSurfaceSupportKHR error");
			presIdx = i;
		}
	}

	if (gfxIdx == -1)
	{
		bail_out("no graphics queue familty found");
	}

	if (presIdx == -1)
	{
		bail_out("no presentation queue familty found");
	}

	*gfxFamilyIndex = gfxIdx;
	*presentationFamilyIndex = presIdx;
}

static void
init_swapchain(handles_t *handles)
{
    /*
     * get current image extend
     */
	VkSurfaceCapabilitiesKHR pSurfaceCapabilities;
	check_res(
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(handles->phyDevice,
			handles->surface,
			&pSurfaceCapabilities),
		"vkGetPhysicalDeviceSurfaceCapabilitiesKHR error");


    /*
     * create swapchain
     */
    VkSwapchainCreateInfoKHR createInfo = {};

    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = handles->surface;
    createInfo.minImageCount = 1;
    createInfo.imageFormat = FRAME_BUF_FORMAT;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = pSurfaceCapabilities.currentExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    /* we don't support the case when graphics and presentation queues are different */
    assert(handles->gfxQueue == handles->presentationQueue);
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0; // Optional
    createInfo.pQueueFamilyIndices = nullptr; // Optional
    createInfo.preTransform = pSurfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    check_res(
        vkCreateSwapchainKHR(handles->device,
                             &createInfo,
                             NULL,
                             &(handles->swapchain)),
        "vkCreateSwapchainKHR error");

    uint32_t count;

    vkGetSwapchainImagesKHR(handles->device, handles->swapchain, &count, nullptr);
    handles->swapChainImages.resize(count);
    vkGetSwapchainImagesKHR(handles->device, handles->swapchain,
                            &count, handles->swapChainImages.data());

    handles->swapChainImageViews.resize(count);
    for (size_t i = 0; i < handles->swapChainImages.size(); i++)
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = handles->swapChainImages[i];

        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = FRAME_BUF_FORMAT;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        check_res(
            vkCreateImageView(handles->device,
                              &viewInfo,
                              NULL,
                              &(handles->swapChainImageViews[i])),
            "vkCreateImageView error");
    }

    handles->swapchainExtend = pSurfaceCapabilities.currentExtent;
}

static void
init_device(handles_t *handles)
{
	get_phy_device(handles, &(handles->phyDevice));

	/*
	 * allocate one graphics and presentation capable queue
	 */

    get_queue_families(handles,
                       &(handles->gfxFamilyIndex),
                       &(handles->presentationFamilyIndex));
    printf("gfx queue %d, pres queue %d\n",
           handles->gfxFamilyIndex,
            handles->presentationFamilyIndex);

	if (handles->gfxFamilyIndex != handles->presentationFamilyIndex)
	{
		/*
		 * to lazy to impement this now, as nvidia GTX 1060 have same queue
		 * family for both graphics and presentation
		 */
		bail_out("different queue familties for graphics and presentation not supported");
	}

	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = handles->gfxFamilyIndex;
	queueCreateInfo.queueCount = 1;
	float queuePriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	/*
	 * no special features
	 */
	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.ppEnabledExtensionNames = get_device_extensions(&createInfo.enabledExtensionCount);
	createInfo.ppEnabledLayerNames = get_layers(&createInfo.enabledLayerCount);

	check_res(
        vkCreateDevice(handles->phyDevice, &createInfo, NULL, &(handles->device)),
        "vkCreateDevice error");

	vkGetDeviceQueue(handles->device, handles->gfxFamilyIndex, 0, &(handles->gfxQueue));
	/* we are cheating here as we know that gfx and presentation queue are the same */
	handles->presentationQueue = handles->gfxQueue;
}

static void
create_semaphores(handles_t *handles)
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    check_res(
        vkCreateSemaphore(handles->device, &semaphoreInfo,
                          NULL, &(handles->imageAvailableSemaphore)),
        "vkCreateSemaphore imageAvailableSemaphore");

    check_res(
        vkCreateSemaphore(handles->device, &semaphoreInfo,
                            NULL, &(handles->renderFinishedSemaphore)),
        "vkCreateSemaphore renderFinishedSemaphore");

}

static void
init_vulkan(handles_t *handles)
{
	/*
	 * create Vulkan Instance
	 */
	VkInstanceCreateInfo info = {};

	info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	info.pNext = NULL;
	info.flags = 0;
	info.pApplicationInfo = NULL;

	info.ppEnabledLayerNames = get_layers(&info.enabledLayerCount);

	auto extensions = get_vulkan_extensions();
	info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	info.ppEnabledExtensionNames = extensions.data();

	VkResult res = vkCreateInstance(&info, NULL, &(handles->instance));
	check_res(res, "vkCreateInstance error");

	/*
	 * create debug callback handle
	 */
	VkDebugReportCallbackCreateInfoEXT dbgCallbackInfo = {};
	dbgCallbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	dbgCallbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	dbgCallbackInfo.pfnCallback = debugCallback;

	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
		   handles->instance,
           "vkCreateDebugReportCallbackEXT");

	func(handles->instance, &dbgCallbackInfo, NULL, &(handles->debug_cb));

	/*
	 * init window surface
	 */
	check_res(
		glfwCreateWindowSurface(handles->instance,
								handles->window,
								NULL,
								&(handles->surface)),
		"error creating window surface");

	//dump_gfx_cards(handles);

	/*
	 * init device, swapchain, graphics pipeline
	 */
    init_device(handles);
    init_swapchain(handles);
    create_descriptor_set_layout(handles);
    create_gfk_pipeline(handles);
    create_framebuffers(handles);
    create_command_pool(handles);
    create_vertex_buffer(handles, vertices);
    create_index_buffer(handles, indices);
    create_uniform_buffer(handles);
    create_descriptor_pool(handles);
    create_descriptor_set(handles);
    create_command_buffers(handles, static_cast<uint32_t>(indices.size()));
    create_semaphores(handles);
}

static void
cleanup_vulkan(handles_t *handles)
{
    printf("cleanup...\n");

    /* destroy semaphores */
    vkDestroySemaphore(handles->device, handles->imageAvailableSemaphore, NULL);
    vkDestroySemaphore(handles->device, handles->renderFinishedSemaphore, NULL);

    /* destroy command pool */
    vkDestroyCommandPool(handles->device, handles->commandPool, NULL);

    /* destroy frame buffers */
    cleanup_framebuffers(handles);

    /* destroy render pass */
    vkDestroyRenderPass(handles->device, handles->renderPass, NULL);

    /* destroy pipeline */
    vkDestroyPipelineLayout(handles->device, handles->pipelineLayout, NULL);

    /* destroy descriptor set layout */
    vkDestroyDescriptorSetLayout(handles->device, handles->descriptorSetLayout, NULL);

    /* destroy swapchain */
    vkDestroySwapchainKHR(handles->device, handles->swapchain, NULL);

    /* destroy descriptor pool */
    vkDestroyDescriptorPool(handles->device, handles->descriptorPool, NULL);

    /* destroy index buffer */
    vkDestroyBuffer(handles->device, handles->indexBuffer, NULL);
    vkFreeMemory(handles->device, handles->indexBufferMemory, NULL);

    /* destroy vertix buffer */
    vkDestroyBuffer(handles->device, handles->vertexBuffer, NULL);
    vkFreeMemory(handles->device, handles->vertexBufferMemory, NULL);

    /* destroy uniform buffer */
    vkDestroyBuffer(handles->device, handles->uniformBuffer, NULL);
    vkFreeMemory(handles->device, handles->uniformBufferMemory, NULL);

	/* destroy debug callback handle */
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
		handles->instance,
		"vkDestroyDebugReportCallbackEXT");
	func(handles->instance, handles->debug_cb, NULL);

	/* destroy window surface */
	vkDestroySurfaceKHR(handles->instance, handles->surface, NULL);

	/* destroy vulkan instance */
	vkDestroyInstance(handles->instance, NULL);
}

static void
update_uniform_buffer(handles_t *handles)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

    UniformBufferObject ubo = {};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(
        glm::radians(45.0f),
        handles->swapchainExtend.width / (float) handles->swapchainExtend.height,
        0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    void* data;
    vkMapMemory(handles->device, handles->uniformBufferMemory, 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(handles->device, handles->uniformBufferMemory);
}

static void
draw_frame(handles_t *handles)
{
    vkQueueWaitIdle(handles->presentationQueue);

    uint32_t imageIndex;

    /* acquire image */
    check_res(
        vkAcquireNextImageKHR(handles->device,
                              handles->swapchain,
                              std::numeric_limits<uint64_t>::max(),
                              handles->imageAvailableSemaphore, VK_NULL_HANDLE,
                              &imageIndex),
        "vkAcquireNextImageKHR");

    /* sumbit command buffer */
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {handles->imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &(handles->commandBuffers[imageIndex]);
    VkSemaphore signalSemaphores[] = {handles->renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    check_res(
        vkQueueSubmit(handles->gfxQueue, 1, &submitInfo, VK_NULL_HANDLE),
        "vkQueueSubmit");


    /* present */

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {handles->swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(handles->presentationQueue, &presentInfo);
}

int
main()
{
	//dump_extensions();
    //dump_layers();

    handles_t handles;

	init_gui(&handles);
	init_vulkan(&handles);

	while (!glfwWindowShouldClose(handles.window)) {
        glfwPollEvents();
        update_uniform_buffer(&handles);
        draw_frame(&handles);
    }

    vkDeviceWaitIdle(handles.device);

	cleanup_vulkan(&handles);
	cleanup_gui(&handles);

	return 0;
}
