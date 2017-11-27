#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif // !GLM_FORCE_RADIANS

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <iostream>
#include <vector>
#include <cstring>
#include <set>
#include <algorithm>
#include <fstream>
#include <array>
#include <chrono>

#include "camera.h"
#include "Texture.h"
#include "Geometry.h"
#include "Shader.h"


#define DEBUG_VALIDATION 1

#define WORKGROUP_SIZE 32

struct QueueFamilyIndices {
    int graphicsFamily = -1; // capable of graphics pipeline?
    int computeFamily = -1; // capable of compute pipeline? TODO not sure if this is done
    int presentFamily = -1; // capable of presenting image to screen surface?

    bool isComplete() {
        return graphicsFamily >= 0 && computeFamily >= 0 && presentFamily >= 0;
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class VulkanApplication
{
private:
    /// --- Baseline Application Functions
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    void updateUniformBuffer();

    GLFWwindow* window;

    const int WIDTH = 800;
    const int HEIGHT = 600;

    /// --- Vulkan Setup Functions
    void createInstance();
    void setupDebugCallback();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSurface();
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();

    /// --- Graphics Pipeline
    void createRenderPass(); // <------ ech
    //void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();

    void createBackgroundPipeline();

    // command buffer helpers
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    void createDescriptorSetLayout();
    void createUniformBuffer();
    void createDescriptorPool();
    void createDescriptorSet();

    /// --- Compute Pipeline
    void createComputePipeline();
    void createComputeCommandBuffer(); // TODO: rename this to be plural if we end up needing more compute shaders

    void drawFrame();
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    void createSemaphores();

    /// --- Swap Chain Setup Functions
    void createSwapChain();
    void createImageViews();
    void recreateSwapChain();
    void cleanupSwapChain();

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector <VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    VkImageView createImageView(VkImage image, VkFormat format);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
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

    static void onWindowResized(GLFWwindow* window, int width, int height) {
        if (width == 0 || height == 0) return;

        VulkanApplication* app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
        app->recreateSwapChain();
    }

    /// --- Vulkan Objects and Attributes
    VkInstance instance;
    VkDebugReportCallbackEXT callback;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkSurfaceKHR surface;

    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkQueue presentQueue;

    // these can likely be moved to their own class
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages; 
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    // for a graphics pipeline
    VkRenderPass renderPass;

    // TODO: abstract background
    VkDescriptorSetLayout backgroundSetLayout;
    VkDescriptorSet backgroundSet;

    // TODO: abstract compute
    VkDescriptorSetLayout computeSetLayout;
    VkDescriptorSet computeSet;

    /// background pipeline
    VkPipelineLayout backgroundPipelineLayout;

    VkPipeline backgroundPipeline;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    // Compute pipeline
    VkPipelineLayout computePipelineLayout;
    VkPipeline computePipeline;
    VkCommandBuffer computeCommandBuffer; // TODO: if we need multiple compute shaders for some reason,
                                          // make this a vector like above
    VkCommandPool computeCommandPool;

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    Geometry* sceneGeometry;
    Geometry* backgroundGeometry;
    void initializeGeometry();
    void cleanupGeometry();

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;

    VkDescriptorPool descriptorPool;
    VkDescriptorPool backgroundDescriptorPool;
    VkDescriptorPool computeDescriptorPool;

    // TODO: convenient way of managing textures
    void initializeTextures();
    void cleanupTextures();
    Texture* meshTexture;
    Texture* backgroundTexture;
    Texture* depthTexture;

    void initializeShaders();
    void cleanupShaders();
    MeshShader* meshShader;

#if _DEBUG
    // enable a range of validation layers through the SDK
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_LUNARG_standard_validation"
    };
#endif

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    /// --- Window interaction / functionality
    Camera mainCamera;
    void processInputs();
    float deltaTime;
    float prevTime;
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }
    VulkanApplication();
    ~VulkanApplication();
};

