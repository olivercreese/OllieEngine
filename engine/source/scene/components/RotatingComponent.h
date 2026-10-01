#pragma once
#include "scene/Component.h"
#include <glm/vec3.hpp>

namespace eng
{
    class RotatingComponent : public Component
    {
        COMPONENT(RotatingComponent)
    public:
        RotatingComponent(const glm::vec3& axis, float degreesPerSecond)
            : m_axis(axis), m_speed(degreesPerSecond) {}
        void Update(float deltaTime) override;
    private:
        glm::vec3 m_axis;
        float m_speed;
    };
}