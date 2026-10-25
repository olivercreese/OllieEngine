#include "Game.h"
#include "TestObject.h"
#include <glm/gtc/quaternion.hpp>


bool Game::Init() 
{
    m_scene = new eng::Scene();
    eng::Engine::GetInstance().SetScene(m_scene);

    //////  camera object 
    auto camera = m_scene->CreateObject("Camera");
    camera->AddComponent(new eng::CameraComponent());
    camera->SetPosition(glm::vec3(0.0f, 0.0f, 2.0f));
    camera->SetRotation(glm::angleAxis(glm::radians(-40.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
    camera->AddComponent(new eng::PlayerControllerComponent());

    m_scene->SetMainCamera(camera);
    /////

    ////// orbiting cubes 
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
    ///////

    ///suzanne gltf model 
    auto suzanneObject = eng::GameObject::LoadGLTF("models/suzanne/Suzanne.gltf");
    suzanneObject->SetPosition(glm::vec3(0.0f, -8.0f, -6.0f));
    ///////////
    
    ///// gun object 
    auto gun = eng::GameObject::LoadGLTF("models/sten_gunmachine_carbine/scene.gltf");
    gun->SetParent(camera);
    gun->SetPosition(glm::vec3(0.75f, -0.5f, -0.75f));
    gun->SetScale(glm::vec3(-1.0f, 1.0f, 1.0f));
    ///gun object animation
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
    //////
    
    //lighting
    auto light = m_scene->CreateObject("light");
    auto lightcomp = new eng::LightComponent();
    lightcomp->SetColor(glm::vec3(1.0f));
    light->AddComponent(lightcomp);
    light->SetPosition(glm::vec3(0.0f, 0.0f, 10.0f));
    //////
    
    //////ground physics object
    auto ground = m_scene->CreateObject("Ground");
    ground->SetPosition(glm::vec3(0.0f, -5.0f, 0.0f));

    glm::vec3 groundExtents(20.0f, 2.02f, 20.0f);
    auto groundMesh = eng::Mesh::CreateBox(groundExtents);
    auto material = eng::Material::Load("materials/brick.mat");
    ground->AddComponent(new eng::MeshComponent(groundMesh, material));

    auto groundCollider = std::make_shared<eng::BoxCollider>(groundExtents);
    auto groundBody = std::make_shared<eng::RigidBody>(eng::BodyType::Static,
        groundCollider, 0.0f, 0.5f);
    ground->AddComponent(new eng::PhysicsComponent(groundBody));
    ///////////

    /// falling box object 
    auto boxObj = m_scene->CreateObject<TestObject>("Falling Box");
    boxObj->SetPosition(glm::vec3(0.0f, 2.0f, 2.0f));
    boxObj->SetRotation(glm::quat(glm::vec3(1.0f, 2.0f, 0.0f)));
    auto boxCollider = std::make_shared<eng::BoxCollider>(glm::vec3(1.0f));
    auto boxBody = std::make_shared<eng::RigidBody>(
        eng::BodyType::Dynamic, boxCollider, 5.0f, 0.5f);
    boxObj->AddComponent(new eng::PhysicsComponent(boxBody));

    camera->SetPosition(glm::vec3(0.0f, 1.0f, 7.0f));

    return true;
}

void Game::Update(float deltaTime) 
{
    m_scene->Update(deltaTime);


}

void Game::Destroy() 
{


}