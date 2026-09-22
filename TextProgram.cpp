#include "TextProgram.hpp"
#include "gl_compile_program.hpp"

TextProgram::TextProgram() {
    program = gl_compile_program(R"GLSL(#version 330
layout(location = 0) in vec2 Position;
layout(location = 1) in vec2 TexCoord;
uniform vec2 Viewport;
uniform vec2 Origin;
out vec2 uv;
void main() {
    vec2 p = Position + Origin;
    gl_Position = vec4(2.0 * p.x / Viewport.x - 1.0,
                       1.0 - 2.0 * p.y / Viewport.y, 0.0, 1.0);
    uv = TexCoord;
}
)GLSL", R"GLSL(#version 330
uniform sampler2D Atlas;
uniform vec4 TextColor;
in vec2 uv;
out vec4 fragColor;
void main() {
    float coverage = texture(Atlas, uv).r;
    fragColor = vec4(TextColor.rgb, TextColor.a * coverage);
}
)GLSL");
    viewport = glGetUniformLocation(program, "Viewport");
    origin = glGetUniformLocation(program, "Origin");
    color = glGetUniformLocation(program, "TextColor");
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "Atlas"), 0);
    glUseProgram(0);
}

TextProgram::~TextProgram() {
    if (program) glDeleteProgram(program);
}
