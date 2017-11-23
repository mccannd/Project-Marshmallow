#pragma once

#include "VulkanObject.h"
#include <string>

class Texture : VulkanObject
{
private:
    int width, height, channels;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;

    VkFormat imageFormat;

    virtual void cleanup();
    void createSampler();
    void createImageView();

    void createImage(uint32_t width, uint32_t height, VkImageUsageFlags usage, VkFormat format, VkMemoryPropertyFlags properties, VkImageTiling tiling);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
public:
    Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM)
        : VulkanObject(device, physicalDevice, commandPool, queue) {
        imageFormat = format;
    }

    ~Texture() {
        cleanup();
    }

    VkImageView textureImageView;
    VkSampler textureSampler;

    void initFromFile(std::string path);
    void initForStorage(VkExtent2D extent);
};

