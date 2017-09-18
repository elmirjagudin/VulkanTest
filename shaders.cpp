#include "shaders.h"

#include "utils.h"

VkShaderModule
load_shader(handles_t *handles, const std::string& filename)
{
    auto shaderCode = read_file(filename);

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    VkShaderModule shaderModule;

    check_res(
        vkCreateShaderModule(handles->device, &createInfo, nullptr, &shaderModule),
        "error vkCreateShaderModule");

    return shaderModule;
}
