#include "Game.h"
#include "TestObject.h"
#include <glm/gtc/quaternion.hpp>


bool Game::Init() 
{
    m_scene = new eng::Scene();
    eng::Engine::GetInstance().SetScene(m_scene);

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

    auto suzanneObject = eng::GameObject::LoadGLTF("models/suzanne/Suzanne.gltf");
    suzanneObject->SetPosition(glm::vec3(0.0f, -8.0f, -6.0f));

    auto gun = eng::GameObject::LoadGLTF("models/sten_gunmachine_carbine/scene.gltf");
    gun->SetParent(camera);
    gun->SetPosition(glm::vec3(0.75f, -0.5f, -0.75f));
    gun->SetScale(glm::vec3(-1.0f, 1.0f, 1.0f));

    if (auto anim = gun->GetComponent<eng::AnimationComponent>())
    {
        if (auto bullet = gun->FindChildByName("bullet_33"))
        {
            bullet->SetActive(false);
        }

        if (auto fire = gun->FindChildByName("BOOM_35"))
        {
            fire->SetActive(false);
        }

        anim->Play("shoot", false);
    }

    auto light = m_scene->CreateObject("Ligth");
    auto lightcomp = new eng::LightComponent();
    lightcomp->SetColor(glm::vec3(1.0f));
    light->AddComponent(lightcomp);
    light->SetPosition(glm::vec3(0.0f, 0.0f, 10.0f));


    return true;
}

void Game::Update(float deltaTime) 
{
    m_scene->Update(deltaTime);


}

void Game::Destroy() 
{


}