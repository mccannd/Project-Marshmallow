#pragma once
#include "Texture.h"
#include "Geometry.h"
#include <fstream>

// Need to move this
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

struct UniformCameraObject {
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 cameraPosition;

    static VkDescriptorSetLayoutBinding getLayoutBinding(uint32_t bind)
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = bind;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        return uboLayoutBinding;
    }
};

struct UniformModelObject {
    glm::mat4 model;
    glm::mat4 invTranspose;

    static VkDescriptorSetLayoutBinding getLayoutBinding(uint32_t bind)
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = bind;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        return uboLayoutBinding;
    }
};

class Shader: public VulkanObject
{
protected:
    // All shaders have layouts and pipelines to delete.
    virtual void cleanup();

    // Different shaders will have different numbers of uniform buffers, need to implement cleanup for each.
    virtual void cleanupUniforms() = 0;

    virtual void createDescriptorSetLayout() = 0;
    virtual void createDescriptorPool() = 0;
    virtual void createDescriptorSet() = 0;
    virtual void createUniformBuffer() = 0;
    virtual void createPipelineLayout() = 0;
    virtual void createPipeline() = 0;

    VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);

    std::vector<std::string> shaderFilePaths;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    // Textures to be used for samplers
    std::vector<Texture*> textures;
    // Renderpass will handle attachments like depth/stencil, MUST initialize externally
    VkRenderPass* renderPass;
    // View size
    VkExtent2D extent;

public:
    // all shaders need a setup function, but will have variable # arguments...
    // for now: make part of constructor

    Shader(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, VkExtent2D extent) : 
        VulkanObject(device, physicalDevice, commandPool, queue) {
        this->extent = extent;
    }
    virtual ~Shader() {
        cleanup();
    }

    // Must set the render pass for shaders before pipeline creation.
    void setRenderPass(VkRenderPass* renderPass) { this->renderPass = renderPass; }
    
    // Samplers must be initialized before pipeline / descriptor creation.
    void addTexture(Texture* tex) { textures.push_back(tex); }
};

// TODO: only albedo for the moment,
// 1. Add cook torrance code for ROUGH_METAL_AO
// 2. Add normal mapping support
// 3. Emissive?
// 4. Find ways to pack alpha channels?
enum PBRTEXTURES
{
    ALBEDO = 0, ROUGH_METAL_AO, NORMAL
};

/*
* A shader pipeline to rasterize a mesh.
*/
class MeshShader : Shader
{
private:
    
protected:
    virtual void createDescriptorSetLayout();
    virtual void createDescriptorPool();
    virtual void createDescriptorSet();

    virtual void createUniformBuffer();

    virtual void createPipeline();

    UniformCameraObject cameraUniforms;
    UniformModelObject modelUniforms;

    VkBuffer uniformCameraBuffer;
    VkDeviceMemory uniformCameraBufferMemory;
    VkBuffer uniformModelBuffer;
    VkDeviceMemory uniformModelBufferMemory;

    virtual void cleanupUniforms();
public:
    void setupShader(std::string vertPath, std::string fragPath) {
        shaderFilePaths.push_back(vertPath);
        shaderFilePaths.push_back(fragPath);

        createDescriptorSetLayout();
        createPipeline();
        createUniformBuffer();
        createDescriptorPool();
        createDescriptorSet();
    }
    
    MeshShader(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, VkExtent2D extent) : Shader(device, physicalDevice, commandPool, queue, extent) {}
    MeshShader(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, VkExtent2D extent, VkRenderPass *renderPass, std::string vertPath, std::string fragPath, Texture* tex) :
        Shader(device, physicalDevice, commandPool, queue, extent) {
        this->renderPass = renderPass;
        addTexture(tex);
        setupShader(vertPath, fragPath);
    }

    virtual ~MeshShader() { cleanupUniforms(); }

    void updateUniformBuffers(UniformCameraObject cam, UniformModelObject model) {
        void* data;
        vkMapMemory(device, uniformCameraBufferMemory, 0, sizeof(cam), 0, &data);
        memcpy(data, &cam, sizeof(cam));
        vkUnmapMemory(device, uniformCameraBufferMemory);

        vkMapMemory(device, uniformModelBufferMemory, 0, sizeof(model), 0, &data);
        memcpy(data, &model, sizeof(model));
        vkUnmapMemory(device, uniformModelBufferMemory);
    }

};

/*
TODO: A pipeline for drawing a background quad
*/

/*
TODO: A pipeline for computing clouds
*/

