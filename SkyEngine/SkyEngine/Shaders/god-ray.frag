#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D texColor;

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

// Uniform buffers - need camera and sun parameters

layout(binding = 1) uniform UniformCameraObject {

    mat4 view;
    mat4 proj;
    vec3 cameraPosition;
} camera;

// all of these components are calculated in SkyManager.h/.cpp
layout(set = 0, binding = 2) uniform UniformSunObject {
    
    vec4 location;
    vec4 direction;
    vec4 color;
    mat4 directionBasis;
    float intensity;
} sun;

// Based on the GPU Gem: https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch13.html
// Also referenced: https://medium.com/community-play-3d/god-rays-whats-that-5a67f26aeac2

#define NUM_SAMPLES 80
#define NUM_SAMPLES_F float(NUM_SAMPLES)

// Parameters, TODO: should move to a uniform buffer
#define DENSITY 0.5
#define DECAY 0.99 // this generally makes the god rays more or less intense
#define EXPOSURE 1.0
#define SAMPLE_WEIGHT 1.0 / NUM_SAMPLES_F

void main() {
    vec2 scrPt = fragUV * 2.0 - 1.0;
    vec2 currentSamplePoint = scrPt;
    vec4 sunPos = camera.proj * camera.view * sun.location;
    sunPos /= sunPos.w;
    vec2 deltaLightVec = currentSamplePoint - sunPos.xy;
    deltaLightVec *= SAMPLE_WEIGHT * DENSITY;

    vec4 currentFragment = texture(texColor, fragUV);
    vec3 accumSampleColor = currentFragment.xyz * currentFragment.a * 0.5;
    float illuminationDecay = 1.0;

    for(int i = 0; i < NUM_SAMPLES; ++i)
    {
        // Step the sample position
        currentSamplePoint -= deltaLightVec;
        
        // Sample the scene
        vec4 currentSampleColor = texture(texColor, currentSamplePoint * 0.5 + 0.5) * 0.5;
        currentSampleColor.xyz *= SAMPLE_WEIGHT * illuminationDecay * currentSampleColor.a;
        
        // Accumulate color
        accumSampleColor += currentSampleColor.xyz;
        
        // Factor in decay
        illuminationDecay *= DECAY;
    }

    outColor = vec4(accumSampleColor * EXPOSURE + currentFragment.xyz * 0.5, 1.0);
}