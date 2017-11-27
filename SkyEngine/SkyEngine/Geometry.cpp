#include "Geometry.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

void Geometry::cleanup() {
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexDeviceMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexDeviceMemory, nullptr);
}

void Geometry::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexDeviceMemory);

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Geometry::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // note that this is specified as an index buffer
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexDeviceMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

/* Calls commands to ready the buffers for drawing.
* Call this only after the respective command buffer is recording and UBO are bound
*/
void Geometry::enqueueDrawCommands(VkCommandBuffer& commandBuffer) {
    if (!initialized) return;
    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void Geometry::setupAsQuad() {
    if (initialized) cleanup();

    vertices = {
        { { -0.5f, -0.5f, 0.0f },{ 1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f }, {0.0f, 0.0f, -1.0f } },
        { { 0.5f, -0.5f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, -1.0f } },
        { { 0.5f,  0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, -1.0f } },
        { { -0.5f,  0.5f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 0.0f, -1.0f } },

        { { -0.5f, -0.5f, -0.5f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, -1.0f } },
        { { 0.5f, -0.5f, -0.5f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 0.0f, -1.0f } },
        { { 0.5f,  0.5f, -0.5f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 0.0f, -1.0f } },
        { { -0.5f,  0.5f, -0.5f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, -1.0f } }
    };

    indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };

    createVertexBuffer();
    createIndexBuffer();

    initialized = true;
}

void Geometry::setupAsBackgroundQuad() {
    if (initialized) cleanup();

    vertices = {
        { { -1.0f, -1.0f, 0.999f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, -1.0f } },
        { { 1.0f, -1.0f, 0.999f },{ 1.0f, 1.0f, 1.0f },{ 1.0f, 0.0f },{ 0.0f, 0.0f, -1.0f } },
        { { 1.0f,  1.0f, 0.999f },{ 1.0f, 1.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 0.0f, -1.0f } },
        { { -1.0f,  1.0f, 0.999f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, -1.0f } }
    };

    indices = {
        0, 1, 2, 2, 3, 0
    };

    createVertexBuffer();
    createIndexBuffer();

    initialized = true;

}

void Geometry::setupFromMesh(std::string path) {
    if (initialized) cleanup();

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str())) {
        throw std::runtime_error(err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

    for (const auto& shape : shapes) {

        for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {};
            
            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.uv = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.col = { 1.0f, 1.0f, 1.0f };

            vertex.nor = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }

    createVertexBuffer();
    createIndexBuffer();

    initialized = true;
}