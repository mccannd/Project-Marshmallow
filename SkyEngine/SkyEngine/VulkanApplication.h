#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <iostream>
#include <vector>
#include <cstring>
#include <set>
#include <algorithm>
#include <fstream>
#include <array>

#define DEBUG_VALIDATION 1

#define WORKGROUP_SIZE 32

struct QueueFamilyIndices {
    int graphicsFamily = -1; // capable of graphics pipeline?
    int computeFamily = -1; // capable of compute pipeline? TODO not sure if this is done
    int presentFamily = -1; // capable of presenting image to screen surface?

    bool isComplete() {
        return graphicsFamily >= 0 && presentFamily >= 0;
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// TODO: Move to a mesh class
struct Vertex {
    glm::vec2 pos;
    glm::vec3 col;

    // rate to load data from memory throughout vertices
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    // Set layout bindings for vertex struct to vertex shader
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, col);

        return attributeDescriptions;
    }

};

const std::vector<Vertex> vertices = {
    { { 0.0f, -0.5f },{ 1.0f, 0.0f, 0.0f } },
    { { 0.5f, 0.5f },{ 0.0f, 1.0f, 0.0f } },
    { { -0.5f, 0.5f },{ 0.0f, 0.0f, 1.0f } }
};


// TODO: Move this ASAP
// Read an entire file (bytecode)
static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

class VulkanApplication
{
private:
    /// --- Baseline Application Functions
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

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
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();

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
    VkPipelineLayout graphicsPipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    // Compute pipeline
    VkPipelineLayout computePipelineLayout;
    VkPipeline computePipeline;
    VkCommandBuffer computeCommandBuffer; // TODO: if we need multiple compute shaders for some reason,
                                          // make this a vector like above
    VkCommandPool computeCommandPool;

    // TODO: move to mesh class
    void createVertexBuffer();
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;


#if DEBUG_VALIDATION
    // enable a range of validation layers through the SDK
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_LUNARG_standard_validation"
    };
#endif

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

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

