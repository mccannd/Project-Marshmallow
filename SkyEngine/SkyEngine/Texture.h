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

    bool initialized = false;
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    VkImageAspectFlagBits usageBit = VK_IMAGE_ASPECT_COLOR_BIT;
public:
    Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM)
        : VulkanObject(device, physicalDevice, commandPool, queue) {
        imageFormat = format;
    }

    ~Texture() {
        cleanup();
    }

    VkFormat getFormat() { return imageFormat; }
    VkImageView textureImageView;
    VkSampler textureSampler;

    void initFromFile(std::string path);
    void initForStorage(VkExtent2D extent);
    void initForDepthAttachment(VkExtent2D extent);

    static VkDescriptorSetLayoutBinding getLayoutBinding(uint32_t bind)
    {
        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = bind;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        return samplerLayoutBinding;
    }
};

/// Texture 3D

class Texture3D : VulkanObject
{
private:
    int width, height, depth, channels;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;

    VkFormat imageFormat;

    virtual void cleanup();
    void createSampler();
    void createImageView();

    void createImage(uint32_t width, uint32_t height, uint32_t depth, VkImageUsageFlags usage, VkFormat format, VkMemoryPropertyFlags properties, VkImageTiling tiling);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth);

    bool initialized = false;
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    VkImageAspectFlagBits usageBit = VK_IMAGE_ASPECT_COLOR_BIT;
public:
    Texture3D(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue,
        const uint32_t width, const uint32_t height, const uint32_t depth,
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM) :
        VulkanObject(device, physicalDevice, commandPool, queue), width(width), height(height), depth(depth) {
        imageFormat = format;
    }

    ~Texture3D() {
        cleanup();
    }

    VkFormat getFormat() { return imageFormat; }
    VkImageView textureImageView;
    VkSampler textureSampler;

    // This function should supply the "base" name of each texture slice file.
    void initFromFile(std::string path);
    void initForStorage(VkExtent3D extent);
    void initForDepthAttachment(VkExtent3D extent);

    static VkDescriptorSetLayoutBinding getLayoutBinding(uint32_t bind)
    {
        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = bind;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        return samplerLayoutBinding;
    }
};
