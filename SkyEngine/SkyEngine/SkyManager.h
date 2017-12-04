#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <algorithm>

struct UniformSunObject {
    
    glm::vec4 location; // only really used for the procedural scattering. regard this as a directional light
    glm::vec4 direction; 
    glm::vec4 color;
    glm::mat4 directionBasis; // Equivalent to TBN, for transforming cone samples in ray marcher
    float intensity;
    static VkDescriptorSetLayoutBinding getLayoutBinding(uint32_t bind)
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = bind;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        return uboLayoutBinding;
    }
};

struct UniformSkyObject{
    
    // many other constants are stored in the sky manager, minimizing the amount of stuff transferred to shader

    // precalculated, depends on sun
    glm::vec4 betaR;
    glm::vec4 betaV;
    glm::vec4 wind;
    float mie_directional;

    static VkDescriptorSetLayoutBinding getLayoutBinding(uint32_t bind)
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = bind;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        return uboLayoutBinding;
    }
};

class SkyManager
{
private:
    float elevation, azimuth;
    float turbidity;
    float rayleigh;
    float mie;
    UniformSkyObject sky;
    UniformSunObject sun;
    void calcSunPosition();
    void calcSunIntensity();
    void calcSunColor();

    void calcSkyBetaR();
    void calcSkyBetaV();

public:
    SkyManager();
    ~SkyManager();
    void rebuildSkyFromNewSun(float elevation, float azimuth);
    void rebuildSkyFromScattering(float turbidity, float mie, float mie_directional);
    void rebuildSky(float elevation, float azimuth, float turbidity, float mie, float mie_directional);
    void setWindDirection(const glm::vec3 dir) { sky.wind = glm::vec4(dir, sky.wind.w); }
    void setTime(float t) { sky.wind.w = t; }
    UniformSunObject getSun() { return sun; }
    UniformSkyObject getSky() { return sky; }
};

