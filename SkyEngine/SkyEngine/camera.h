#pragma once
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif // !GLM_FORCE_RADIANS



#define RAD2DEG 57.2957f
#define DEG2RAD 0.01745f
#define EPSILON 0.00001f

class Camera
{
private:

	glm::vec3 m_position;
	glm::vec3 m_forward;
	glm::vec3 m_right;
	glm::vec3 m_up;

	float m_near = 0.1f;
	float m_far = 100.0f;
	float m_fov = 45.0f;
	float m_aspect = 800.0f / 600.0f;

	glm::mat4 m_proj;

	float m_pitch;
	float m_yaw;

	bool m_lockedTarget = false;
	glm::vec3 m_target;

    double lastY;
    double lastX;
    bool firstMouse = true;

public:
	enum directions {
		FORWARD,
		BACKWARD,
		UP,
		DOWN,
		LEFT,
		RIGHT
	};
	Camera();
	Camera(glm::vec3 pos, glm::vec3 target);
	Camera(glm::vec3 pos, glm::vec3 target, float newNear, float newFar, float fov);
	~Camera();

	glm::mat4 getView();
	glm::mat4 getProj();
	glm::mat4 getViewProj();
	glm::vec3 getPosition();
    float getAspect() { return m_aspect; }
    float getHTanFov() { return std::tan(0.5f * DEG2RAD * m_fov); }

	void movePosition(Camera::directions direction, float delta);
    void addPitch(float delta); // rotate by delta degrees about right
    void addYaw(float delta); // rotate by delta degrees about up
    void addPitchLocal(float delta); // rotate by delta degrees about right
    void addYawLocal(float delta); // rotate by delta degrees about up
	void setPosition(const glm::vec3 &newPosition);
	void lookAt(const glm::vec3 &target);

	void setAspect(float width, float height);
	void setFOV(float fov);
	void setFrustum(float newNear, float newFar, float fov);

	void beginTarget(const glm::vec3 &target);
	void endTarget();

    void mouseRotate(double x, double y);
};

