#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D texColor;

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

// Uniform buffers - need camera and sun parameters

/*layout(binding = 1) uniform UniformCameraObject {

    mat4 view;
    mat4 proj;
    vec3 cameraPosition;
} camera;*/

// all of these components are calculated in SkyManager.h/.cpp
layout(set = 0, binding = 2) uniform UniformSunObject {
    
    vec4 location;
    vec4 direction;
    vec4 color;
    mat4 directionBasis;
    float intensity;
} sun;

#define NUM_SAMPLES 80
#define NUM_SAMPLES_F float(NUM_SAMPLES)

void main() {
    vec2 scrPt = fragUV * 2.0 - 1.0;

    vec4 currentFragment = texture(texColor, fragUV);





    outColor = vec4(sun.color.xyz * sun.intensity * currentFragment.a + 0.5 * currentFragment.xyz, 1.0);
}