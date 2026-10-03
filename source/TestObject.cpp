#include "TestObject.h"
#include <GLFW/glfw3.h>
#include <iostream>


TestObject::TestObject()
{
    
    auto material = eng::Material::Load("materials/brick.mat");

    std::vector<float> vertices =
    {
        // Front face
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,

        // Top face 
        0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,

        // Right face
        0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,

        // Left face
        -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,

        // Bottom face
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,

        // Back face
        -0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f

    };

    std::vector<unsigned int> indices =
    {
        //front face
        0, 1, 2,
        0, 2, 3,
        //top face
        4, 5, 6,
        4, 6, 7,
        //right face
        8, 9, 10,
        8, 10, 11,
        //left face
        12, 13, 14,
        12, 14, 15,
        //bottom face
        16, 17, 18,
        16, 18, 19,
        //back face
        20, 21, 22,
        20, 22, 23
    };

    eng::VertexLayout vertexLayout;

    //position
    vertexLayout.elements.push_back({
        0,
        3,
        GL_FLOAT,
        0
        });
    //colour
    vertexLayout.elements.push_back({
        1,
        3,
        GL_FLOAT,
        sizeof(float) * 3
        });
    //UV
    vertexLayout.elements.push_back({
        2,
        2,
        GL_FLOAT,
        sizeof(float) * 6
        });
    vertexLayout.stride = sizeof(float) * 8;

    auto mesh = std::make_shared<eng::Mesh>(vertexLayout, vertices, indices);
    AddComponent(new eng::MeshComponent(mesh, material));
}

void TestObject::Update(float deltaTime)
{
    eng::GameObject::Update(deltaTime);
}