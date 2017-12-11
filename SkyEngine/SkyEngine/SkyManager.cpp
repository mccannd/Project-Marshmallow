#include "SkyManager.h"
#define SHADOW_CUTOFF 1.6110731557f
#define SHADOW_STEEPNESS 1.5f
#define EE 1000.0f
#define PI 3.14159265f
#define E 2.718281828459f
#define SUN_DISTANCE 400000.0f
#define MIE_CONST glm::vec3( 1.839991851443397f, 2.779802391966052f, 4.079047954386109f)
#define RAYLEIGH_TOTAL glm::vec3(5.804542996261093E-6, 1.3562911419845635E-5, 3.0265902468824876E-5)

float clamp(float t, float min, float max) {
    return std::max(min, std::min(max, t));
}

void SkyManager::calcSunColor() {

    if (sun.direction.y < 0.0f) {
        sun.color = glm::vec4(0.8f, 0.9f, 1.0f, sun.color.a);
    } else {
        glm::vec3 sunset = glm::vec3(2.f, 0.33922, 0.0431f);
        float t = (sun.direction.y) * 13.f;
        t = clamp(t, 0.f, 1.f);
        glm::vec3 color = (1.f - t) * sunset + (t)* glm::vec3(1.f);
        sun.color = glm::vec4(color, sun.color.a);
    }
}

void SkyManager::calcSunIntensity() {
    float zenithAngleCos = clamp(sun.direction.y, -1.f, 1.f);
    sun.intensity = EE * std::max(0.f, 1.f - powf(E, -((SHADOW_CUTOFF - acosf(zenithAngleCos)) / SHADOW_STEEPNESS)));
    if (sun.direction.y < 0.0f) {
        sun.intensity = 2.0f;
    }
}

void SkyManager::calcSkyBetaR() {
    float sunFade = 1.0f - clamp(1.0f - exp(sun.location.y / 450000.0), 0.0f, 1.0f);
    sky.betaR = glm::vec4(RAYLEIGH_TOTAL * (rayleigh - 1.f + sunFade), 0.0f); // UBO padding
}

void SkyManager::calcSkyBetaV() {
    float c = (0.2f * turbidity) * 10E-18f;
    sky.betaV = glm::vec4(0.434f * c * MIE_CONST * mie, 0.0f); // UBO padding
}

void SkyManager::calcSunPosition() {
    float theta = 2.0 * PI * (elevation - 0.5);
    float phi = 2.0 * PI * (azimuth - 0.5);
    (cos(phi), sin(phi) * sin(theta), sin(phi) * cos(theta)); // double-check this


    glm::vec3 dir = glm::vec3(cos(phi), sin(phi) * sin(theta), sin(phi) * cos(theta));
    //glm::normalize(glm::vec3(std::cos(elevation), cos(azimuth) * sin(elevation), sin(azimuth) * cos(elevation))); // not terribly realistic / standard, but...
    sun.direction = glm::vec4(dir, 0.0f);
    //sun.direction *= -1.f;
    sun.location = glm::vec4(SUN_DISTANCE * dir, 1.0f); // assume center is 0 0 0 for sky
    if (sun.direction.y < 0.0f) {
        sun.location *= -1.0f;
    }
    sun.directionBasis = glm::mat4(1);
    glm::vec3 right = (abs(sun.direction.y) < 0.001f) ? ((sun.direction.z < 0) ? glm::vec3(0, 1, 0) : glm::vec3(0, -1, 0)) : glm::vec3(0, 0, 1);
    glm::vec3 dirN = glm::normalize(glm::cross(dir, right));
    glm::vec3 dirB = glm::normalize(glm::cross(dir, dirN));
    sun.directionBasis[1] = sun.direction;
    sun.directionBasis[0] = glm::vec4(dirN, 0.0f);
    sun.directionBasis[2] = glm::vec4(dirB, 0.0f);
    if (sun.direction.y < 0.0f) {
        sun.directionBasis *= -1.0f;
    }
}

SkyManager::SkyManager()
{
    elevation = PI / 4.f; // 0 is sunrise, PI is sunset
    azimuth = PI / 8.f; // here: tilt about x axis with y up. not realistic, but ok for 'seasonal tilt'
    //sun = {};
    //sky = {};
    sun = {
        glm::vec4(0.f),
        glm::vec4(0.f),
        glm::vec4(0.f),
        glm::mat4(1.f),
        0.0f
    };
    sky = {
        glm::vec4(0.f),
        glm::vec4(0.f),
        glm::vec4(1, 0.05, 1, 0),
        0.f,
    };
    sun.color = glm::vec4(1, 1, 1, 0); // TODO. Note, alpha channel has to start at 0 as it serves a much different purpose
    calcSunPosition();
    calcSunColor();
    calcSunIntensity();
    mie = 0.005f;
    sky.mie_directional = 0.8;
    rayleigh = 2.f;
    calcSkyBetaR();
    calcSkyBetaV();
}


SkyManager::~SkyManager()
{
}


void SkyManager::rebuildSkyFromNewSun(float elevation, float azimuth) {
    this->elevation = elevation;
    this->azimuth = azimuth;
    calcSunPosition();
    calcSunIntensity();
    calcSunColor();
    calcSkyBetaR();
    calcSkyBetaV();
}

void SkyManager::rebuildSkyFromScattering(float turbidity, float mie, float mie_directional) {
    sky.mie_directional = mie_directional;
    this->mie = mie;
    calcSkyBetaR();
    calcSkyBetaV();
}

void SkyManager::rebuildSky(float elevation, float azimuth, float turbidity, float mie, float mie_directional) {
    this->elevation = elevation;
    this->azimuth = azimuth;
    calcSunPosition();
    calcSunIntensity();
    calcSunColor();
    sky.mie_directional = mie_directional;
    this->mie = mie;
    calcSkyBetaR();
    calcSkyBetaV();
}