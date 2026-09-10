#pragma once

#include <eng.h>

class TestObject : public eng::GameObject
{
public:
    TestObject();

    void Update(float deltaTime) override;

private:
    eng::Material m_material;
    std::shared_ptr<eng::Mesh> m_mesh;
};