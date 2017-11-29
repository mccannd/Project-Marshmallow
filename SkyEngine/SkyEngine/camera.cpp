#include "camera.h"

Camera::Camera()
{
	m_position = glm::vec3(0, 0, -3);
	m_forward = glm::normalize(m_position);
	glm::vec3 wUp = glm::vec3(0, 1, 0);
	m_right = glm::normalize(glm::cross(m_forward, wUp));
	m_up = glm::normalize(glm::cross(m_right, m_forward));
	m_proj = glm::perspective(DEG2RAD * m_fov, m_aspect, m_near, m_far);
}

Camera::Camera(glm::vec3 pos, glm::vec3 target)
{
	setPosition(pos);
	lookAt(target);
	m_proj = glm::perspective(DEG2RAD * m_fov, m_aspect, m_near, m_far);
}

Camera::Camera(glm::vec3 pos, glm::vec3 target, float newNear, float newFar, float fov)
{
	setPosition(pos);
	lookAt(target);
	setFrustum(newNear, newFar, fov);
}

glm::mat4 Camera::getView()
{
	glm::mat4 trans = glm::mat4(1.0);
	trans[3] = glm::vec4(-m_position.x, -m_position.y, -m_position.z, 1.0);

	glm::mat4 view = glm::mat4(1.0);
	view[0] = glm::vec4(m_right.x, m_up.x, m_forward.x, 0.0);
	view[1] = glm::vec4(m_right.y, m_up.y, m_forward.y, 0.0);
	view[2] = glm::vec4(m_right.z, m_up.z, m_forward.z, 0.0);

    return view * trans;
}

glm::mat4 Camera::getProj()
{
	return m_proj;
}

glm::mat4 Camera::getViewProj()
{
	return getView() * getProj();
}

glm::vec3 Camera::getPosition()
{
	return m_position;
}

void Camera::movePosition(Camera::directions direction, float delta)
{
	glm::vec3 dir;
	switch (direction) {
	case FORWARD:
		dir = glm::vec3(-m_forward);
		break;
	case BACKWARD:
		dir = glm::vec3(m_forward);
		break;
	case UP:
        dir = glm::vec3(0, 1, 0);
		break;
	case DOWN:
        dir = glm::vec3(0, -1, 0);
		break;
	case RIGHT:
		dir = glm::vec3(m_right);
		break;
	case LEFT:
		dir = glm::vec3(-m_right);
		break;
	default: return;
	}
	m_position += delta * dir;

	if (m_lockedTarget) {
		lookAt(m_target);
	}
}

glm::mat3 angleAxis(const glm::vec3 &u, float deltaDeg) {
    float angle = DEG2RAD * deltaDeg;
    float cost = std::cos(angle);
    float sint = std::sin(angle);

    glm::mat3 rot = glm::mat3(1.0f);
    rot[0] = glm::vec3(
        cost + u.x * u.x * (1.f - cost),
        u.y * u.x * (1.f - cost) + u.z * sint,
        u.z * u.x * (1.f - cost) - u.y * sint
        );
    rot[1] = glm::vec3(
        u.x * u.y * (1.f - cost) - u.z * sint,
        cost + u.y * u.y * (1.f - cost),
        u.z * u.y * (1.f - cost) + u.x * sint
        );
    rot[2] = glm::vec3(
        u.x * u.z * (1.f - cost) + u.y * sint,
        u.y * u.z * (1.f - cost) - u.x * sint,
        cost + u.z * u.z * (1.f - cost)
        );
    return rot;
}

void Camera::addPitchLocal(float delta) {
    if (m_lockedTarget) return;
    glm::mat3 rot = angleAxis(m_right, delta);
    m_forward = glm::normalize(rot * m_forward);
    m_up = glm::normalize(rot * m_up);
}

void Camera::addYawLocal(float delta) {
    if (m_lockedTarget) return;
    glm::mat3 rot = angleAxis(m_up, delta);
    m_right = glm::normalize(rot * m_right);
    m_forward = glm::normalize(rot * m_forward);
}

void Camera::addPitch(float delta) {
    if (m_lockedTarget) return;
    m_pitch += delta;
    if (RAD2DEG * m_pitch < -89.0f) m_pitch = DEG2RAD * -89.0f;
    if (RAD2DEG * m_pitch > 89.0f) m_pitch = DEG2RAD * 89.0f;
    m_forward = glm::vec3(std::cos(m_yaw) * std::cos(m_pitch), std::sin(m_pitch), std::sin(m_yaw) * std::cos(m_pitch));
    glm::vec3 wUp = (1.0f - std::abs(glm::dot(m_forward, glm::vec3(0, 1, 0))) < EPSILON) ?
        glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
    m_right = glm::normalize(glm::cross(m_forward, wUp));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
}

void Camera::addYaw(float delta) {
    if (m_lockedTarget) return;
    m_yaw += delta;
    m_forward = glm::vec3(std::cos(m_yaw) * std::cos(m_pitch), std::sin(m_pitch), std::sin(m_yaw) * std::cos(m_pitch));
    glm::vec3 wUp = (1.0f - std::abs(glm::dot(m_forward, glm::vec3(0, 1, 0))) < EPSILON) ?
        glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
    m_right = glm::normalize(glm::cross(m_forward, wUp));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
}


void Camera::setPosition(const glm::vec3 &newPosition)
{
	m_position = newPosition;
	if (m_lockedTarget) lookAt(m_target);
}

void Camera::setAspect(float width, float height)
{
	if (width < EPSILON || height < EPSILON) return;
	m_aspect = width / height;
	m_proj = glm::perspective(DEG2RAD * m_fov, m_aspect, m_near, m_far);
}

void Camera::setFOV(float fov)
{
	if (fov < EPSILON) return;
	m_fov = fov;
	m_proj = glm::perspective(DEG2RAD * m_fov, m_aspect, m_near, m_far);
}

void Camera::setFrustum(float newNear, float newFar, float fov)
{
	if (fov < EPSILON || newNear < EPSILON || newFar - newNear < EPSILON) return;
	m_fov = fov;
	m_near = newNear;
	m_far = newFar;
	m_proj = glm::perspective(DEG2RAD * m_fov, m_aspect, m_near, m_far);
}

void Camera::lookAt(const glm::vec3 &target)
{
    glm::vec3 toTarget = glm::normalize(target - m_position);
    glm::vec3 projected = glm::normalize(glm::vec3(toTarget.x, 0, toTarget.z));
    m_pitch = std::acos(glm::dot(toTarget, projected));
    m_yaw = std::atan2(projected.x, projected.z);

    m_forward = glm::vec3(std::cos(m_yaw) * std::cos(m_pitch), std::sin(m_pitch), std::sin(m_yaw) * std::cos(m_pitch));
	glm::vec3 wUp = (1.0f - std::abs(glm::dot(m_forward, glm::vec3(0, 1, 0))) < EPSILON) ?
		glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
	m_right = glm::normalize(glm::cross(m_forward, wUp));
	m_up = glm::normalize(glm::cross(m_right, m_forward));

	if (m_lockedTarget && glm::length(m_target - target) > EPSILON) {
		m_lockedTarget = false;
	}
}

Camera::~Camera()
{
}

void Camera::beginTarget(const glm::vec3 &target)
{
	m_target = glm::vec3(target);
	lookAt(m_target);
	m_lockedTarget = true;
}

void Camera::endTarget()
{
	m_lockedTarget = false;
}

void Camera::mouseRotate(double x, double y) {
    if (firstMouse)
    {
        lastX = x;
        lastY = y;
        firstMouse = false;
    }

    float xoffset = static_cast<float>(lastX - x);
    float yoffset = static_cast<float>(y - lastY); // borf
    lastX = x;
    lastY = y;

    float sensitivity = 0.01f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;

    if (RAD2DEG * m_pitch < -89.0f) m_pitch = DEG2RAD * -89.0f;
    if (RAD2DEG * m_pitch > 89.0f) m_pitch = DEG2RAD * 89.0f;

    m_forward = glm::vec3(std::cos(m_yaw) * std::cos(m_pitch), std::sin(m_pitch), std::sin(m_yaw) * std::cos(m_pitch));
    glm::vec3 wUp = (1.0f - std::abs(glm::dot(m_forward, glm::vec3(0, 1, 0))) < EPSILON) ?
        glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
    m_right = glm::normalize(glm::cross(m_forward, wUp));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
}
