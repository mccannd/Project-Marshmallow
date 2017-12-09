#include "VulkanApplication.h"

/// --- callback proxy functions
VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

/// empty for now
VulkanApplication::VulkanApplication()
{
}

VulkanApplication::~VulkanApplication()
{
}

void VulkanApplication::initWindow() {
    glfwInit();
        
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Sky Engine", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);
    glfwSetWindowSizeCallback(window, VulkanApplication::onWindowResized);

    mainCamera = Camera(glm::vec3(0.f, 1.f, 1.f), glm::vec3(0.f, 0.f, 0.f), 0.1f, 10.0f, 45.0f);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

}

void VulkanApplication::initVulkan() {
    createInstance();
#ifdef _DEBUG
    setupDebugCallback();
#endif
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain(); 
    createImageViews();

    createRenderPass();

    createCommandPool();
    
    initializeTextures();

    createFramebuffers(); // must come after so depth texture is initialized

    setupOffscreenPass();

    initializeGeometry();

    initializeShaders();

    createCommandBuffers();
    createPostProcessCommandBuffer();
    createComputeCommandBuffer();
    createSemaphores();

    mainCamera = Camera(glm::vec3(0.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.1f, 100.0f, 45.0f);
    mainCamera.setAspect((float) swapChainExtent.width, (float)swapChainExtent.height);
    skySystem = SkyManager();
}

void VulkanApplication::mainLoop() {
    static auto startTime = std::chrono::high_resolution_clock::now();
    prevTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count() / 1000.0f;
    deltaTime = prevTime;
    while (!glfwWindowShouldClose(window)) {

        float time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count() / 1000.0f;
        deltaTime = time - prevTime;

        glfwPollEvents();
        processInputs();
        updateUniformBuffer();
        drawFrame();

        prevTime = time;
    }

    vkDeviceWaitIdle(device);
}

void VulkanApplication::cleanup() {

    for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
    }
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        vkDestroyImageView(device, swapChainImageViews[i], nullptr);
    }
    cleanupOffscreenPass();
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyCommandPool(device, computeCommandPool, nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroySwapchainKHR(device, swapChain, nullptr);

    cleanupGeometry();

    cleanupTextures();
    cleanupShaders();

    vkDestroyDevice(device, nullptr);

#ifdef _DEBUG
    DestroyDebugReportCallbackEXT(instance, callback, nullptr);
#endif

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}

void VulkanApplication::processInputs() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        mainCamera.movePosition(Camera::FORWARD, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        mainCamera.movePosition(Camera::BACKWARD, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        mainCamera.movePosition(Camera::LEFT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        mainCamera.movePosition(Camera::RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        mainCamera.movePosition(Camera::UP, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        mainCamera.movePosition(Camera::DOWN, deltaTime);

    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);

    mainCamera.mouseRotate(xPos, yPos);
}

void VulkanApplication::drawFrame() {

    // submit compute queue
    // Compute queue submit
    VkSubmitInfo computeSubmitInfo = {};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    computeSubmitInfo.commandBufferCount = 1;
    computeSubmitInfo.pCommandBuffers = &computeCommandBuffer;

    if (vkQueueSubmit(computeQueue, 1, &computeSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit compute command buffer");
    }

    // acquire image from swap chain
    // execute corresponding command buffer
    // return the image to the swap chain, presentation mode
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    // must recreate swapchain -or- swap chain isn't working
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages; // what part of the pipeline is blocked by semaphore; vertex processing can still continue

    // Do all offscreen rendering
    submitInfo.pSignalSemaphores = { &offscreenPass.semaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &offscreenPass.commandBuffer;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit offscreen command buffer!");
    }

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };

    // Draw the scene onto the screen
    submitInfo.pWaitSemaphores = &offscreenPass.semaphore;
    submitInfo.pSignalSemaphores = signalSemaphores;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex]; // what is executed

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    vkQueuePresentKHR(presentQueue, &presentInfo); // present the image

    vkQueueWaitIdle(presentQueue); // sync to avoid mem leaks
}

void VulkanApplication::initializeTextures() {
    meshTexture = new Texture(device, physicalDevice, commandPool, graphicsQueue);
    meshTexture->initFromFile("Textures/tilesColor.png");
    backgroundTexture = new Texture(device, physicalDevice, commandPool, graphicsQueue, VK_FORMAT_R32G32B32A32_SFLOAT); // compute queue?
    backgroundTexture->initForStorage(swapChainExtent);
    depthTexture = new Texture(device, physicalDevice, commandPool, graphicsQueue);
    depthTexture->initForDepthAttachment(swapChainExtent);
    cloudPlacementTexture = new Texture(device, physicalDevice, commandPool, graphicsQueue);
    cloudPlacementTexture->initFromFile("Textures/CloudPlacement.png");
    lowResCloudShapeTexture3D = new Texture3D(device, physicalDevice, commandPool, graphicsQueue, 128, 128, 128); // 128, 128, 128
    lowResCloudShapeTexture3D->initFromFile("Textures/3DTextures/lowResCloudShape/lowResCloud"); // note: no .png
    hiResCloudShapeTexture3D = new Texture3D(device, physicalDevice, commandPool, graphicsQueue, 32, 32, 32); // 128, 128, 128
    hiResCloudShapeTexture3D->initFromFile("Textures/3DTextures/hiResCloudShape/hiResClouds "); // note: no .png
}

// TODO: management
void VulkanApplication::cleanupTextures() {
    delete meshTexture;
    delete backgroundTexture;
    delete depthTexture;
    delete cloudPlacementTexture;
    delete lowResCloudShapeTexture3D;
    delete hiResCloudShapeTexture3D;
}

void VulkanApplication::initializeGeometry() {
    sceneGeometry = new Geometry(device, physicalDevice, commandPool, graphicsQueue);
    //sceneGeometry->setupAsQuad();
    sceneGeometry->setupFromMesh("Models/DisplayCube.obj");
    backgroundGeometry = new Geometry(device, physicalDevice, commandPool, graphicsQueue);
    backgroundGeometry->setupAsBackgroundQuad();
}

void VulkanApplication::cleanupGeometry() {
    delete sceneGeometry;
    delete backgroundGeometry;
}

void VulkanApplication::initializeShaders() {
    meshShader = new MeshShader(device, physicalDevice, commandPool, graphicsQueue, swapChainExtent, 
        &offscreenPass.renderPass, std::string("Shaders/model.vert.spv"), std::string("Shaders/model.frag.spv"), meshTexture);
    
    backgroundShader = new BackgroundShader(device, physicalDevice, commandPool, graphicsQueue, swapChainExtent, 
        &offscreenPass.renderPass, std::string("Shaders/background.vert.spv"), std::string("Shaders/background.frag.spv"), backgroundTexture);

    // Note: we pass the background shader's texture with the intention of writing to it with the compute shader
    computeShader = new ComputeShader(device, physicalDevice, commandPool, computeQueue, swapChainExtent, 
        &offscreenPass.renderPass, std::string("Shaders/compute-clouds.comp.spv"), backgroundTexture, cloudPlacementTexture, 
        lowResCloudShapeTexture3D, hiResCloudShapeTexture3D);

    // Post shaders: there will be many
    // This is still offscreen, so the render pass is the offscreen render pass
    godRayShader = new PostProcessShader(device, physicalDevice, commandPool, graphicsQueue, swapChainExtent,
        &offscreenPass.renderPass, std::string("Shaders/post-pass.vert.spv"), std::string("Shaders/god-ray.frag.spv"), &offscreenPass.framebuffers[0].descriptor);

    radialBlurShader = new PostProcessShader(device, physicalDevice, commandPool, graphicsQueue, swapChainExtent,
        &offscreenPass.renderPass, std::string("Shaders/post-pass.vert.spv"), std::string("Shaders/radialBlur.frag.spv"), &offscreenPass.framebuffers[1].descriptor);

    toneMapShader = new PostProcessShader(device, physicalDevice, commandPool, graphicsQueue, swapChainExtent,
        &renderPass, std::string("Shaders/post-pass.vert.spv"), std::string("Shaders/tonemap.frag.spv"), &offscreenPass.framebuffers[2].descriptor);
}

void VulkanApplication::cleanupShaders() {
    delete meshShader;
    delete backgroundShader;
    delete computeShader;
    delete toneMapShader;
    delete godRayShader;
    delete radialBlurShader;
}

void VulkanApplication::cleanupOffscreenPass() {
    vkDestroySampler(device, offscreenPass.sampler, nullptr);
    
    for (auto& framebuffer : offscreenPass.framebuffers)
    {
        // Attachments
        vkDestroyImageView(device, framebuffer.color.view, nullptr);
        vkDestroyImage(device, framebuffer.color.image, nullptr);
        vkFreeMemory(device, framebuffer.color.mem, nullptr);
        vkDestroyImageView(device, framebuffer.depth.view, nullptr);
        vkDestroyImage(device, framebuffer.depth.image, nullptr);
        vkFreeMemory(device, framebuffer.depth.mem, nullptr);

        vkDestroyFramebuffer(device, framebuffer.framebuffer, nullptr);
    }

    vkDestroyRenderPass(device, offscreenPass.renderPass, nullptr);
    vkFreeCommandBuffers(device, commandPool, 1, &offscreenPass.commandBuffer);
    vkDestroySemaphore(device, offscreenPass.semaphore, nullptr);
}

void VulkanApplication::updateUniformBuffer() {
    float time = prevTime + deltaTime;

    UniformCameraObject uco = {};
    uco.proj = mainCamera.getProj();
    uco.proj[1][1] *= -1; // :(
    uco.view = mainCamera.getView();
    uco.cameraPosition = mainCamera.getPosition();

    UniformModelObject umo = {};
    umo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
    umo.model = glm::mat4(1.0f);
    umo.model[0][0] = 8.0f;
    umo.model[2][2] = 8.0f;
    umo.invTranspose = glm::inverse(glm::transpose(umo.model));
    float interp = sin(time * 0.2f) * 0.5f + 0.5f;

    skySystem.rebuildSkyFromNewSun(interp * 0.5f, 0.25f);
    skySystem.setTime(std::fmod(time * 2.f, 10000.f));

    UniformSkyObject sky = skySystem.getSky();
    UniformSunObject sun = skySystem.getSun();

    meshShader->updateUniformBuffers(uco, umo);
    computeShader->updateUniformBuffers(uco, sky, sun);
    godRayShader->updateUniformBuffers(uco, sun);
    radialBlurShader->updateUniformBuffers(uco, sun);
}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

// copy the contents from one buffer to another
void VulkanApplication::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

VkImageView VulkanApplication::createImageView(VkImage image, VkFormat format) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; //VK_IMAGE_USAGE_STORAGE_BIT
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }
    
    return imageView;
}

/// --- Vulkan Setup Functions

void VulkanApplication::createInstance() {

    #ifdef _DEBUG
    if (!checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers not supported");
    }
    #endif

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Sky Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // check GLFW extensions
    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    auto allextensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(allextensions.size());
    createInfo.ppEnabledExtensionNames = allextensions.data();

    // Validation Layer enabling
# if _DEBUG
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
# else
    createInfo.enabledLayerCount = 0;
# endif


    //VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

// If debugging is enabled, ensure that Vulkan is able to use validation layers.
bool VulkanApplication::checkValidationLayerSupport() {

#ifdef _DEBUG

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
#else
    return false;
#endif // _DEBUG

}

std::vector<const char*> VulkanApplication::getRequiredExtensions() {
    std::vector<const char*> extensions;

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (unsigned int i = 0; i < glfwExtensionCount; i++) {
        extensions.push_back(glfwExtensions[i]);
    }

#ifdef _DEBUG
    extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif
    return extensions;
}

void VulkanApplication::setupDebugCallback() {
#ifdef _DEBUG
    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = debugCallback;

    if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug callback!");
    }
#else 
    return;
#endif
}

bool VulkanApplication::isDeviceSuitable(VkPhysicalDevice device) {
/* TODO what is this lul
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    */
    // Add necessary features here, e.g.
    //return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
    //deviceFeatures.geometryShader;

    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

// Find the best GPU to run Vulkan on. Fail if nothing is suitable.
void VulkanApplication::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

// Check what command queues the GPU is capable of handling.
QueueFamilyIndices VulkanApplication::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        
        //TODO: is this correct
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            indices.computeFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (queueFamily.queueCount > 0 && presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

// After picking the GPU, make a logical device representation.
void VulkanApplication::createLogicalDevice() {

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

    float queuePriority = 1.0f;
    for (int queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // TODO : modify with specific features
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

#ifdef _DEBUG //DEBUG_VALIDATION
    
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

#else
    
    createInfo.enabledLayerCount = 0;
#endif
    
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    // TODO : multiple queues
    vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.computeFamily, 0, &computeQueue);
    vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
}

// Make a surface for Vulkan to draw on. GLFW handles this. (Platform-dependent)
void VulkanApplication::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

bool VulkanApplication::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

/// --- Swap Chain Functions

// TODO: move ASAP to a shader class
/// DELETE
VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

// TODO: get this out
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) {
    return findSupportedFormat(
    { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, physicalDevice
        );
}
void VulkanApplication::createRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // color and depth buffer
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0; // shader does layout(location = 0) for color!
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = findDepthFormat(physicalDevice); // depthTexture->getFormat(); << made out of order
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void VulkanApplication::createSemaphores() {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {

        throw std::runtime_error("failed to create semaphores!");
    }


}

/*void VulkanApplication::createComputePipeline() {
    // Set up programmable shader
    auto computeShaderCode = readFile("Shaders/helloTriangle.comp.spv");
    VkShaderModule computeShaderModule = createShaderModule(computeShaderCode, device);

    VkPipelineShaderStageCreateInfo computeShaderStageInfo = {};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = computeShaderModule;
    computeShaderStageInfo.pName = "main";

    // TODO: compute descriptor set layouts
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {computeSetLayout};

    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = 0;

    // Create that layout
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    // Create compute pipeline
    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = computeShaderStageInfo;
    pipelineInfo.layout = computePipelineLayout;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    // Create that pipeline
    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute pipeline");
    }

    // No longer need shader module
    vkDestroyShaderModule(device, computeShaderModule, nullptr);
}*/

void VulkanApplication::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
    poolInfo.flags = 0; // for recording once and executing many times. 
    // otherwise, use VK_COMMAND_POOL_CREATE_TRANSIENT_BIT -or- VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }

    // Compute command pool
    VkCommandPoolCreateInfo computePoolInfo = {};
    computePoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    computePoolInfo.queueFamilyIndex = queueFamilyIndices.computeFamily; //TODO: need compute index or whatever
    computePoolInfo.flags = 0;

    if (vkCreateCommandPool(device, &computePoolInfo, nullptr, &computeCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }
}

VkCommandBuffer VulkanApplication::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanApplication::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

// This function renders everything that is offscreen. The PostProcessCommandBuffer actually renders to the screen.
void VulkanApplication::createCommandBuffers() {

    if (offscreenPass.commandBuffer == VK_NULL_HANDLE)
    {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &offscreenPass.commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate offscreen command buffer!");
        }
    }

    if (offscreenPass.semaphore == VK_NULL_HANDLE)
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &offscreenPass.semaphore) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate offscreen semaphore!");
        }
    }

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &offscreenPass.commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate offscreen command buffer!");
    }
 
     VkCommandBufferBeginInfo beginInfo = {};
     beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
     beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
     beginInfo.pInheritanceInfo = nullptr; // Optional

     vkBeginCommandBuffer(offscreenPass.commandBuffer, &beginInfo);

     std::array<VkClearValue, 2> clearValues = {};
     clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
     clearValues[1].depthStencil = { 1.0f, 0 };

     // Actual render pass creation
     VkRenderPassBeginInfo renderPassInfo = {};
     renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
     renderPassInfo.renderPass = offscreenPass.renderPass;
     renderPassInfo.framebuffer = offscreenPass.framebuffers[0].framebuffer;
     renderPassInfo.renderArea.offset = { 0, 0 };
     renderPassInfo.renderArea.extent = swapChainExtent;
     VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
     renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
     renderPassInfo.pClearValues = clearValues.data();

     // Render pass recording
     vkCmdBeginRenderPass(offscreenPass.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

     // Draw Background
     backgroundShader->bindShader(offscreenPass.commandBuffer);
     backgroundGeometry->enqueueDrawCommands(offscreenPass.commandBuffer);

     vkCmdEndRenderPass(offscreenPass.commandBuffer);

     // Use the next framebuffer in the offscreen pass
     renderPassInfo.framebuffer = offscreenPass.framebuffers[1].framebuffer;

     // God rays and mesh drawing

     vkCmdBeginRenderPass(offscreenPass.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

     godRayShader->bindShader(offscreenPass.commandBuffer);
     backgroundGeometry->enqueueDrawCommands(offscreenPass.commandBuffer);

     vkCmdEndRenderPass(offscreenPass.commandBuffer);

     // Use the next framebuffer in the offscreen pass
     renderPassInfo.framebuffer = offscreenPass.framebuffers[2].framebuffer;

     // Radial Blur
     vkCmdBeginRenderPass(offscreenPass.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

     radialBlurShader->bindShader(offscreenPass.commandBuffer);
     backgroundGeometry->enqueueDrawCommands(offscreenPass.commandBuffer);

     // Draw Scene
     /*meshShader->bindShader(offscreenPass.commandBuffer);
     sceneGeometry->enqueueDrawCommands(offscreenPass.commandBuffer);*/

     vkCmdEndRenderPass(offscreenPass.commandBuffer);

     if (vkEndCommandBuffer(offscreenPass.commandBuffer) != VK_SUCCESS) {
         throw std::runtime_error("failed to record offscreen command buffer!");
     }
}

// Run the final post process that renders to the screen
void VulkanApplication::createPostProcessCommandBuffer() {
    commandBuffers.resize(swapChainFramebuffers.size());
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr; // Optional

        vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };

        // Actual render pass creation
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChainExtent;
        VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        // Render pass recording
        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        toneMapShader->bindShader(commandBuffers[i]);
        backgroundGeometry->enqueueDrawCommands(commandBuffers[i]);

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

void VulkanApplication::createComputeCommandBuffer() {
    // Specify the command pool and number of buffers to allocate
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = computeCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &computeCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    // Begin recording
    if (vkBeginCommandBuffer(computeCommandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording compute command buffer");
    }

    computeShader->bindShader(computeCommandBuffer);

    // TODO: dispatch according to the number of pixels, do in a 2d manner? see the raytracing example
    // first TODO: launch this compute shader for the triangle being rendered
    //const int IMAGE_SIZE = 32;
    const glm::ivec2 texDims(swapChainExtent.width, swapChainExtent.height);
    vkCmdDispatch(computeCommandBuffer, static_cast<uint32_t>((texDims.x + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE), static_cast<uint32_t>((texDims.y + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE), 1);

    // End recording
    if (vkEndCommandBuffer(computeCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record compute command buffer");
    }
}

void VulkanApplication::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());
    // iterate through all image views and create frame buffers from them
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            swapChainImageViews[i],
            depthTexture->textureImageView
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

// Needs to be called once for each post process effect
// Expects a framebuffer, color and depth format to sample
void VulkanApplication::createOffscreenFramebuffer(FrameBuffer* framebuffer, VkFormat colorFormat, VkFormat depthFormat)
{
    // Color attachment
    VkImageCreateInfo image {};
    image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = colorFormat;
    image.extent.width = WIDTH;
    image.extent.height = HEIGHT;
    image.extent.depth = 1;
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    // We will sample directly from the color attachment
    image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VkMemoryAllocateInfo memAlloc {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memReqs;

    VkImageViewCreateInfo colorImageView {};
    colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorImageView.format = colorFormat;
    colorImageView.flags = 0;
    colorImageView.subresourceRange = {};
    colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorImageView.subresourceRange.baseMipLevel = 0;
    colorImageView.subresourceRange.levelCount = 1;
    colorImageView.subresourceRange.baseArrayLayer = 0;
    colorImageView.subresourceRange.layerCount = 1;

    if (vkCreateImage(device, &image, nullptr, &framebuffer->color.image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    vkGetImageMemoryRequirements(device, framebuffer->color.image, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice);
    if (vkAllocateMemory(device, &memAlloc, nullptr, &framebuffer->color.mem) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate memory!");
    }

    if (vkBindImageMemory(device, framebuffer->color.image, framebuffer->color.mem, 0) != VK_SUCCESS) {
        throw std::runtime_error("failed to bind image memory!");
    }

    colorImageView.image = framebuffer->color.image;
    if(vkCreateImageView(device, &colorImageView, nullptr, &framebuffer->color.view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    // Depth stencil attachment
    image.format = depthFormat;
    image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageViewCreateInfo depthStencilView {};
    depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthStencilView.format = depthFormat;
    depthStencilView.flags = 0;
    depthStencilView.subresourceRange = {};
    depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;// | VK_IMAGE_ASPECT_STENCIL_BIT;
    depthStencilView.subresourceRange.baseMipLevel = 0;
    depthStencilView.subresourceRange.levelCount = 1;
    depthStencilView.subresourceRange.baseArrayLayer = 0;
    depthStencilView.subresourceRange.layerCount = 1;

    if (vkCreateImage(device, &image, nullptr, &framebuffer->depth.image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }
    vkGetImageMemoryRequirements(device, framebuffer->depth.image, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice);
    if (vkAllocateMemory(device, &memAlloc, nullptr, &framebuffer->depth.mem) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate memory!");
    }

    if (vkBindImageMemory(device, framebuffer->depth.image, framebuffer->depth.mem, 0) != VK_SUCCESS) {
        throw std::runtime_error("failed to bind image!");
    }

    depthStencilView.image = framebuffer->depth.image;
    if (vkCreateImageView(device, &depthStencilView, nullptr, &framebuffer->depth.view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    VkImageView attachments[2];
    attachments[0] = framebuffer->color.view;
    attachments[1] = framebuffer->depth.view;

    VkFramebufferCreateInfo fbufCreateInfo {};
    fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbufCreateInfo.renderPass = offscreenPass.renderPass;
    fbufCreateInfo.attachmentCount = 2;
    fbufCreateInfo.pAttachments = attachments;
    fbufCreateInfo.width = WIDTH;
    fbufCreateInfo.height = HEIGHT;
    fbufCreateInfo.layers = 1;

    if (vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &framebuffer->framebuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer!");
    }

    // Fill a descriptor for later use in a descriptor set 
    framebuffer->descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    framebuffer->descriptor.imageView = framebuffer->color.view;
    framebuffer->descriptor.sampler = offscreenPass.sampler;
}

void VulkanApplication::setupOffscreenPass() {
    offscreenPass.width = WIDTH;
    offscreenPass.height = HEIGHT;

    // Find a suitable depth format
    VkFormat fbDepthFormat = findDepthFormat(physicalDevice);

    // Create a separate render pass for the offscreen rendering as it may differ from the one used for scene rendering
    std::array<VkAttachmentDescription, 2> attachmentDescriptions = {};

    // Color attachment
    attachmentDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Depth attachment
    attachmentDescriptions[1].format = fbDepthFormat;
    attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;
    subpassDescription.pDepthStencilAttachment = &depthReference;

    // Use subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Create the actual renderpass
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
    renderPassInfo.pAttachments = attachmentDescriptions.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &offscreenPass.renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }

    // Create sampler to sample from the color attachments
    VkSamplerCreateInfo sampler {};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = sampler.addressModeU;
    sampler.addressModeW = sampler.addressModeU;
    sampler.mipLodBias = 0.0f;
    sampler.maxAnisotropy = 1.0f;
    sampler.minLod = 0.0f;
    sampler.maxLod = 1.0f;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    if (vkCreateSampler(device, &sampler, nullptr, &offscreenPass.sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create sampler!");
    }

    // Create offscreen frame buffers - note the image format, they are HDR
    createOffscreenFramebuffer(&offscreenPass.framebuffers[0], VK_FORMAT_R32G32B32A32_SFLOAT, fbDepthFormat);
    createOffscreenFramebuffer(&offscreenPass.framebuffers[1], VK_FORMAT_R32G32B32A32_SFLOAT, fbDepthFormat);
    createOffscreenFramebuffer(&offscreenPass.framebuffers[2], VK_FORMAT_R32G32B32A32_SFLOAT, fbDepthFormat);
}

void VulkanApplication::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    // queue length. 0 means no limit aside from memory
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.oldSwapchain = VK_NULL_HANDLE; // only one swap chain for now
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    // create the handles from the swap chain to images
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
}

// Same as cleanup(), but all things related to swapchain
void VulkanApplication::cleanupSwapChain() {
    for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
    }

    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    //vkDestroyPipeline(device, graphicsPipeline, nullptr);
    //vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        vkDestroyImageView(device, swapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

// TODO
void VulkanApplication::recreateSwapChain() {
    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    //createGraphicsPipeline();
    createFramebuffers();
    createCommandBuffers();
}

void VulkanApplication::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (uint32_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);
    }
}

// Populate the details of swap chain capabilities
SwapChainSupportDetails VulkanApplication::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }


    return details;
}

// Determine the format / color space of the swap chain, e.g. r8g8b8a8
VkSurfaceFormatKHR VulkanApplication::chooseSwapSurfaceFormat(const std::vector <VkSurfaceFormatKHR>& availableFormats) {
    // default: no preferred format
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0]; // probably shouldn't happen
}

// Determine the way the swap chain will handle submitted images
VkPresentModeKHR VulkanApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
    // Types of modes:
    // IMMEDIATE: submitted by app goes to screen ASAP
    // FIFO: queue of images, analagous to vsync
    // FIFO_RELAXED: queue but does ASAP transfer if queue is empty
    // MAILBOX: queue but overwrites if full
    
    // MAILBOX is ideal but not guaranteed to be supported
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
//		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
//			bestMode = availablePresentMode;
//		}
    }

    return bestMode;
}

// Get the resolution of the swap chain
VkExtent2D VulkanApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}