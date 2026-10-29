#include "physics/KinematicCharacterController.h"
#include "Engine.h"

#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

namespace eng
{
    KinematicCharacterController::KinematicCharacterController(float radius, float height) 
        :   m_radius(radius), m_height(height)

    {
        auto world = Engine::GetInstance().GetPhysicsManager().GetWorld();

        auto capsule = new btCapsuleShape(m_radius, m_height);

        m_ghost = std::make_unique<btPairCachingGhostObject>();
        btTransform start;
        start.setIdentity();
        start.setOrigin(btVector3(0.0f, 2.0f, 0.0f));
        m_ghost->setWorldTransform(start);
        m_ghost->setCollisionShape(capsule);
        m_ghost->setCollisionFlags(m_ghost->getCollisionFlags() | btCollisionObject::CF_CHARACTER_OBJECT);

        world->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());

        const btScalar stepHeight = 0.35f;
        m_controller = std::make_unique<btKinematicCharacterController>(m_ghost.get(), capsule, stepHeight);

        m_controller->setMaxSlope(btRadians(50.0f));
        m_controller->setGravity(world->getGravity());

        world->addCollisionObject(
            m_ghost.get(),
            btBroadphaseProxy::CharacterFilter,
            btBroadphaseProxy::AllFilter & ~btBroadphaseProxy::SensorTrigger);
        world->addAction(m_controller.get());
    }

    KinematicCharacterController::~KinematicCharacterController()
    {
        auto world = Engine::GetInstance().GetPhysicsManager().GetWorld();
        if (m_controller)
        {
            world->removeAction(m_controller.get());
        }
        if (m_ghost)
        {
            world->removeCollisionObject(m_ghost.get());
        }
    }

    glm::vec3 KinematicCharacterController::GetPosition() const
    {
        const auto& pos = m_ghost->getWorldTransform().getOrigin();
        const glm::vec3 offset(0.0f, m_height + 0.05f + m_radius, 0.0f);
        return glm::vec3(pos.x(), pos.y(), pos.z()) + offset;
    }

    glm::quat KinematicCharacterController::GetRotation() const
    {
        const auto& rot = m_ghost->getWorldTransform().getRotation();
        return glm::quat(rot.w(), rot.x(), rot.y(), rot.z());
    }

    void KinematicCharacterController::Walk(const glm::vec3& direction)
    {
        m_controller->setWalkDirection(btVector3(
            btScalar(direction.x), 
            btScalar(direction.y), 
            btScalar(direction.z)));
    }

    void KinematicCharacterController::Jump(const glm::vec3& direction)
    {
        if (m_controller->onGround())
        {
            m_controller->jump(btVector3(
                btScalar(direction.x),
                btScalar(direction.y),
                btScalar(direction.z)));
        }
    }

    bool KinematicCharacterController::OnGround() const
    {
        return m_controller->onGround();
    }
}