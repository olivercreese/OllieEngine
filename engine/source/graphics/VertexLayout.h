#pragma once
#include "GL/glew.h"
#include <vector>
#include <stdint.h>

namespace eng
{
    struct VertexElement
    {
        GLuint index; // attribute location
        GLuint size; // number of componenet
        GLuint type; // data type(e.g GL_FLOAT)
        uint32_t offset; // bytes offset from start of vertex

        static constexpr int PositionIndex = 0;
        static constexpr int ColorIndex = 1;
        static constexpr int UVIndex = 2;
        static constexpr int NormalIndex = 3;
    };
    struct VertexLayout
    {
        std::vector<VertexElement> elements;
        uint32_t stride = 0; // totla size of a single vertex
    };
}