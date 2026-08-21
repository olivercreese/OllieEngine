#pragma once
#include <GL/glew.h>
#include <memory>
#include <vector>
#include <string>

namespace eng
{
    class ShaderProgram;
    class Material;

    class GraphicsAPI
    {
    public:
        std::shared_ptr<ShaderProgram> CreateShaderProgram(const std::string& vertexSource, 
                                                            const std::string& fragmentSource);

        GLuint CreateVertexBuffer(const std::vector<float>& vertices);
        GLuint CreateIndexBuffer(const std::vector<uint32_t>& indices);

        void BindShaderProgram(ShaderProgram* shaderProgram);
        void BindMaterial(Material* material);


    };
}