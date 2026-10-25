#include "TestObject.h"
#include <GLFW/glfw3.h>
#include <iostream>


TestObject::TestObject()
{
    
    auto material = eng::Material::Load("materials/brick.mat");
    
    glm::vec3 boxExtents = glm::vec3(1.0f);
    auto mesh = eng::Mesh::CreateBox(boxExtents);
    AddComponent(new eng::MeshComponent(mesh, material));
}

void TestObject::Update(float deltaTime)
{
    eng::GameObject::Update(deltaTime);
}