/*

    arfminesweeper: Cross-plataform multi-frontend game
    Copyright (C) 2023 arf20 (Ángel Ruiz Fernandez)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    vk.c: Vulkan/GLFW frontend

*/

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <common/frontconf.h>
#include "vk.h"
#include <common/game.h>

static const int *board = NULL;
static int size = 0;

static int wWidth, wHeight;
static GLFWwindow* window;

static VkInstance instance;
static VkPhysicalDevice physicalDevice;
static VkDevice device;
static VkSurfaceKHR surface;
static VkSwapchainKHR swapChain;

static uint32_t swapChainImageCount;
static VkImage* swapChainImages;
static VkImageView *swapChainImageViews;

static VkShaderModule vertShaderModule;
static VkShaderModule fragShaderModule;

static VkDescriptorSetLayout descriptorSetLayout;
static VkPipelineLayout pipelineLayout;
static VkRenderPass renderPass;
static VkPipeline graphicsPipeline;

static VkFramebuffer *swapChainFramebuffers;
static VkCommandPool commandPool;
static VkCommandBuffer commandBuffer;

static VkSemaphore imageAvailableSemaphore;
static VkSemaphore renderFinishedSemaphore;
static VkFence inFlightFence;

static VkBuffer boardSSBO;
static VkDeviceMemory ssboMemory;
static void *boardMapped;

static VkDescriptorPool descriptorPool;
static VkDescriptorSet descriptorSet;

#define VALIDATION_LAYER "VK_LAYER_KHRONOS_validation"
static char *validationLayer = NULL;

#define EXTENSIONS VK_KHR_SWAPCHAIN_EXTENSION_NAME
static const char *requiredExtensions[] = { EXTENSIONS };
static size_t requiredExtCount = sizeof(requiredExtensions) / sizeof(requiredExtensions[0]);

#define FORMAT      VK_FORMAT_B8G8R8A8_SRGB
#define COLORSPACE  VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
#define PRESENTMODE VK_PRESENT_MODE_MAILBOX_KHR

static VkExtent2D swapChainExtent;
static VkSurfaceFormatKHR swapChainSurfaceFormat;
static VkPresentModeKHR presentMode;


/* SHADER THING: uniform struct */
typedef struct {
    float sSizeX, sSizeY;
    uint32_t size;
} PushConstantsData;

PushConstantsData pushConstantsData;


void
updateBoardSSBO() {
    memcpy(boardMapped, board, sizeof(int) * size * size);
}

static void
mouseCallback(GLFWwindow* window, int button, int action, int mods) {
    if (action != GLFW_RELEASE) return;

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    int ix = ((int)x - W_MARGIN) /
        (CELL_SIZE + CELL_MARGIN);
    int iy = ((int)y - HEADER_HEIGHT) /
        (CELL_SIZE + CELL_MARGIN);
    if (ix < 0 || ix >= size || iy < 0 || iy >= size) return;

    printf("mouse %d,%d\n", ix, iy);

    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT: {
            gameClearCell(ix, iy);
            updateBoardSSBO();
        } break;
        case GLFW_MOUSE_BUTTON_RIGHT: {
            gameFlagCell(ix, iy);
            updateBoardSSBO();
        }
    }
}



int
checkValidationLayerSupport() {
    /* get validation layers */
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    VkLayerProperties* availableLayers = malloc(sizeof(VkLayerProperties) * layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    /* search layer */
    printf("Available Layers:\n");
    int found = 0;
    for (int i = 0; i < layerCount; i++) {
        printf("\t%s\n", availableLayers[i].layerName);
	    if (strcmp(availableLayers[i].layerName, VALIDATION_LAYER) == 0) {
            found = 1;
	    }
    }

    validationLayer = malloc(strlen(VALIDATION_LAYER) + 1);
    strcpy(validationLayer, VALIDATION_LAYER);

    free(availableLayers);

    return found;
}


VkPhysicalDevice
findDGPU(VkPhysicalDevice *devices, int n) {
    for (int i = 0; i < n; i++) {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
        vkGetPhysicalDeviceFeatures(devices[i], &deviceFeatures);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
            && deviceFeatures.geometryShader)
        {
            return devices[i];
        }
    }
    return VK_NULL_HANDLE;
}

int
findGraphicQueueFamilies(VkPhysicalDevice device) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
    VkQueueFamilyProperties* queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    int graphicQueueFamilyIdx = -1;
    for (int i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicQueueFamilyIdx = i;
            break;
        }
    }

    free(queueFamilies);

    return graphicQueueFamilyIdx;
}

int
findPresentationQueueFamilies(VkPhysicalDevice device) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
    VkQueueFamilyProperties* queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    VkBool32 presentSupport = 0;
    int presentationQueueFamilyIdx = -1;
    for (int i = 0; i < queueFamilyCount; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);  
        if (presentSupport) {
            presentationQueueFamilyIdx = i;
            break;
        }
    }

    free(queueFamilies);

    return presentationQueueFamilyIdx;
}

VkPhysicalDevice
pickDevice() {
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);

    if (deviceCount == 0)
        printf("Failed to find GPUs with Vulkan support\n");

    VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    VkPhysicalDeviceProperties dp;
    printf("Available Physical Devices:\n");
    for (int i = 0; i < deviceCount; i++) {
        vkGetPhysicalDeviceProperties(devices[i], &dp);
        printf("\t%s\n", dp.deviceName);
    }

    /* Find dGPU */
    int discrete = 1;
    physicalDevice = findDGPU(devices, deviceCount);
    if (physicalDevice == VK_NULL_HANDLE) {
        physicalDevice = devices[0];
        discrete = 0;
    }

    vkGetPhysicalDeviceProperties(physicalDevice, &dp);
    printf("Using ");
    discrete ? printf("d") : printf("i");
    printf("GPU %s\n", dp.deviceName);

    free(devices);

    return physicalDevice;
}

int
checkExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
    VkExtensionProperties* availableExtensions = malloc(sizeof(VkExtensionProperties) * extensionCount);
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions);
    
    /* Print extensions */
    printf("Available Extensions:\n");
    for (int i = 0; i < extensionCount; i++)
        printf("\t%s\n", availableExtensions[i].extensionName);
    
    
    for (int i = 0; i < requiredExtCount; i++) {
        int extFound = 0;
        for (int j = 0; j < extensionCount; j++) {
            if (strcmp(availableExtensions[j].extensionName, requiredExtensions[i]) == 0) {
                extFound = 1;
                break;
            }
        }
        if (!extFound) {
            printf("Required extension %s unavailable\n", requiredExtensions[i]);
            return -1;
        }
    }

    free(availableExtensions);

    return 0;
}

int
checkSwapChainSupport(VkPhysicalDevice device) {
    uint32_t formatCount, presentModeCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, NULL);

    VkSurfaceFormatKHR *formats = NULL;
    VkPresentModeKHR *presentModes = NULL;

    if (formatCount != 0) {
        formats = malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, formats);
    }

    if (presentModeCount != 0) {
        presentModes = malloc(sizeof(VkPresentModeKHR) * presentModeCount);;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, presentModes);
    }
    
    int formatAvailable = 0;
    for (int i = 0; i < formatCount; i++) {
        if (formats[i].format == FORMAT && formats[i].colorSpace == COLORSPACE) {
            formatAvailable = 1;
            swapChainSurfaceFormat = formats[i];
            break;
        }
    }

    int presentModesAvailable = 0;
    for (int i = 0; i < presentModeCount; i++) {
        if (presentModes[i] == PRESENTMODE) {
            presentModesAvailable = 1;
            presentMode = presentModes[i];
            break;
        }
    }
    // NOTE: VK_PRESENT_MODE_FIFO_KHR is guaranteed to be available
    
    if (formatAvailable == 0) {
        printf("Format unavailable for swap chain\n");
        return -1;
    }
    if (presentModeCount == 0) {
        printf("No presentation modes for swap chain\n");
        return -1;
    }

    free(formats);
    free(presentModes);

    return 0;
}

uint32_t
clamp(uint32_t d, uint32_t min, uint32_t max) {
  const uint32_t t = d < min ? min : d;
  return t > max ? max : t;
}

VkExtent2D
chooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            (uint32_t)width,
            (uint32_t)height
        };

        actualExtent.width  = clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

size_t
readFile(const char *path, char **buff) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) return -1;
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

    *buff = malloc(fsize + 1);
    fread(*buff, fsize, 1, f);

    fclose(f);
    (*buff)[fsize] = '\0';

    return fsize;
}


VkShaderModule
createShaderModule(const char *code, size_t size) {
    VkShaderModuleCreateInfo shaderModuleCreateInfo = { 0 };
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = size;
    shaderModuleCreateInfo.pCode = (uint32_t*)code;

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &shaderModuleCreateInfo, NULL, &shaderModule) != VK_SUCCESS) {
        printf("Failed to create shader module\n");
    }

    return shaderModule;
}

int
recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = { 0 };
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        printf("Failed to begin recording command buffer\n");
        return -1;
    }

    VkRenderPassBeginInfo renderPassInfo = { 0 };
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = (VkOffset2D){ .x = 0, .y = 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);


    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantsData), &pushConstantsData);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

    vkCmdDraw(commandBuffer, 4, 1, 0, 0); /* Draw command */

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        printf("Failed to record command buffer\n");
        return -1;
    }

    return 0;
}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    printf("Failed to find suitable memory type\n");
    return -1;
}

/* ===================== MAIN ===================== */

int
vkStart(const int *lboard, int lsize) {
    board = lboard;
    size = lsize;

    wWidth = (2 * W_MARGIN) + (size * CELL_SIZE) + ((size - 1) * CELL_MARGIN);
    wHeight = HEADER_HEIGHT + W_MARGIN + (size * CELL_SIZE) +
        ((size - 1) * CELL_MARGIN);

    pushConstantsData = (PushConstantsData){ .sSizeX = wWidth, .sSizeY = wHeight, .size = size };

    /* Init glfw */
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    /* Create window */
    if ((window = glfwCreateWindow(wWidth, wHeight, TXT_TITLE, NULL, NULL))
        == NULL)
    {
        printf("Error creating window\n");
        return -1;
    }

    glfwSetMouseButtonCallback(window, mouseCallback);

    /* ==================== GLFW & WINDOW INITIALISED ==================== */
    
    /* Create vulkan instance */
    VkApplicationInfo appInfo = { 0 };
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = TXT_TITLE;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Fuck Engines";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceCreateInfo = { 0 };
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    instanceCreateInfo.enabledExtensionCount = glfwExtensionCount;
    instanceCreateInfo.ppEnabledExtensionNames = glfwExtensions;


    if (!checkValidationLayerSupport()) {
        printf("Validation layer " VALIDATION_LAYER " unavailable\n");
        return 0;
    }

    instanceCreateInfo.enabledLayerCount = 1;
    instanceCreateInfo.ppEnabledLayerNames = (const char* const*)&validationLayer;

    VkResult result = vkCreateInstance(&instanceCreateInfo, NULL, &instance);
    if (result != VK_SUCCESS) {
        printf("Failed to create VK instance\n");
        return 0;
    }

    /* Create surface */
    if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
        printf("Failed to create window surface\n");
    }

    /* Pick physical device */
    physicalDevice = pickDevice();
    if (physicalDevice == VK_NULL_HANDLE) {
        return 0;
    }

    if (checkExtensionSupport(physicalDevice) < 0) {
        return 0;
    }

    if (checkSwapChainSupport(physicalDevice) < 0) {
        return 0;
    }

    /* Create Logical Device */
    VkPhysicalDeviceFeatures deviceFeatures = { 0 };

    VkDeviceCreateInfo deviceCreateInfo = { 0 };
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    deviceCreateInfo.enabledExtensionCount = requiredExtCount;
    deviceCreateInfo.ppEnabledExtensionNames = requiredExtensions;

    deviceCreateInfo.enabledLayerCount = 1;
    deviceCreateInfo.ppEnabledLayerNames = (const char* const*)&validationLayer;

    int graphicQueueFamilyIdx = findGraphicQueueFamilies(physicalDevice);
    if (graphicQueueFamilyIdx < 0) {
        printf("Graphic Queue Family not supported on this device\n");
        return 0;
    }

    int presentationQueueFamilyIdx = findPresentationQueueFamilies(physicalDevice);
    if (presentationQueueFamilyIdx < 0) {
        printf("Presentation Queue Family not supported on this device\n");
        return 0;
    }

    VkDeviceQueueCreateInfo* queueCreateInfos = malloc(sizeof(VkDeviceQueueCreateInfo) * 2);
    memset(queueCreateInfos, 0, sizeof(VkDeviceQueueCreateInfo) * 2);
    uint32_t queueFamilyIndices[] = { graphicQueueFamilyIdx, presentationQueueFamilyIdx };

    queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[0].queueFamilyIndex = queueFamilyIndices[0];
    queueCreateInfos[0].queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfos[0].pQueuePriorities = &queuePriority;

    queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[1].queueFamilyIndex = queueFamilyIndices[1];
    queueCreateInfos[1].queueCount = 1;
    queueCreateInfos[1].pQueuePriorities = &queuePriority;

    deviceCreateInfo.queueCreateInfoCount = 2;
    // HACK: if not unique lol
    if (queueFamilyIndices[0] == queueFamilyIndices[1])
        deviceCreateInfo.queueCreateInfoCount = 1;

    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;


    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device) != VK_SUCCESS) {
        printf("Failed to create logical device\n");
        return 0;
    }

    free(queueCreateInfos);

    /* Get Graphics Queue Handler */
    VkQueue graphicsQueue;
    vkGetDeviceQueue(device, graphicQueueFamilyIdx, 0, &graphicsQueue);
    VkQueue presentQueue;
    vkGetDeviceQueue(device, presentationQueueFamilyIdx, 0, &presentQueue);

    /* Create swap chain */
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

    swapChainExtent = chooseSwapExtent(capabilities); /* Resolution */

    swapChainImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && swapChainImageCount > capabilities.maxImageCount) {
        swapChainImageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo = { 0 };
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = surface;
    swapChainCreateInfo.minImageCount = swapChainImageCount;
    swapChainCreateInfo.imageFormat = swapChainSurfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = swapChainSurfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent = swapChainExtent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (graphicQueueFamilyIdx != presentationQueueFamilyIdx) {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;  // Optional
        swapChainCreateInfo.pQueueFamilyIndices = NULL; // Optional
    }

    swapChainCreateInfo.preTransform = capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.clipped = VK_TRUE;
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &swapChainCreateInfo, NULL, &swapChain) != VK_SUCCESS) {
        printf("Failed to create swap chain\n");
        return 0;
    }

    /* Get swap chain images */
    vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, NULL);
    swapChainImages = malloc(sizeof(VkImage) * swapChainImageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, swapChainImages);


    /* Image views */
    swapChainImageViews = malloc(sizeof(VkImageView) * swapChainImageCount);

    for (size_t i = 0; i < swapChainImageCount; i++) {
        VkImageViewCreateInfo imageViewCreateInfo = { 0 };
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapChainImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = swapChainSurfaceFormat.format;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &imageViewCreateInfo, NULL, &swapChainImageViews[i]) != VK_SUCCESS) {
            printf("Failed to create image views\n");
            return 0;
        }
    }


    /* ==================== VULKAN INITIALISED ==================== */

    /* ==================== GRAPHICS PIPELINE  ==================== */
    
    char *vertShaderCode, *fragShaderCode;
    size_t vertShaderCodeSize = readFile("../assets/msboard4vs.spv", &vertShaderCode);
    if (vertShaderCodeSize < 0) {
        printf("Error reading vertex shader\n");
        return 0;
    }
    
    size_t fragShaderCodeSize = readFile("../assets/msboard4fs.spv", &fragShaderCode);
    if (fragShaderCodeSize < 0) {
        printf("Error reading frag shader\n");
        return 0;
    }
    
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, vertShaderCodeSize);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, fragShaderCodeSize);

    

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = { 0 };
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = { 0 };
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


    VkPipelineVertexInputStateCreateInfo vertexInputInfo = { 0 };
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = { 0 };
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = { 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = { 0 };
    scissor.offset.x = 0; scissor.offset.y = 0;
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState = { 0 };
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;


    VkPipelineRasterizationStateCreateInfo rasterizer = { 0 };
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    VkPipelineMultisampleStateCreateInfo multisampling = { 0 };
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = { 0 };
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = { 0 };
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPushConstantRange range = { 0 };
    range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    range.offset = 0;
    range.size = sizeof(PushConstantsData); /* SHADER THING: vec2 + uint = 2 * 4 + 4 = 12 bytes */

    VkDescriptorSetLayoutBinding ssboLayoutBinding = { 0 };
    ssboLayoutBinding.binding = 0;
    ssboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ssboLayoutBinding.descriptorCount = 1;
    ssboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


    VkDescriptorSetLayoutCreateInfo layoutInfo = { 0 };
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &ssboLayoutBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &descriptorSetLayout) != VK_SUCCESS) {
        printf("Failed to create descriptor set layout\n");
        return 0;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = { 0 };
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &range;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout) != VK_SUCCESS) {
        printf("Failed to create pipeline layout\n");
        return 0;
    }


    VkAttachmentDescription colorAttachment = { 0 };
    colorAttachment.format = swapChainSurfaceFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = { 0 };
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass = { 0 };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = { 0 };
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


    VkRenderPassCreateInfo renderPassInfo = { 0 };
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass) != VK_SUCCESS) {
        printf("failed to create render pass\n");
        return 0;
    }


    /* Create pipeline finally */
    VkGraphicsPipelineCreateInfo pipelineInfo = { 0 };
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline) != VK_SUCCESS) {
        printf("Failed to create graphics pipeline\n");
        return 0;
    }


    free(vertShaderCode);
    free(fragShaderCode);

    vkDestroyShaderModule(device, fragShaderModule, NULL);
    vkDestroyShaderModule(device, vertShaderModule, NULL);

    /* ==================== COMMAND POOL + BUFFER ==================== */

    swapChainFramebuffers = malloc(sizeof(VkFramebuffer) * swapChainImageCount);

    for (size_t i = 0; i < swapChainImageCount; i++) {
        VkImageView attachments[] = {
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = { 0 };
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, NULL, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            printf("Failed to create framebuffer\n");
            return 0;
        }
    }

    VkCommandPoolCreateInfo commanPoolInfo = { 0 };
    commanPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commanPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commanPoolInfo.queueFamilyIndex = graphicQueueFamilyIdx;

    if (vkCreateCommandPool(device, &commanPoolInfo, NULL, &commandPool) != VK_SUCCESS) {
        printf("Failed to create command pool\n");
        return 0;
    }

    VkCommandBufferAllocateInfo allocInfo = { 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        printf("Failed to allocate command buffers\n");
        return 0;
    }

    /* ==================== SYNC OBJECTS ==================== */

    VkSemaphoreCreateInfo semaphoreInfo = { 0 };
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = { 0 };
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; /* prevent vkWaitForFences to block indefinitely in the first frame */

    if (vkCreateSemaphore(device, &semaphoreInfo, NULL, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, NULL, &renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, NULL, &inFlightFence) != VK_SUCCESS) {
        printf("Failed to create semaphores\n");
        return 0;
    }
   
    /* ==================== BOARD SSBO ==================== */

    VkBufferCreateInfo bufferInfo = { 0 };
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.size = sizeof(int) * size * size;

    if (vkCreateBuffer(device, &bufferInfo, NULL, &boardSSBO) != VK_SUCCESS) {
        printf("Failed to create vertex buffer\n");
        return 0;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, boardSSBO, &memRequirements);

    VkMemoryAllocateInfo ssboAllocInfo = { 0 };
    ssboAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ssboAllocInfo.allocationSize = memRequirements.size;
    ssboAllocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &ssboAllocInfo, NULL, &ssboMemory) != VK_SUCCESS) {
        printf("Failed to allocate board SSBO memory\n");
        return 0;
    }

    vkBindBufferMemory(device, boardSSBO, ssboMemory, 0);

    vkMapMemory(device, ssboMemory, 0, sizeof(int) * size * size, 0, &boardMapped);


    /* ==================== DESCRIPTOR POOL + SETS ==================== */

    VkDescriptorPoolSize poolSize = { 0 };
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = 1u;

    VkDescriptorPoolCreateInfo descriptorPoolInfo = { 0 };
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.poolSizeCount = 1;
    descriptorPoolInfo.pPoolSizes = &poolSize;
    descriptorPoolInfo.maxSets = 1u;

    if (vkCreateDescriptorPool(device, &descriptorPoolInfo, NULL, &descriptorPool) != VK_SUCCESS) {
        printf("Failed to create descriptor pool\n");
        return 0;
    }


    VkDescriptorSetAllocateInfo descriptorSetAllocInfo = { 0 };
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = descriptorPool;
    descriptorSetAllocInfo.descriptorSetCount = 1u;
    descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;

    if (vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSet) != VK_SUCCESS) {
        printf("Failed to allocate descriptor sets\n");
        return 0;
    }

    VkDescriptorBufferInfo descriptorBufferInfo = { 0 };
    descriptorBufferInfo.buffer = boardSSBO;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = sizeof(int) * size * size;

    VkWriteDescriptorSet descriptorWrite = { 0 };
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &descriptorBufferInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);


    /* ==================== MAIN LOOP ==================== */

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFence);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        vkResetCommandBuffer(commandBuffer, 0); /* make sure its able to be recorded */
        if (recordCommandBuffer(commandBuffer, imageIndex) < 0) {
            printf("Failed to record command buffer\n");
            return 0;
        }

        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSubmitInfo submitInfo = { 0 };
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
            printf("Failed to submit draw command buffer\n");
            return 0;
        }

        VkPresentInfoKHR presentInfo = { 0 };
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(presentQueue, &presentInfo);
    }

    vkDeviceWaitIdle(device);
}


void
vkDestroy() {
    vkDestroySemaphore(device, imageAvailableSemaphore, NULL);
    vkDestroySemaphore(device, renderFinishedSemaphore, NULL);
    vkDestroyFence(device, inFlightFence, NULL);

    vkDestroyCommandPool(device, commandPool, NULL);

    for (size_t i = 0; i < swapChainImageCount; i++) {
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], NULL);
    }

    vkDestroyPipeline(device, graphicsPipeline, NULL);
    vkDestroyPipelineLayout(device, pipelineLayout, NULL);
    vkDestroyRenderPass(device, renderPass, NULL);

    for (size_t i = 0; i < swapChainImageCount; i++) {
        vkDestroyImageView(device, swapChainImageViews[i], NULL);
    }
    free(swapChainImageViews);
    free(swapChainImages);

    vkDestroySwapchainKHR(device, swapChain, NULL);

    vkDestroyBuffer(device, boardSSBO, NULL);
    vkFreeMemory(device, ssboMemory, NULL);

    vkDestroyDescriptorPool(device, descriptorPool, NULL);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);

    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();
}

