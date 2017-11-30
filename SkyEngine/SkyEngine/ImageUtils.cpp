#include "ImageUtils.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <algorithm>

#define CURL_DIM 128
#define EPS 0.001

const glm::vec3 basis[12]{
    glm::vec3(0.7071, 0.7071, 0),
    glm::vec3(0.7071, -0.7071, 0),
    glm::vec3(-0.7071, 0.7071, 0),
    glm::vec3(-0.7071, -0.7071, 0),
    glm::vec3(0.7071, 0, 0.7071),
    glm::vec3(0.7071, 0, -0.7071),
    glm::vec3(-0.7071, 0, 0.7071),
    glm::vec3(-0.7071, 0, -0.7071),
    glm::vec3(0, 0.7071, 0.7071),
    glm::vec3(0, 0.7071, -0.7071),
    glm::vec3(0, -0.7071, 0.7071),
    glm::vec3(0, -0.7071, -0.7071),
};                    

float hashNoise(float x, float y, float z) {
    float n = sin(dot(glm::vec3(x, y, z), glm::vec3(12.9898f, 78.233f, 47.387))) * 43758.5453f;
    n = n - floor(n);
    return n;
}

glm::vec3 hashVec(float x, float y, float z) {
    float a = hashNoise(x, y, z);
    return basis[(int)floor(12.f * a)];
}

float lerp(float a, float b, float t) {
    return (1.0 - t) * a + t * b;
}

// given normalized point and frequency
float perlinNoise(glm::vec3 pt_normal, float freq) {
    pt_normal *= freq;
    glm::vec3 f = glm::vec3(floor(pt_normal.x), floor(pt_normal.y), floor(pt_normal.z));
    glm::vec3 r = pt_normal - f;
    glm::vec3 u = r * r * r * (r * (r * 6.0f - glm::vec3(15.0f)) + glm::vec3(10.0f)); // smootherstep

    // force tiling boundaries
    glm::vec3 f1 = glm::vec3(f) + glm::vec3(1);
    if (fabs(f1.x - freq) < 0.001f) f1.x = freq;
    if (fabs(f1.y - freq) < 0.001f) f1.y = freq;
    if (fabs(f1.z - freq) < 0.001f) f1.z = freq;

    // 8 Basis vectors dot products
    float nnn = glm::dot(hashVec( f.x,  f.y,  f.z  ), r);
    float nnp = glm::dot(hashVec( f.x,  f.y,  f1.z ), r - glm::vec3(0, 0, 1));
    float npn = glm::dot(hashVec( f.x,  f1.y, f.z  ), r - glm::vec3(0, 1, 0));
    float npp = glm::dot(hashVec( f.x,  f1.y, f1.z ), r - glm::vec3(0, 1, 1));
    float pnn = glm::dot(hashVec( f1.x, f.y,  f.z  ), r - glm::vec3(1, 0, 0));
    float pnp = glm::dot(hashVec( f1.x, f.y,  f1.z ), r - glm::vec3(1, 0, 1));
    float ppn = glm::dot(hashVec( f1.x, f1.y, f.z  ), r - glm::vec3(1, 1, 0));
    float ppp = glm::dot(hashVec( f1.x, f1.y, f1.z ), r - glm::vec3(1, 1, 1));

    float nn = lerp(nnn, pnn, u.x);
    float np = lerp(nnp, pnp, u.x);
    float pn = lerp(npn, ppn, u.x);
    float pp = lerp(npp, ppp, u.x);

    float n = lerp(nn, pn, u.y);
    float p = lerp(np, pp, u.y);

    return lerp(n, p, u.z);

}

glm::vec3 curlNoise(glm::vec2 pt, float freq) {
    float a, b;

    // dydx
    a = perlinNoise(glm::vec3(pt.x - EPS, pt.y, 0.5f), freq);
    b = perlinNoise(glm::vec3(pt.x + EPS, pt.y, 0.5f), freq);
    float dydx = (b - a) / (2.f * EPS);

    // dxdy
    a = perlinNoise(glm::vec3(pt.x, pt.y - EPS, 0.5f), freq);
    b = perlinNoise(glm::vec3(pt.x, pt.y + EPS, 0.5f), freq);
    float dxdy = (b - a) / (2.f * EPS);



    // dxdz
    a = perlinNoise(glm::vec3(pt.x, 0.5f, pt.y - EPS), freq);
    b = perlinNoise(glm::vec3(pt.x, 0.5f, pt.y + EPS), freq);
    float dxdz = (b - a) / (2.f * EPS);

    // dzdx
    a = perlinNoise(glm::vec3(pt.x - EPS, 0.5f, pt.y), freq);
    b = perlinNoise(glm::vec3(pt.x + EPS, 0.5f, pt.y), freq);
    float dzdx = (b - a) / (2.f * EPS);



    // dzdy
    a = perlinNoise(glm::vec3(0.5, pt.y - EPS, pt.x), freq);
    b = perlinNoise(glm::vec3(0.5, pt.y + EPS, pt.x), freq);
    float dzdy = (b - a) / (2.f * EPS);

    // dydz
    a = perlinNoise(glm::vec3(0.5f, pt.y, pt.x - EPS), freq);
    b = perlinNoise(glm::vec3(0.5f, pt.y, pt.x + EPS), freq);
    float dydz = (b - a) / (2.f * EPS);


    return glm::vec3(dzdy - dydz, dxdz - dzdx, dydx - dxdy);
}

float remap(float x, float oldMin, float oldMax, float newMin, float newMax) {
    float m = newMin + ((x - oldMin) / (oldMax - oldMin) * (newMax - newMin));
    return m;
}

void GenerateCurlNoise(std::string path) {
    unsigned char* pixels = new unsigned char[4 * CURL_DIM * CURL_DIM];
    glm::vec3* curls = new glm::vec3[CURL_DIM * CURL_DIM];
    float lowerBoundx = FLT_MAX;
    float upperBoundx = -FLT_MAX;
    float lowerBoundy = FLT_MAX;
    float upperBoundy = -FLT_MAX;
    float lowerBoundz = FLT_MAX;
    float upperBoundz = -FLT_MAX;
    // create an interleaved image
    for (int row = 0; row < CURL_DIM; row++) {
        for (int col = 0; col < CURL_DIM; col++) {
            glm::vec3 noise = curlNoise(glm::vec2((float)col / CURL_DIM, (float)row / CURL_DIM), 8.f);
            curls[row * CURL_DIM + col] = noise;
            lowerBoundx = std::min(lowerBoundx, noise.x);
            upperBoundx = std::max(upperBoundx, noise.x);
            lowerBoundy = std::min(lowerBoundy, noise.y);
            upperBoundy = std::max(upperBoundy, noise.y);
            lowerBoundz = std::min(lowerBoundz, noise.z);
            upperBoundz = std::max(upperBoundz, noise.z);
        }
    }

    for (int i = 0; i < CURL_DIM * CURL_DIM; i++) {
        curls[i].x = remap(curls[i].x, lowerBoundx, upperBoundx, 0.f, 1.f);
        curls[i].y = remap(curls[i].y, lowerBoundy, upperBoundy, 0.f, 1.f);
        curls[i].z = remap(curls[i].z, lowerBoundz, upperBoundz, 0.f, 1.f);
    }

    for (int row = 0; row < CURL_DIM; row++) {
        for (int col = 0; col < CURL_DIM; col++) {
            glm::vec3 noise = curls[row * CURL_DIM + col];
            unsigned char R = (unsigned char)((int)roundf(noise.x * 255.f));
            unsigned char G = (unsigned char)((int)roundf(noise.y * 255.f));
            unsigned char B = (unsigned char)((int)roundf(noise.z * 255.f));
            unsigned char A = (unsigned char)255;
            int pxStart = 4 * (row * CURL_DIM + col);
            pixels[pxStart] = R;
            pixels[pxStart + 1] = G;
            pixels[pxStart + 2] = B;
            pixels[pxStart + 3] = A;
        }
    }
    stbi_write_tga(path.c_str(), CURL_DIM, CURL_DIM, 4, pixels);
    delete pixels;
    delete curls;
}