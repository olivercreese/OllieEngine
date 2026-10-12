#include "TestObject.h"
#include <GLFW/glfw3.h>
#include <iostream>


TestObject::TestObject()
{
    
    auto material = eng::Material::Load("materials/brick.mat");
    
    auto mesh = eng::Mesh::CreateCube();
    AddComponent(new eng::MeshComponent(mesh, material));
}

void TestObject::Update(float deltaTime)
{
    eng::GameObject::Update(deltaTime);
}