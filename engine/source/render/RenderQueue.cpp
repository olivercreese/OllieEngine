#include "render/RenderQueue.h"
#include "render/Mesh.h"
#include "render/Material.h"
#include "graphics/GraphicsAPI.h"
#include "graphics/ShaderProgram.h"

namespace eng
{
    void RenderQueue::Submit(const RenderCommand& command) 
    {
        m_commands.push_back(command);
    }

    void RenderQueue::Draw(GraphicsAPI& graphicsapi, const CameraData& cameradata, const std::vector<LightData>& lights)
    {
        for (auto& command : m_commands)
        {
            graphicsapi.BindMaterial(command.material);
            auto shaderProgram = command.material->GetShaderProgram();
            shaderProgram->SetUniform("uModel", command.modelMatrix);
            shaderProgram->SetUniform("uView", cameradata.viewMatrix);
            shaderProgram->SetUniform("uProjection", cameradata.projectionMatrix);
            if (!lights.empty())
            {
                auto& light = lights[0];
                shaderProgram->SetUniform("uLight.color", light.color);
                shaderProgram->SetUniform("uLight.position", light.position);

            }
            
            graphicsapi.BindMesh(command.mesh);
            graphicsapi.DrawMesh(command.mesh);
        }

        m_commands.clear();
    }
}