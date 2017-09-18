#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <vulkan/vulkan.h>

#include "utils.h"

void
bail_out(const char *msg)
{
	printf("error: %s\n", msg);
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

std::vector<char>
read_file(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}
