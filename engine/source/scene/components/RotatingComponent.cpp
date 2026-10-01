#include "RotatingComponent.h"
#include <glm/gtc/quaternion.hpp>
#include <Engine.h>

namespace eng
{
    void RotatingComponent::Update(float deltaTime)
    {
        glm::quat spin = glm::angleAxis(glm::radians(m_speed) * deltaTime, m_axis);
        m_owner->SetRotation(glm::normalize(spin * m_owner->GetRotation()));
    }
}