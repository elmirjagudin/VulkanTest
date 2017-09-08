#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

void
bail_out(const char *msg)
{
	printf("error: %s", msg);
	fgetc(stdin);
	exit(EXIT_FAILURE);
}

void
check_res(VkResult res, const char *msg)
{
	if (res != VK_SUCCESS)
	{
		bail_out(msg);
	}

}
