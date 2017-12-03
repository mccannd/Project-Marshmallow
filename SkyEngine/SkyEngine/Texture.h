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

