
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <vector>

#include "main.h"
#include "utils.h"
#include "dump.h"

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
	static char *layers[1] = { "VK_LAYER_LUNARG_standard_validation" };
	return layers;
}

static const char * const*
get_device_extensions(uint32_t *count)
{
	*count = 1;
	static char *extensions[1] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
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

	///*
	// * check surface caps
	// */
	//VkSurfaceCapabilitiesKHR pSurfaceCapabilities;
	//check_res(
	//	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device,
	//		handles->surface,
	//		&pSurfaceCapabilities),
	//	"vkGetPhysicalDeviceSurfaceCapabilitiesKHR error");

	/*
	 * surface formats
	 */
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, handles->surface, &count, NULL);
	if (count < 1)
	{
		printf("no surface formats supported\n");
		return false;
	}
	std::vector<VkSurfaceFormatKHR> formats(count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, handles->surface, &count, formats.data());
	
	/*
	 * presentation modes
	 */
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, handles->surface, &count, NULL);
	if (count < 1)
	{
		printf("no presentation modes supported\n");
		return false;
	}
	//std::vector<VkPresentModeKHR> presentationModes(count);
	//vkGetPhysicalDeviceSurfacePresentModesKHR(device, handles->surface, &count, presentationModes.data());

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

	bail_out("No descrete GPU found");
}

static void
get_queue_families(handles_t *handles,
	               VkPhysicalDevice device,
	               uint32_t *gfxFamilyIndex,
	               uint32_t *presentationFamilyIndex)
{
	uint32_t count;
	int32_t gfxIdx = -1;
	int32_t presIdx = -1;

	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, NULL);
	std::vector<VkQueueFamilyProperties> qFamilies(count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, qFamilies.data());

	for (int32_t i = 0; i < qFamilies.size(); i += 1)
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
				device,
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
init_device(handles_t *handles)
{
	VkPhysicalDevice dev;
	get_phy_device(handles, &dev);

	/*
	 * allocate one graphics capable queue
	 */

	uint32_t gfxFamilyIndex;
	uint32_t presentationFamilyIndex;

	get_queue_families(handles, dev, &gfxFamilyIndex, &presentationFamilyIndex);
	printf("gfx queue %d, pres queue %d\n", gfxFamilyIndex, presentationFamilyIndex);

	if (gfxFamilyIndex != presentationFamilyIndex)
	{
		/*
		 * to lazy to impement this now, as nvidia GTX 1060 have same queue
		 * family for  both graphics and presentation
		 */
		bail_out("different queue familties for graphics and presentation not supported");
	}

	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = gfxFamilyIndex;
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

	VkDevice ldev;
	check_res(vkCreateDevice(dev, &createInfo, NULL, &ldev), "vkCreateDevice error");

	vkGetDeviceQueue(ldev, gfxFamilyIndex, 0, &(handles->gfxQueue));
	/* we are cheating here as we know that gfx and presentation queue are the same */
	handles->presentationQueue = handles->gfxQueue;
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

	dump_gfx_cards(handles);

	/*
	 * init device
	 */
	init_device(handles);
}

static void
cleanup_vulkan(handles_t *handles)
{
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
	}

	cleanup_vulkan(&handles);
	cleanup_gui(&handles);

	return 0;
}
