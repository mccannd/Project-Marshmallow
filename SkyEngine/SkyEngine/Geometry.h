#pragma once
#include "VulkanObject.h"


#include <unordered_map>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 col;
    glm::vec2 uv;
    glm::vec3 nor;

    // rate to load data from memory throughout vertices
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    // Set layout bindings for vertex struct to vertex shader
    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, col);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, uv);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, nor);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && col == other.col && uv == other.uv && nor == other.nor;
    }

};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.col) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv) << 1);
        }
    };
}


class Geometry : VulkanObject
{
private:
    virtual void cleanup();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexDeviceMemory;

    VkBuffer indexBuffer;
    VkDeviceMemory indexDeviceMemory;

    void createVertexBuffer();
    void createIndexBuffer();

    bool initialized = false;

    void initializeTBN();

public:
    Geometry(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue) : VulkanObject(device, physicalDevice, commandPool, queue) {}
    ~Geometry() { cleanup(); }

    // not terribly neat, but better than subclasses for now...
    void setupAsQuad();
    void setupAsBackgroundQuad();
    void setupFromMesh(std::string path);

    void enqueueDrawCommands(VkCommandBuffer& commandBuffer);
};

