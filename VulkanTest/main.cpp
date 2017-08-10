#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <vector>

#include "utils.h"
#include "dump.h"

typedef struct handles_s
{
	GLFWwindow* window;
	VkInstance instance;
	VkDebugReportCallbackEXT debug_cb;
	VkSurfaceKHR surface;
} handles_t;


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

static void
get_phy_device(VkInstance instance, VkPhysicalDevice *device)
{
	uint32_t deviceCount = 0;

	vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	VkPhysicalDeviceProperties props;
	for (auto dev = devices.begin(); dev != devices.end(); ++dev)
	{
		vkGetPhysicalDeviceProperties(*dev, &props);
		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			printf("Using %s for rendering\n", props.deviceName);
			*device = *dev;
			return;
		}
	}

	bail_out("No descrete GPU found");
}

static void
get_gfx_queue_family(VkPhysicalDevice device, uint32_t *queueFamilyIndex)
{
	uint32_t count;

	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, NULL);
	std::vector<VkQueueFamilyProperties> qFamilies(count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, qFamilies.data());

	for (std::size_t i = 0; i < qFamilies.size(); i += 1)
	{
		auto props = qFamilies[i];
		if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			*queueFamilyIndex = (uint32_t)i;
			return;
		}
	}

	bail_out("No queue supporting graphics operations found.");
}

static void
init_device(VkInstance instance)
{
	VkPhysicalDevice dev;
	get_phy_device(instance, &dev);

	/*
	 * allocate one graphics capable queue
	 */
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	get_gfx_queue_family(dev, &queueCreateInfo.queueFamilyIndex);
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
	createInfo.enabledExtensionCount = 0;
	createInfo.ppEnabledLayerNames = get_layers(&createInfo.enabledLayerCount);

	VkDevice ldev;
	check_res(vkCreateDevice(dev, &createInfo, NULL, &ldev), "vkCreateDevice error");

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

	//dump_gfx_cards(*instance);

	/*
	 * init window surface
	 */
	check_res(
		glfwCreateWindowSurface(handles->instance,
								handles->window,
								NULL,
								&(handles->surface)),
		"error creating window surface");

	/*
	 * init device
	 */
	init_device(handles->instance);
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
