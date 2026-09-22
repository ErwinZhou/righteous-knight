#pragma once
#include "GL.hpp"

struct TextProgram {
    TextProgram();
    ~TextProgram();
    TextProgram(TextProgram const &) = delete;
    TextProgram &operator=(TextProgram const &) = delete;
    GLuint program = 0;
    GLint viewport = -1;
    GLint origin = -1;
    GLint color = -1;
};
