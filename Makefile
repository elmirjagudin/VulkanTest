VULKAN_SDK_PATH = /home/boris/vulkan/VulkanSDK/1.0.57.0/x86_64
CFLAGS = -std=c++11 -Wall -I$(VULKAN_SDK_PATH)/include -I/home/boris/vulkan/libs/usr/local/include
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib -L/home/boris/vulkan/libs/usr/local/lib -lvulkan -lglfw3 -lrt -lm -ldl -lXrandr -lXinerama -lXi -lXcursor -lXrender -lGL -lm -lpthread -ldl -ldrm -lXdamage -lXfixes -lX11-xcb -lxcb-glx -lxcb-dri2 -lXxf86vm -lXext -lX11 -lpthread -lxcb -lXau -lXdmcp -Wl,-R$(VULKAN_SDK_PATH)/lib


prog: main.cpp dump.cpp utils.cpp
	g++ $(CFLAGS) -o prog main.cpp dump.cpp utils.cpp $(LDFLAGS)
