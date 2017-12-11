#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(binding = 0) uniform UniformCameraObject {
    mat4 view;
    mat4 proj;
    vec4 cameraPosition;
} camera;

layout(binding = 1) uniform UniformModelObject {
    mat4 model;
    mat4 invTranspose;
} model;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragPosition;
layout(location = 3) out vec3 fragNormal;
layout(location = 4) out vec3 fragTangent;
layout(location = 5) out vec3 fragBitangent;

void main() {
    gl_Position = camera.proj * camera.view * model.model * vec4(inPosition, 1.0);
    fragUV = inUV;
	fragColor = inColor;

    fragPosition = (camera.view * model.model * vec4(inPosition, 1.0)).xyz;
    //(model.model * vec4(inPosition, 1.0)).xyz;//
    
    fragNormal = normalize((camera.view * vec4(normalize((model.invTranspose * vec4(inNormal, 0.0)).xyz), 0.0)).xyz);
    vec3 up = normalize(vec3(0.001, 1, -0.004));
    fragTangent = normalize(cross(fragNormal, up));
    fragBitangent =cross(fragNormal, fragTangent);
}