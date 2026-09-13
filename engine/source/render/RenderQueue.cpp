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

    void RenderQueue::Draw(GraphicsAPI& graphicsapi, const CameraData& cameradata)
    {
        for (auto& command : m_commands)
        {
            graphicsapi.BindMaterial(command.material);
            auto shaderProgram = command.material->GetShaderProgram();
            shaderProgram->SetUniform("uModel", command.modelMatrix);
            shaderProgram->SetUniform("uView", cameradata.viewMatrix);
            shaderProgram->SetUniform("uProjection", cameradata.projectionMatrix);
            graphicsapi.BindMesh(command.mesh);
            graphicsapi.DrawMesh(command.mesh);
        }

        m_commands.clear();
    }
}