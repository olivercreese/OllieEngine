#include "Game.h"
#include "TestObject.h"
#include <glm/gtc/quaternion.hpp>


bool Game::Init() 
{
    m_scene = new eng::Scene();

    auto camera = m_scene->CreateObject("Camera");
    camera->AddComponent(new eng::CameraComponent());
    camera->SetPosition(glm::vec3(0.0f, 0.0f, 2.0f));
    camera->SetRotation(glm::angleAxis(glm::radians(-40.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
    camera->AddComponent(new eng::PlayerControllerComponent());

    m_scene->SetMainCamera(camera);

    auto cube = m_scene->CreateObject<TestObject>("TestObject");
    cube->AddComponent(new eng::RotatingComponent(glm::vec3(0.0f, 1.0f, 0.0f), 100.0f));
    cube->SetScale(glm::vec3(0.75f, 0.75f, 0.75f));
    cube->SetPosition(glm::vec3(0.0f, 0.0f, -3.0f));
    auto cube1 = m_scene->CreateObject<TestObject>("TestObjectChild", cube);
    cube1->SetPosition(glm::vec3(2.0f, 0.75f, 0.0f));
    cube1->SetScale(glm::vec3(0.25f, 0.25f, 0.25f));
    auto cube2 = m_scene->CreateObject<TestObject>("TestObject2");
    cube2->AddComponent(new eng::RotatingComponent(glm::vec3(0.0f, 1.0f, 0.0f), 60.0f));
    cube2->SetPosition(glm::vec3(0.0f, -8.0f, -10.0f));
    cube2->SetScale(glm::vec3(2.0f, 2.0f, 2.0f));
    m_scene->SetParent(cube, cube2);

    eng::Engine::GetInstance().SetScene(m_scene);

    return true;
}

void Game::Update(float deltaTime) 
{
    m_scene->Update(deltaTime);


}

void Game::Destroy() 
{


}