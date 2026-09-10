#include "render/RenderQueue.h"
#include "render/Mesh.h"
#include "render/Material.h"
#include "graphics/GraphicsAPI.h"

namespace eng
{
    void RenderQueue::Submit(const RenderCommand& command) 
    {
        m_commands.push_back(command);
    }

    void RenderQueue::Draw(GraphicsAPI& graphicsapi)
    {
        for (auto& command : m_commands)
        {
            graphicsapi.BindMaterial(command.material);
            graphicsapi.BindMesh(command.mesh);
            graphicsapi.DrawMesh(command.mesh);
        }

        m_commands.clear();
    }
}