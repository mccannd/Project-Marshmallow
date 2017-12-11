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

// all of these components are calculated in SkyManager.h/.cpp
layout(set = 0, binding = 2) uniform UniformSunObject {
    
    vec4 location;
    vec4 direction;
    vec4 color;
    mat4 directionBasis;
    float intensity;
} sun;

// note: a lot of sky constants are stored/precalculated in SkyManager.h / .cpp
layout(binding = 3) uniform UniformSkyObject {
    
    vec4 betaR;
    vec4 betaV;
    vec4 wind;
    float mie_directional;
} sky;

layout(binding = 4) uniform sampler2D texColor;
layout(binding = 5) uniform sampler2D pbrInfo; 
layout(binding = 6) uniform sampler2D normalMap;
layout(binding = 7) uniform sampler2D cloudPlacement;
layout(binding = 8) uniform sampler3D lowResCloudShape;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragPosition;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec3 fragTangent;
layout(location = 5) in vec3 fragBitangent;
layout(location = 6) in vec3 fragPositionWC;

layout(location = 0) out vec4 outColor;

/// Defines/Functions copied from compute-clouds.comp. A brief raymarch occurs in this function for shadows.

struct Intersection {
    vec3 normal;
    vec3 point;
    bool valid;
    float t;
};

#define ATMOSPHERE_RADIUS 1000000.0 //* 0.0625
#define ASPECT_RATIO 1280.0 / 720.0
#define NUM_SHADOW_STEPS 6
#define WIND_STRENGTH 20.0

float remap(in float value, in float oldMin, in float oldMax, in float newMin, in float newMax) {
    return newMin + (((value - oldMin) / (oldMax - oldMin)) * (newMax - newMin));
}

float remapClamped(in float value, in float oldMin, in float oldMax, in float newMin, in float newMax) {
    return clamp(newMin + (((value - oldMin) / (oldMax - oldMin)) * (newMax - newMin)), newMin, newMax);
}

// Get the point projected to the inner atmosphere shell
vec3 getProjectedShellPoint(in vec3 pt, in vec3 center) {
    return 0.5 * ATMOSPHERE_RADIUS * normalize(pt - center) + center;
}

// Given a point, the point projected to the inner atmosphere shell, and the thickness of the shell
// return the normalized height within the shell.
float getRelativeHeight(in vec3 pt, in vec3 projectedPt, in float thickness) {
    return clamp(length(pt - projectedPt) / thickness, 0.0, 1.0);
}

// Get the blended density gradient for 3 different cloud types
// relativeHeight is normalized distance from inner to outer atmosphere shell
// cloudType is read from cloud placement blue channel
float cloudLayerDensity(float relativeHeight, float cloudType) {
    relativeHeight = clamp(relativeHeight, 0, 1);

    float cumulus = max(0.0, remap(relativeHeight, 0.0, 0.2, 0.0, 1.0) * remap(relativeHeight, 0.7, 1.0, 1.0, 0.0));
    float stratocumulus = max(0.0, remap(relativeHeight, 0.0, 0.2, 0.0, 1.0) * remap(relativeHeight, 0.2, 0.7, 1.0, 0.0)); 
    float stratus = max(0.0, remap(relativeHeight, 0.0, 0.1, 0.0, 1.0) * remap(relativeHeight, 0.2, 0.3, 1.0, 0.0)); 

    float d1 = mix(stratus, stratocumulus, clamp(cloudType * 2.0, 0.0, 1.0));
    float d2 = mix(stratocumulus, stratus, clamp((cloudType - 0.5) * 2.0, 0.0, 1.0));
    return mix(d1, d2, cloudType);
}

float heightBiasCoverage(float coverage, float height) {
    return pow(coverage, clamp(remap(height, 0.7, 0.8, 1.0, 0.6), 0.6, 1.0));
}

// Checks if a cloud is at this point. If not, return 0 immediately. Otherwise get low-res density. (can still be 0 given cloud coverage)
float cloudTest(in vec3 pos, in float relativeHeight, in vec3 earthCenter, inout float coverage) {

    float density;

    vec3 currentProj = getProjectedShellPoint(pos, earthCenter);
    vec3 cloudInfo = texture(cloudPlacement, 0.00001 * (currentProj.xz - camera.cameraPosition.xz)).xyz;
    float layerDensity = cloudLayerDensity(relativeHeight, cloudInfo.z);

    vec4 densityNoise = texture(lowResCloudShape, 0.000057 * vec3(pos));

    density = layerDensity * remapClamped(densityNoise.x, 0.3, 1.0, 0.0, 1.0);

    coverage = 0.0;
    // early check before more expensive math
    if (density < 0.0001) return 0.0;

    coverage = heightBiasCoverage(relativeHeight, min(0.85, cloudInfo.r));

    float erosion = 0.625 * densityNoise.y + 0.25 * densityNoise.z + 0.125 * densityNoise.w;

    erosion = remapClamped(erosion, coverage, 1.0, 0.0, 1.0);
    //density = remapClamped(density, 1.0 - coverage, 1.0, 0.0, 1.0);
    density = remapClamped(density, erosion, 1.0, 0.0, 1.0);

    return density;
}

// Compute sphere intersection
Intersection raySphereIntersection(in vec3 ro, in vec3 rd, in vec4 sphere) {
	Intersection isect;
    isect.valid = false;
    isect.point = vec3(0);
    isect.normal = vec3(0, 1, 0);
    
    // no rotation, only uniform scale, always a sphere
    ro -= sphere.xyz;
    ro /= sphere.w;
    
    float A = dot(rd, rd);
    float B = 2.0 * dot(rd, ro);
    float C = dot(ro, ro) - 0.25;
    float discriminant = B * B - 4.0 * A * C;
    
    if (discriminant < 0.0) return isect;
    float t = (-sqrt(discriminant) - B) / A * 0.5;
    if (t < 0.0) t = (sqrt(discriminant) - B) / A * 0.5;
    
    if (t >= 0.0) {
        isect.valid = true;
    	vec3 p = vec3(ro + rd * t);
        isect.normal = normalize(p);
        p *= sphere.w;
        p += sphere.xyz;
        isect.point = p;
        isect.t = length(p - ro);
    }
    
    return isect;
}

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
    albedo.xyz = pow(albedo.xyz, vec3(2.2));
    vec4 pbrParams = texture(pbrInfo, fragUV);
    vec3 N = normalize(getNormal());
    vec3 V = -normalize(fragPosition);
    vec3 L = normalize(sun.direction.xyz); //normalize((camera.view * vec4(normalize(vec3(1, 1, 1)), 0)).xyz); // arbitrary for now

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
    color *= sun.color.xyz * sun.intensity;

    /// Shadows - brief raymarch of low-res clouds

    // Ray intersection with the atmosphere slices
    /// Raytrace the scene (a sphere, to become the atmosphere)
    vec3 earthCenter = camera.cameraPosition.xyz;
    earthCenter.y = -ATMOSPHERE_RADIUS * 0.5 * 0.99;
    vec4 atmosphereSphereInner = vec4(earthCenter, ATMOSPHERE_RADIUS);
    float atmosphereThickness = 0.5 * ATMOSPHERE_RADIUS * 0.02;
    vec4 atmosphereSphereOuter = vec4(earthCenter, ATMOSPHERE_RADIUS * 1.02);
    Intersection atmosphereIsectInner = raySphereIntersection(fragPositionWC, sun.directionBasis[1].xyz, atmosphereSphereInner);
    Intersection atmosphereIsectOuter = raySphereIntersection(fragPositionWC, sun.directionBasis[1].xyz, atmosphereSphereOuter);

    float timeOffset = sky.wind.w;
    const float stepSize = 0.1f * atmosphereThickness;
    float t = atmosphereIsectInner.t;
    float accumDensity = 0.0;

    //const vec3 shadowDir = normalize(sun.direction.xyz);
    vec3 shadowRayOrigin = fragPositionWC * 4.0;

    for(int i = 0; i < NUM_SHADOW_STEPS; ++i) {
        vec3 currentPos = shadowRayOrigin + t * L;
       
        float coverage;
        vec3 currentProj = getProjectedShellPoint(currentPos, earthCenter);
        float rHeight = getRelativeHeight(currentPos, currentProj, atmosphereThickness);
        vec3 windOffset = WIND_STRENGTH * (sky.wind.xyz + vec3(0, 0.2 * rHeight, 0)) * (timeOffset + rHeight * 200.0);

        float density = cloudTest(currentPos + windOffset, rHeight, earthCenter, coverage);
        //accumDensity += density;
        accumDensity = max(density, accumDensity);
        
        if(accumDensity > 0.99) {
            accumDensity = 1.0;
            break;
        }
        t += stepSize;
    }

    color *= (1.0 - accumDensity);

    color += diffuse * mix(vec3(0), 2.0 * vec3(0.6, 0.7, 1.0), 0.5 + 0.5 * dot(N, normalize((camera.view * vec4(normalize(vec3(0, 1, 0)), 0)).xyz)));
    vec3 aoColor = mix(vec3(0.1, 0.1, 0.3), vec3(1), pbrParams.b);
    color.rgb *= aoColor;

    //color = sun.directionBasis[1].xyz;
    //color = abs(fragPositionWC);
    outColor = vec4(color, 1.0);
}