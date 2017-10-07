VULKAN_SDK_PATH = /home/boris/vulkan/VulkanSDK/1.0.57.0/x86_64
CFLAGS = -std=c++11 -Wall -I$(VULKAN_SDK_PATH)/include -I/home/boris/vulkan/libs/usr/local/include
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib -L/home/boris/vulkan/libs/usr/local/lib -lvulkan -lglfw3 -lrt -lm -ldl -lXrandr -lXinerama -lXi -lXcursor -lXrender -lGL -lm -lpthread -ldl -ldrm -lXdamage -lXfixes -lX11-xcb -lxcb-glx -lxcb-dri2 -lXxf86vm -lXext -lX11 -lpthread -lxcb -lXau -lXdmcp -Wl,-R$(VULKAN_SDK_PATH)/lib

# shader compiler
SHADER_C = $(VULKAN_SDK_PATH)/bin/glslangValidator -V

SRC = main.cpp dump.cpp utils.cpp gfx_pipeline.cpp shaders.cpp frame_buf.cpp cmd_buf.cpp gpu_buf.cpp
FILES = prog frag.spv vert.spv

prog: $(SRC) frag.spv vert.spv
	g++ -g $(CFLAGS) -o prog $(SRC) $(LDFLAGS)

frag.spv: shader.frag
	$(SHADER_C) shader.frag

vert.spv: shader.vert
	$(SHADER_C) shader.vert


clean:
	rm -rf $(FILES)

