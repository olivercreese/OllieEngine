#include "Game.h"
#include "TestObject.h"


bool Game::Init() 
{
    m_scene = new eng::Scene();

    auto camera = m_scene->CreateObject("Camera");
    camera->AddComponent(new eng::CameraComponent());
    camera->SetPosition(glm::vec3(0.0f, 0.0f, 2.0f));
    camera->AddComponent(new eng::PlayerControllerComponent());

    m_scene->SetMainCamera(camera);

    auto cube = m_scene->CreateObject<TestObject>("TestObject");
    cube->AddComponent(new eng::RotatingComponent(glm::vec3(0.0f, 1.0f, 0.0f), 45.0f));
    cube->SetPosition(glm::vec3(0.0f, 0.0f, -4.0f));
    auto cube1 = m_scene->CreateObject<TestObject>("TestObjectChild", cube);
    cube1->SetPosition(glm::vec3(2.0f, 2.0f, 0.0f));
    cube1->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

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