#include <vulkan/vulkan.h>
#include <stdio.h>

#include "utils.h"
#include <vector>

void
dump_layers()
{
	uint32_t count;
	VkLayerProperties layers[16];

	check_res(vkEnumerateInstanceLayerProperties(&count, NULL),
		"VkEnumerateInstanceLayerProperties error");

	printf("%d layers\n", count);

	count = 16;
	check_res(vkEnumerateInstanceLayerProperties(&count, layers),
		"VkEnumerateInstanceLayerProperties error");

	for (uint32_t i = 0; i < count; i++)
	{
		printf("layer %d '%s'\n", i, layers[i].layerName);
	}

}

void
dump_extensions()
{
	VkExtensionProperties ext[16];
	uint32_t ext_count;

	check_res(vkEnumerateInstanceExtensionProperties(
		NULL, &ext_count, NULL),
		"vkEnumerateInstanceExtensionProperties get count error");

	printf("%d extensions\n", ext_count);

	ext_count = 16;
	check_res(vkEnumerateInstanceExtensionProperties(
		NULL, &ext_count, ext),
		"vkEnumerateInstanceExtensionProperties get extensions");

	for (uint32_t i = 0; i < ext_count; i++)
	{
		printf("ext %d '%s'\n", i, ext[i].extensionName);
	}
}

static char *
yes_no(VkBool32 b)
{
	return b ? "yes" : "no";
}

static void dump_queues(VkPhysicalDevice device)
{
	uint32_t count;

	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, NULL);
	printf("Has %d queue familys\n", count);

	std::vector<VkQueueFamilyProperties> qFamilies(count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, qFamilies.data());

	for (auto qf = qFamilies.begin(); qf != qFamilies.end(); ++qf)
	{
		printf("Graphics %s Compute %s Transfer %s\n",
			   yes_no(qf->queueFlags & VK_QUEUE_GRAPHICS_BIT),
			   yes_no(qf->queueFlags & VK_QUEUE_COMPUTE_BIT),
			   yes_no(qf->queueFlags & VK_QUEUE_TRANSFER_BIT));
		printf("queue count %d\n", qf->queueCount);
	}
}

void 
dump_gfx_cards(VkInstance instance)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);

	printf("I see %d device%s\n", deviceCount, deviceCount > 1 ? "s" : "");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (auto it = devices.begin(); it != devices.end(); ++it)
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(*it, &props);

		VkPhysicalDeviceFeatures feat;
		vkGetPhysicalDeviceFeatures(*it, &feat);

		printf("name %s, vulkan ver %d.%d.%d, device type %d\n",
			   props.deviceName,
			   VK_VERSION_MAJOR(props.apiVersion),
			   VK_VERSION_MINOR(props.apiVersion),
			   VK_VERSION_PATCH(props.apiVersion),
			   props.deviceType);

		printf("multiViewport %s geometryShader %s\n",
			   yes_no(feat.multiViewport),
			   yes_no(feat.geometryShader));

		dump_queues(*it);

		printf("--------\n");
	}
	
}

