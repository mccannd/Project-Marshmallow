#version 450
#extension GL_ARB_separate_shader_objects : enable
#define EPSILON 0.00001
#define PI 3.14159

layout(binding = 0) uniform UniformCameraObject {
    mat4 view;
    mat4 proj;
    vec4 cameraPosition;
} camera;

layout(binding = 1) uniform UniformModelObject {
    mat4 model;
    mat4 invTranspose;
} model;

layout(binding = 2) uniform sampler2D texColor;
layout(binding = 3) uniform sampler2D pbrInfo; 
layout(binding = 4) uniform sampler2D normalMap;
// rough metal AO height

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragPosition;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec3 fragTangent;
layout(location = 5) in vec3 fragBitangent;

layout(location = 0) out vec4 outColor;


vec3 getNormal() {
    vec3 nm = texture(normalMap, fragUV).xyz;
    nm.xyz = 2.0 * nm.xyz - 1.0;
    return nm.r * normalize(fragTangent) - nm.g * normalize(fragBitangent) + nm.b * normalize(fragNormal);
}

// call this separately from the BRDF
vec3 fresnelSchlick(in vec3 specular, in vec3 N, in vec3 V) {
    float NdotV = max(0.0, dot(N, V));
    float refl = max(max(specular.r, specular.g), specular.b);
    refl = (clamp(refl * 25.0, 0.0, 1.0));
    vec3 reflectance0 = specular;
    vec3 reflectance90 = vec3(refl);
    return reflectance0 + (reflectance90 - reflectance0) * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);
}

float smithG1(in float NdotV, in float r) {
    float ts = (1.0 - NdotV * NdotV) / max(NdotV * NdotV, EPSILON);
    return 2.0 / (1.0 + sqrt(1.0 + r * r * ts));
}

float geometricOcclusion(in float NdotL, in float NdotV, in float roughness) {
    return smithG1(NdotL, roughness) * smithG1(NdotV, roughness);
}

float distributionGGX(in float roughness, in float NdotH) {
    float alpha = roughness * roughness;
    float f = (NdotH * alpha - NdotH) * NdotH + 1.0;
    return alpha / (PI * f * f);
}

float specularBRDFwithoutFresnel(in vec3 N, in vec3 L, in vec3 V, in float roughness) {
    vec3 H = normalize(L + V);
    float NdotL = clamp(dot(N, L), 0.0, 1.0);
    float NdotV = abs(dot(N, V));
    float NdotH = clamp(dot(N, H), 0.0, 1.0);

    float G = geometricOcclusion(NdotL, NdotV, roughness);
    float D = distributionGGX(roughness, NdotH);
    return G * D / max(EPSILON, (4.0 * NdotL * NdotV));
}

vec3 lambertBRDF(in vec3 fresnel) {
    return (1.0 - fresnel) / PI;
}

vec3 pbrMaterialColor(in vec3 fresnel, in vec3 N, in vec3 L, in vec3 V, in float roughness, in vec3 diffuse, in vec3 specular) {
    return max(0.0, dot(N, L)) * (diffuse * lambertBRDF(fresnel) + specular * fresnel * specularBRDFwithoutFresnel(N, L, V, roughness));
}


void main() {
    vec4 albedo = texture(texColor, fragUV);
    vec4 pbrParams = texture(pbrInfo, fragUV);
    vec3 N = normalize(getNormal());
    vec3 V = -normalize(fragPosition);
    vec3 L = normalize((camera.view * vec4(normalize(vec3(1, 1, 1)), 0)).xyz); // arbitrary for now

    float roughness = pbrParams.r;
    roughness *= roughness; // perceptual roughness
    float metalness = pbrParams.g;

    vec3 f0 = vec3(0.04); // baseline specularity for metallic model
    vec3 diffuse = mix(albedo.rgb * (1.0 - f0), vec3(0.0), metalness);
    vec3 specular = mix(f0, albedo.rgb, metalness);

    // spooky tricks since no real shadow mapping
    float shadowHack = dot(L, normalize((fragNormal)));
    shadowHack = clamp(5.0 * (shadowHack + 0.1), 0.0, 1.0);

    vec3 F = fresnelSchlick(specular, N, V);
    vec3 color = pbrMaterialColor(F, N, L, V, roughness, diffuse, specular);
    color *= shadowHack * 20.0 * vec3(1.0, 0.8, 0.6); // hard code light color for now



    color += diffuse * mix(vec3(0), 2.0 * vec3(0.6, 0.7, 1.0), 0.5 + 0.5 * dot(N, normalize((camera.view * vec4(normalize(vec3(0, 1, 0)), 0)).xyz)));
    vec3 aoColor = mix(vec3(0.1, 0.1, 0.3), vec3(1), pbrParams.b);
    color.rgb *= aoColor;
    outColor = vec4(color, 1.0);

    //outColor = vec4(0.5 + 0.5 * normalize(N), 1.0);
    //outColor = vec4(pbrParams.rgb, 1.0);
}