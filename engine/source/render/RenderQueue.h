#pragma once
#include "Common.h"
#include <vector>
#include <glm/mat4x4.hpp>

namespace eng
{
    class Mesh;
    class Material;
    class GraphicsAPI;

    struct RenderCommand
    {
        Mesh* mesh = nullptr;
        Material* material = nullptr;
        glm::mat4 modelMatrix;
    };


    class RenderQueue
    {
    public:
        void Submit(const RenderCommand& command);
        void Draw(GraphicsAPI& graphicsapi, const CameraData& cameradata, const std::vector<LightData>& lights);
    private:
        std::vector<RenderCommand> m_commands;
    };
}