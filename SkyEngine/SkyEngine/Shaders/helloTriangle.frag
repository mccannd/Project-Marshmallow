#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformCameraObject {
    mat4 view;
    mat4 proj;
    vec3 cameraPosition;
} camera;

layout(binding = 1) uniform UniformModelObject {
    mat4 model;
    mat4 invTranspose;
} model;

layout(binding = 2) uniform sampler2D texColor;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragPosition;
layout(location = 3) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 col = texture(texColor, fragUV).xyz;
    vec3 N = normalize(fragNormal);
    col *= fragColor;
    vec3 lightVec = normalize(vec3(-1, -1, -1));
    lightVec = (camera.view * vec4(lightVec, 0)).xyz;
    float lighting = max(0.2, dot(N, -lightVec));
    col *= lighting;

    //col = 0.5 + 0.5 * N;
    //col = vec3 (clamp(fragPosition.y, 0.0, 1.0));
    //col = clamp(camera.cameraPosition, vec3(0), vec3(1));
    outColor = vec4(col, 1.0);
}